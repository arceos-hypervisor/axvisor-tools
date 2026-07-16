#include <linux/build_bug.h>
#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "includes/ring.h"
#include "includes/utils.h"

static inline struct axivc_ring *axivc_publisher_to_subscriber_ring(void *base)
{
	return &((struct axivc_region *)base)->publisher_to_subscriber;
}

static inline struct axivc_ring *axivc_subscriber_to_publisher_ring(void *base)
{
	return &((struct axivc_region *)base)->subscriber_to_publisher;
}

static int axivc_ring_send(
	struct axivc_ring *ring, u32 kind, u64 sequence, const u8 *payload,
	size_t payload_len)
{
	struct axivc_message_slot *slot;
	u32 head;
	u32 tail;
	u32 slot_index;
	size_t len;

	tail = READ_ONCE(ring->tail);
	head = smp_load_acquire(&ring->head);
	if ((u32)(tail - head) >= AXIVC_RING_CAPACITY)
		return -EAGAIN;

	slot_index = tail % AXIVC_RING_CAPACITY;
	slot = &ring->slots[slot_index];
	len = min_t(size_t, payload_len, AXIVC_SLOT_PAYLOAD_SIZE);
	memcpy(slot->payload, payload, len);
	if (len < AXIVC_SLOT_PAYLOAD_SIZE)
		memset(slot->payload + len, 0, AXIVC_SLOT_PAYLOAD_SIZE - len);
	WRITE_ONCE(slot->sequence, sequence);
	WRITE_ONCE(slot->len, len);
	WRITE_ONCE(slot->kind, kind);
	smp_store_release(&ring->tail, tail + 1);
	return 0;
}

static int axivc_ring_recv(
	struct axivc_ring *ring, u32 expected_kind, char __user *buf, size_t count,
	u64 *sequence, size_t *out_len)
{
	struct axivc_message_slot *slot;
	u32 head;
	u32 tail;
	u32 slot_index;
	u32 raw_kind;
	size_t len;

	head = READ_ONCE(ring->head);
	tail = smp_load_acquire(&ring->tail);
	if (head == tail)
		return 0;

	slot_index = head % AXIVC_RING_CAPACITY;
	slot = &ring->slots[slot_index];
	raw_kind = READ_ONCE(slot->kind);
	if (raw_kind != expected_kind)
		return -EPROTO;

	len = min_t(size_t, READ_ONCE(slot->len), AXIVC_SLOT_PAYLOAD_SIZE);
	len = min_t(size_t, len, count);
	if (copy_to_user(buf, slot->payload, len))
		return -EFAULT;

	*sequence = READ_ONCE(slot->sequence);
	*out_len = len;
	smp_store_release(&ring->head, head + 1);
	return 1;
}

static int axivc_check_layout(void)
{
	BUILD_BUG_ON(sizeof(struct axivc_region_header) != 32);
	BUILD_BUG_ON(sizeof(struct axivc_message_slot) != 64);
	BUILD_BUG_ON(sizeof(struct axivc_ring) != 1088);
	BUILD_BUG_ON(offsetof(struct axivc_region, publisher_to_subscriber) != 64);
	BUILD_BUG_ON(offsetof(struct axivc_region, subscriber_to_publisher) != 1152);
	BUILD_BUG_ON(sizeof(struct axivc_region) != 2240);
	return 0;
}

int axivc_region_validate(
	void *base, size_t shm_region_size, u64 publisher_id, u64 channel_key)
{
	struct axivc_region *region = base;
	struct axivc_region_header *header = &region->header;

	axivc_check_layout();
	if (shm_region_size < sizeof(struct axivc_region))
		return -EINVAL;
	if (READ_ONCE(region->publisher_id) != publisher_id ||
		READ_ONCE(region->key) != channel_key)
		return -EINVAL;
	if (smp_load_acquire(&header->magic) != AXIVC_REGION_MAGIC)
		return -EAGAIN;
	if (smp_load_acquire(&header->version) != AXIVC_REGION_VERSION)
		return -EPROTO;
	if (READ_ONCE(header->region_size) < sizeof(struct axivc_region))
		return -EPROTO;
	if (!(READ_ONCE(header->features) & AXIVC_REGION_FEATURE_SPSC_FIXED_SLOTS))
		return -EPROTO;
	if (READ_ONCE(header->publisher_to_subscriber_offset) !=
		offsetof(struct axivc_region, publisher_to_subscriber))
		return -EPROTO;
	if (READ_ONCE(header->subscriber_to_publisher_offset) !=
		offsetof(struct axivc_region, subscriber_to_publisher))
		return -EPROTO;
	if (READ_ONCE(header->ring_size) != sizeof(struct axivc_ring))
		return -EPROTO;
	return 0;
}

ssize_t axivc_region_recv_request_and_ack(
	void *base, char __user *buf, size_t count, u64 *sequence)
{
	static const u8 ack[] = "ack from linux subscriber";
	struct axivc_ring *rx = axivc_publisher_to_subscriber_ring(base);
	struct axivc_ring *tx = axivc_subscriber_to_publisher_ring(base);
	size_t bytes_read = 0;
	int ret;

	ret = axivc_ring_recv(
		rx, AXIVC_MESSAGE_KIND_REQUEST, buf, count, sequence, &bytes_read);
	if (ret <= 0)
		return ret;

	ret = axivc_ring_send(
		tx, AXIVC_MESSAGE_KIND_ACK, *sequence, ack, sizeof(ack) - 1);
	if (ret)
		return ret;

	return bytes_read;
}

void shm_ring_init(void *shm_base, size_t shm_region_size, uint64_t channel_key)
{
	if (shm_region_size < sizeof(struct axivc_region))
	{
		ERROR(
			"%s: Shared memory region size is too small: %zu bytes, "
			"minimum required is %zu bytes\n",
			__func__, shm_region_size, sizeof(struct axivc_region));
		return;
	}

	if (axivc_region_validate(shm_base, shm_region_size, 0, channel_key))
		WARNING("%s: axivc v2 publisher-side init is not implemented\n", __func__);
}

size_t shm_ring_enqueue(void *base, const char __user *data, size_t len)
{
	u8 payload[AXIVC_SLOT_PAYLOAD_SIZE];
	size_t payload_len = min_t(size_t, len, AXIVC_SLOT_PAYLOAD_SIZE);
	int ret;

	if (copy_from_user(payload, data, payload_len))
		return -EFAULT;

	ret = axivc_ring_send(
		axivc_publisher_to_subscriber_ring(base), AXIVC_MESSAGE_KIND_REQUEST, 0,
		payload, payload_len);
	if (ret)
		return ret;
	return payload_len;
}

/// @brief Dequeue data from the shared memory ring buffer.
/// @param base     Pointer to the base of the shared memory region.
/// @param buf      User-space buffer to store the dequeued data.
/// @param count    Maximum number of bytes to dequeue. If `count` is 0, the
///                 entire slot will be dequeued.
/// @param out_len  Pointer to store the length of the dequeued data.
/// @return `true` if data was dequeued successfully, `false` if the ring is
/// empty.
int shm_ring_dequeue(
	void *base, char __user *buf, size_t count, size_t *out_len)
{
	u64 sequence = 0;
	ssize_t ret;

	ret = axivc_region_recv_request_and_ack(base, buf, count, &sequence);
	if (ret < 0)
		return ret;
	*out_len = ret;
	return 0;
}
