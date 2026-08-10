#pragma once

#include "./ioctl_args.h"

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

	// TODO: record publishers and subscribers created and recycle them when
	// close the IVC manager.
	typedef struct ivc_manager
	{
		int64_t fd; // File descriptor for the IVC device
	} ivc_manager_t, *ivc_manager_p;

	ivc_manager_p ivc_open_manager(void);
	int ivc_close_manager(ivc_manager_p manager);

	typedef struct ivc_subscriber
	{
		ivc_manager_p manager;			   // Pointer to the IVC manager
		ivc_subscribe_arg_t subscribe_arg; // Subscription argument structure
		int64_t fd;				 // File descriptor for the subscriber's device
		uint64_t bytes_received; // Number of bytes received from the channel
		uint64_t bytes_sent;	 // Number of bytes sent to the channel
	} ivc_subscriber_t, *ivc_subscriber_p;

	ivc_subscriber_p ivc_subscribe(
		ivc_manager_p manager, uint64_t publisher_id, uint64_t channel_key);
	// Receives one complete logical message. count must be nonzero. Returns the
	// message length, 0 when no message is available, or a negative value on
	// error.
	int
	ivc_subscriber_recv(ivc_subscriber_p subscriber, void *buf, size_t count);
	// Sends one complete logical message. Empty messages are not supported by
	// the POSIX read/write device adapter.
	int ivc_subscriber_send(
		ivc_subscriber_p subscriber, const void *buf, size_t count);
	int ivc_unsubscribe(ivc_subscriber_p subscriber);

	typedef struct ivc_publisher
	{
		ivc_manager_p manager;		   // Pointer to the IVC manager
		ivc_publish_arg_t publish_arg; // Publish argument structure
		int64_t fd;				 // File descriptor for the publisher's device
		uint64_t bytes_received; // Number of bytes received from the channel
		uint64_t bytes_sent;	 // Number of bytes sent to the channel
	} ivc_publisher_t, *ivc_publisher_p;

	ivc_publisher_p ivc_publish(
		ivc_manager_p manager, uint64_t channel_key, uint64_t channel_size);
	// Receives one complete logical message. count must be nonzero. Returns the
	// message length, 0 when no message is available, or a negative value on
	// error.
	int ivc_publisher_recv(ivc_publisher_p publisher, void *buf, size_t count);
	// Sends one complete logical message. Empty messages are not supported by
	// the POSIX read/write device adapter.
	int ivc_publisher_send(
		ivc_publisher_p publisher, const void *buf, size_t count);
	int ivc_unpublish(ivc_publisher_p publisher);

#ifdef __cplusplus
}
#endif
