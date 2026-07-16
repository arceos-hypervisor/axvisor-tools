#pragma once

#include <linux/types.h>
#include <linux/uaccess.h>

#define AXIVC_REGION_MAGIC 0x49564332U
#define AXIVC_REGION_VERSION 2U
#define AXIVC_REGION_FEATURE_SPSC_FIXED_SLOTS 1U
#define AXIVC_SLOT_PAYLOAD_SIZE 48U
#define AXIVC_RING_CAPACITY 16U
#define AXIVC_MESSAGE_KIND_REQUEST 1U
#define AXIVC_MESSAGE_KIND_ACK 2U

struct axivc_region_header {
	u32 magic;
	u32 version;
	u32 header_size;
	u32 region_size;
	u32 features;
	u32 publisher_to_subscriber_offset;
	u32 subscriber_to_publisher_offset;
	u32 ring_size;
} __aligned(8);

struct axivc_message_slot {
	u64 sequence;
	u32 len;
	u32 kind;
	u8 payload[AXIVC_SLOT_PAYLOAD_SIZE];
} __aligned(64);

struct axivc_ring {
	u32 direction;
	u32 capacity;
	u32 slot_payload_size;
	u32 head;
	u32 tail;
	u32 reserved[3];
	struct axivc_message_slot slots[AXIVC_RING_CAPACITY];
} __aligned(64);

struct axivc_region {
	u64 publisher_id;
	u64 key;
	struct axivc_region_header header;
	struct axivc_ring publisher_to_subscriber;
	struct axivc_ring subscriber_to_publisher;
} __aligned(64);

void shm_ring_init(
	void *shm_base, size_t shm_region_size, uint64_t channel_key);
size_t shm_ring_enqueue(void *base, const char __user *data, size_t len);
int shm_ring_dequeue(
	void *base, char __user *buf, size_t count, size_t *out_len);
int axivc_region_validate(
	void *base, size_t shm_region_size, u64 publisher_id, u64 channel_key);
ssize_t axivc_region_recv_request_and_ack(
	void *base, char __user *buf, size_t count, u64 *sequence);
