#include <asm/barrier.h>
#include <linux/build_bug.h>
#include <linux/compiler.h>
#include <linux/string.h>
#include <linux/types.h>

#include "includes/ring.h"

static void axivc_ring_check_layout(void)
{
	/* Ring layout is wire-visible through the region header: these
	 * constants must match the Rust `axivc` peer exactly. */
	BUILD_BUG_ON(sizeof(struct axivc_ring) != 8448);
	BUILD_BUG_ON(offsetof(struct axivc_ring, slots) != AXIVC_SLOT_SIZE);
}

void axivc_ring_initialize(struct axivc_ring *ring, u32 direction)
{
	axivc_ring_check_layout();
	WRITE_ONCE(ring->direction, direction);
	WRITE_ONCE(ring->capacity, AXIVC_RING_CAPACITY);
	WRITE_ONCE(ring->slot_size, AXIVC_SLOT_SIZE);
	WRITE_ONCE(ring->head, 0);
	memset(ring->slots, 0, sizeof(ring->slots));
	/* Publishing tail last lets a peer that observes an empty ring trust
	 * the layout fields above. */
	smp_store_release(&ring->tail, 0);
}

bool axivc_ring_try_push_slot(
	struct axivc_ring *ring, const u8 slot[AXIVC_SLOT_SIZE])
{
	u32 tail = READ_ONCE(ring->tail);
	u32 head = smp_load_acquire(&ring->head);
	u32 slot_index;

	if ((u32)(tail - head) >= AXIVC_RING_CAPACITY)
		return false;

	slot_index = tail % AXIVC_RING_CAPACITY;
	memcpy(ring->slots[slot_index], slot, AXIVC_SLOT_SIZE);
	smp_store_release(&ring->tail, tail + 1);
	return true;
}

bool axivc_ring_try_peek_slot(struct axivc_ring *ring, u8 slot[AXIVC_SLOT_SIZE])
{
	u32 head = READ_ONCE(ring->head);
	u32 tail = smp_load_acquire(&ring->tail);
	u32 slot_index;

	if (head == tail)
		return false;

	slot_index = head % AXIVC_RING_CAPACITY;
	memcpy(slot, ring->slots[slot_index], AXIVC_SLOT_SIZE);
	return true;
}

void axivc_ring_pop_slot(struct axivc_ring *ring)
{
	u32 head = READ_ONCE(ring->head);

	smp_store_release(&ring->head, head + 1);
}
