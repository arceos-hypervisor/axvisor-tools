#ifndef __IVC_H__
#define __IVC_H__

#include <stddef.h>

#define MAX_VDEVS 16

#define IVC_PUBLISHER_DEV_NAME_PREFIX "axivc_publisher_"
#define IVC_SUBSCRIBER_DEV_NAME_PREFIX "axivc_subscriber_"

#define IVC_DEV_NAME "axivc"

int init_ivc_devices(void);
void uninit_ivc_devices(void);

struct ivc_shm_header
{
	u64 publisher_id;
	u64 key;
	u64 content_size;
};

typedef struct ivc_publish_arg
{
	uint64_t channel_key;
	uint64_t channel_size;
	char device_name[64];
} ivc_publish_arg_t;

typedef struct ivc_subscribe_arg
{
	uint64_t target_publisher_id;
	uint64_t channel_key;
	char device_name[64];
} ivc_subscribe_arg_t;

#define IVC_PUBLISH_CHANNEL _IOW(0, 0, ivc_publish_arg_t)
#define IVC_UNPUBLISH_CHANNEL _IOW(0, 1, ivc_publish_arg_t)
#define IVC_SUBSCRIBE_CHANNEL _IOW(0, 2, ivc_subscribe_arg_t)
#define IVC_UNSUBSCRIBE_CHANNEL _IOW(0, 3, ivc_subscribe_arg_t)

#endif // __IVC_H__