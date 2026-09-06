#include <asm/barrier.h>
#include <linux/build_bug.h>
#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/types.h>

#include "includes/region.h"

static void axivc_region_check_layout(void)
{
	/* Region layout is the shared-memory ABI with the Rust `axivc` peer:
	 * these constants must match it exactly, even though the region
	 * version stays 3. */
	BUILD_BUG_ON(sizeof(struct axivc_region_header) != 32);
	BUILD_BUG_ON(sizeof(struct axivc_ring) != 8448);
	BUILD_BUG_ON(offsetof(struct axivc_region, publisher_to_subscriber) != 256);
	BUILD_BUG_ON(
		offsetof(struct axivc_region, subscriber_to_publisher) != 8704);
	BUILD_BUG_ON(sizeof(struct axivc_region) != 17152);
}

int axivc_region_init_publisher(void *base, size_t shm_size, u64 channel_key)
{
	struct axivc_region *region = base;
	struct axivc_region_header *header;
	u64 publisher_id;
	u64 key;

	axivc_region_check_layout();
	if (shm_size < sizeof(struct axivc_region))
		return -EINVAL;

	header = &region->header;

	/* axvisor writes the channel identity when the host-side channel is
	 * created. Preserve it across the protocol reset; a mismatch means the
	 * region was not prepared by axvisor for this channel at all. */
	publisher_id = READ_ONCE(region->publisher_id);
	key = READ_ONCE(region->key);
	if (key != channel_key)
		return -EPROTO;

	/* A newly mapped region can contain bytes from an earlier session.
	 * Clear publication first so a racing subscriber cannot accept stale
	 * layout metadata while the rings are being reinitialized. */
	smp_store_release(&header->magic, 0);
	WRITE_ONCE(region->publisher_id, publisher_id);
	WRITE_ONCE(region->key, key);
	axivc_ring_initialize(
		&region->publisher_to_subscriber,
		AXIVC_RING_DIRECTION_PUBLISHER_TO_SUBSCRIBER);
	axivc_ring_initialize(
		&region->subscriber_to_publisher,
		AXIVC_RING_DIRECTION_SUBSCRIBER_TO_PUBLISHER);

	/* Publish the protocol header only after both rings are ready. A peer
	 * that observes magic with acquire may immediately use the rings. */
	WRITE_ONCE(header->header_size, sizeof(struct axivc_region_header));
	WRITE_ONCE(header->region_size, sizeof(struct axivc_region));
	WRITE_ONCE(header->features, AXIVC_REGION_FEATURE_SPSC_OPAQUE_SLOTS);
	WRITE_ONCE(
		header->publisher_to_subscriber_offset,
		offsetof(struct axivc_region, publisher_to_subscriber));
	WRITE_ONCE(
		header->subscriber_to_publisher_offset,
		offsetof(struct axivc_region, subscriber_to_publisher));
	WRITE_ONCE(header->ring_size, sizeof(struct axivc_ring));
	smp_store_release(&header->version, AXIVC_REGION_VERSION);
	smp_store_release(&header->magic, AXIVC_REGION_MAGIC);
	return 0;
}

int axivc_region_validate(
	void *base, size_t shm_size, u64 publisher_id, u64 channel_key)
{
	struct axivc_region *region = base;
	struct axivc_region_header *header = &region->header;

	axivc_region_check_layout();
	if (shm_size < sizeof(struct axivc_region))
		return -EINVAL;
	if (READ_ONCE(region->publisher_id) != publisher_id ||
		READ_ONCE(region->key) != channel_key)
		return -EINVAL;
	if (smp_load_acquire(&header->magic) != AXIVC_REGION_MAGIC)
		return -EAGAIN;

	/* v2 peers fail the version check; v3 peers using different slot
	 * parameters fail the strict layout checks below. Both cases reject
	 * explicitly instead of silently corrupting messages. */
	if (smp_load_acquire(&header->version) != AXIVC_REGION_VERSION)
		return -EPROTO;
	if (READ_ONCE(header->header_size) != sizeof(struct axivc_region_header))
		return -EPROTO;
	if (READ_ONCE(header->region_size) < sizeof(struct axivc_region))
		return -EPROTO;
	if ((READ_ONCE(header->features) &
		 AXIVC_REGION_FEATURE_SPSC_OPAQUE_SLOTS) !=
		AXIVC_REGION_FEATURE_SPSC_OPAQUE_SLOTS)
		return -EPROTO;
	if (READ_ONCE(header->publisher_to_subscriber_offset) !=
		offsetof(struct axivc_region, publisher_to_subscriber))
		return -EPROTO;
	if (READ_ONCE(header->subscriber_to_publisher_offset) !=
		offsetof(struct axivc_region, subscriber_to_publisher))
		return -EPROTO;
	if (READ_ONCE(header->ring_size) != sizeof(struct axivc_ring))
		return -EPROTO;
	if (READ_ONCE(region->publisher_to_subscriber.direction) !=
			AXIVC_RING_DIRECTION_PUBLISHER_TO_SUBSCRIBER ||
		READ_ONCE(region->publisher_to_subscriber.capacity) !=
			AXIVC_RING_CAPACITY ||
		READ_ONCE(region->publisher_to_subscriber.slot_size) != AXIVC_SLOT_SIZE)
		return -EPROTO;
	if (READ_ONCE(region->subscriber_to_publisher.direction) !=
			AXIVC_RING_DIRECTION_SUBSCRIBER_TO_PUBLISHER ||
		READ_ONCE(region->subscriber_to_publisher.capacity) !=
			AXIVC_RING_CAPACITY ||
		READ_ONCE(region->subscriber_to_publisher.slot_size) != AXIVC_SLOT_SIZE)
		return -EPROTO;
	return 0;
}
