#pragma once

#include <linux/compiler.h>
#include <linux/types.h>

/* Opaque-slot SPSC ring shared with the Rust `axivc` crate. */
#define AXIVC_SLOT_SIZE 256U
#define AXIVC_RING_CAPACITY 32U

#define AXIVC_RING_DIRECTION_PUBLISHER_TO_SUBSCRIBER 1U
#define AXIVC_RING_DIRECTION_SUBSCRIBER_TO_PUBLISHER 2U

/*
 * Single-producer, single-consumer opaque-slot ring.
 *
 * The ring never interprets slot contents; only head/tail carry
 * synchronization. Producer and consumer synchronize through acquire/release
 * on head/tail exactly like the Rust peer:
 *
 * - producer reads head with acquire, writes the full slot, then publishes
 *   tail with release;
 * - consumer reads tail with acquire, copies the slot out, then releases
 *   head.
 *
 * slots start at offset AXIVC_SLOT_SIZE inside the ring so every slot stays
 * 256-byte aligned; with a page-aligned region base, a 4 KiB page fits
 * exactly 16 slots and no slot crosses a page boundary.
 * sizeof(struct axivc_ring) is 8448.
 *
 * The layout parameters (slot size, capacity, ring size and ring offsets)
 * are part of the compatibility contract with the Rust peer: both ends must
 * change them in lockstep, even when AXIVC_REGION_VERSION stays the same.
 */
struct axivc_ring
{
	u32 direction;
	u32 capacity;
	u32 slot_size;
	u32 head;
	u32 tail;
	u32 reserved[3];
	u8 slots[AXIVC_RING_CAPACITY][AXIVC_SLOT_SIZE] __aligned(AXIVC_SLOT_SIZE);
} __aligned(AXIVC_SLOT_SIZE);

/* Resets the ring to an empty v3 queue. Called before the region is
 * published to the peer, so plain stores are sufficient except for the
 * final tail release. */
void axivc_ring_initialize(struct axivc_ring *ring, u32 direction);

/* Returns false when the ring has no free slot; the slot is never
 * overwritten in that case. */
bool axivc_ring_try_push_slot(
	struct axivc_ring *ring, const u8 slot[AXIVC_SLOT_SIZE]);

/* Copies the oldest published slot without consuming it. Returns false when
 * the ring is empty. The consumer must call axivc_ring_pop_slot() only after
 * the peeked slot has been fully validated and copied. */
bool axivc_ring_try_peek_slot(struct axivc_ring *ring, u8 slot[AXIVC_SLOT_SIZE]);

/* Consumes one previously peeked slot. Must only be called after a
 * successful axivc_ring_try_peek_slot(). */
void axivc_ring_pop_slot(struct axivc_ring *ring);
