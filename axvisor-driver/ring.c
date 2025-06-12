#include <linux/io.h>
#include <linux/uaccess.h>

#include "includes/ring.h"
#include "includes/utils.h"

void shm_ring_init(void *shm_base, size_t shm_region_size, uint64_t channel_key)
{
	shm_ring_t *ring = (shm_ring_t *)shm_base;

	if (shm_region_size < sizeof(shm_ring_t) + RING_CAP * SLOT_SIZE)
	{
		ERROR(
			"%s: Shared memory region size is too small: %zu bytes, "
			"minimum required is %zu bytes\n",
			__func__, shm_region_size,
			sizeof(shm_ring_t) + RING_CAP * SLOT_SIZE);
		return;
	}

	INFO("%s: Channel publisher_id [%llx]\n", __func__, ring->publisher_id);

	if (ring->key != channel_key)
	{
		ERROR(
			"%s: Channel key mismatch: expected 0x%llx, got 0x%llx\n", __func__,
			channel_key, ring->key);
	}

	// DO NOT NEED TO Initialize the ring buffer structure, hypervisor has done
	// it.
	// memset(shm_base, 0, shm_region_size);

	ring->size = RING_CAP;
	ring->head = 0;
	ring->tail = 0;
	ring->slots_offset = sizeof(shm_ring_t);
	ring->data_offset = ring->slots_offset + RING_CAP * SLOT_SIZE;
	ring->data_size = shm_region_size - ring->data_offset;
	ring->alloc_offset = 0;
}

void *shm_ring_alloc_data(void *base, size_t len)
{
	void *ptr;
	shm_ring_t *ring = (shm_ring_t *)base;

	if (ring->alloc_offset + len > ring->data_size)
	{
		// Buffer overflow, no space left
		WARNING("%s: Not enough space in ring buffer\n", __func__);
		ring->alloc_offset = 0;
	}
	ptr = (uint8_t *)base + ring->data_offset + ring->alloc_offset;
	ring->alloc_offset += len;
	return ptr;
}

int shm_ring_enqueue(void *base, const char __user *data, size_t len)
{
	shm_ring_t *ring = (shm_ring_t *)base;
	shm_slot_t *slots = (shm_slot_t *)((uint8_t *)base + ring->slots_offset);
	size_t rel_offset;
	void *buf;

	size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
	size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
	size_t next = (tail + 1) & (ring->size - 1);
	if (next == head)
		return -ENOMEM;

	// Allocate data buffer.
	buf = shm_ring_alloc_data(base, len);
	if (!buf)
		return -ENOMEM;

	// Copy data to the allocated buffer.
	// memcpy(buf, data, len);
	if (copy_from_user(buf, data, len))
	{
		ERROR("axvisor: Failed to copy data from user space\n");
		return -EFAULT;
	}

	// Update the slot with the relative offset and length.
	rel_offset = (uint8_t *)buf - (uint8_t *)base;
	slots[tail].offset = rel_offset;
	slots[tail].length = len;

	atomic_store_explicit(&ring->tail, next, memory_order_release);
	return 0;
}

/// @brief Dequeue data from the shared memory ring buffer.
/// @param base     Pointer to the base of the shared memory region.
/// @param out_data     Pointer to store the dequeued data pointer.
/// @param out_len   Pointer to store the length of the dequeued data.
/// @param count    Optional parameter to specify the maximum number of bytes to
/// dequeue.
/// @return `true` if data was dequeued successfully, `false` if the ring is
/// empty.
bool shm_ring_dequeue(
	void *base, void **out_data, size_t *out_len, size_t count)
{
	shm_ring_t *ring = (shm_ring_t *)base;
	shm_slot_t *slots = (shm_slot_t *)((uint8_t *)base + ring->slots_offset);

	size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
	size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
	if (head == tail)
	{
		*out_len = 0;
		return false;
	}

	*out_data = (uint8_t *)base + slots[head].offset;

	if (count > 0 && count < slots[head].length)
	{
		// If count is specified and less than the slot length,
		// just dequeue the expected count of bytes.

		*out_len = count;
		// Update the slot offset and length to reflect the dequeued count.
		slots[head].offset += count;
		slots[head].length -= count;
	}
	else
	{
		// If count is not specified or greater than the slot length,
		// dequeue the entire slot.

		*out_len = slots[head].length;
		// Reset the slot to indicate it has been dequeued.
		slots[head].offset = 0; // Reset offset
		slots[head].length = 0; // Reset length

		atomic_store_explicit(
			&ring->head, (head + 1) & (ring->size - 1), memory_order_release);
	}

	return true;
}