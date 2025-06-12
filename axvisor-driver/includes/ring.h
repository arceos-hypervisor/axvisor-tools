#ifndef _AXVISOR_RING_H_
#define _AXVISOR_RING_H_

#include <stdatomic.h>
#include <stddef.h>

#define RING_CAP 16					 // 必须是 2 的幂
#define SLOT_SIZE sizeof(shm_slot_t) // 每个槽放一个 offset（指向数据）

/**
 *
|---------------------------|
| struct shm_ring_t         |
|---------------------------|
| shm_slot_t* slot[]        |
|---------------------------|
| data buffer               |
|---------------------------|
 */
typedef struct
{
	uint64_t publisher_id; // channel publisher ID
	uint64_t key;		   // channel key
	size_t size;		   // 槽位数（必须为 2 的幂）
	atomic_size_t head __attribute__((aligned(64)));
	atomic_size_t tail __attribute__((aligned(64)));
	size_t slots_offset; // offset: 指向 void* slots[capacity]
	size_t data_offset;	 // offset: 指向 data buffer 区的起点
	size_t data_size;	 // 数据区大小（单位 byte）
	size_t alloc_offset; // bump 分配器：下一个可用数据区 offset
} shm_ring_t;

typedef struct
{
	size_t offset;
	size_t length;
} shm_slot_t;

void shm_ring_init(
	void *shm_base, size_t shm_region_size, uint64_t channel_key);
void *shm_ring_alloc_data(void *base, size_t len);
int shm_ring_enqueue(void *base, const char __user *data, size_t len);
bool shm_ring_dequeue(
	void *base, void **out_data, size_t *out_len, size_t count);

#endif // _AXVISOR_RING_H_