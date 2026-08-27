#include <ivc/ulib.h>

#include <ivc/ioctl_args.h>
#include <ivc/ivc_dev.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ivc_manager_is_valid(ivc_manager_p manager)
{
	return manager && manager->fd >= 0;
}

static int
ivc_manager_release_endpoint(ivc_manager_p manager, const char *operation)
{
	if (!manager || manager->active_endpoints == 0)
	{
		errno = EINVAL;
		fprintf(
			stderr, "IVC manager endpoint count underflow during %s\n",
			operation);
		return -1;
	}
	manager->active_endpoints--;
	return 0;
}

ivc_manager_p ivc_open_manager(void)
{
	ivc_manager_p manager = malloc(sizeof(*manager));

	if (!manager)
	{
		perror("Failed to allocate memory for IVC manager");
		return NULL;
	}

	manager->fd = open(IVC_DEV_PATH, O_RDWR);
	manager->active_endpoints = 0;
	if (manager->fd < 0)
	{
		int open_errno = errno;

		perror("Failed to open IVC device");
		free(manager);
		errno = open_errno;
		return NULL;
	}
	return manager;
}

int ivc_close_manager(ivc_manager_p manager)
{
	int close_errno = 0;

	if (!ivc_manager_is_valid(manager))
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid IVC manager for close\n");
		return -1;
	}
	if (manager->active_endpoints != 0)
	{
		errno = EBUSY;
		fprintf(
			stderr, "Cannot close IVC manager with %llu active endpoint(s)\n",
			(unsigned long long)manager->active_endpoints);
		return -1;
	}

	/* Linux releases the descriptor before reporting a late close error, so
	 * the manager allocation is consumed on every non-busy close attempt. */
	if (close(manager->fd) < 0)
	{
		close_errno = errno;
		perror("Failed to close IVC device");
	}
	free(manager);
	if (close_errno)
	{
		errno = close_errno;
		return -1;
	}
	return 0;
}

ivc_subscriber_p ivc_subscribe(
	ivc_manager_p manager, uint64_t publisher_id, uint64_t channel_key)
{
	ivc_subscriber_p subscriber;

	if (!ivc_manager_is_valid(manager))
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid IVC manager for subscribe\n");
		return NULL;
	}

	subscriber = malloc(sizeof(*subscriber));
	if (!subscriber)
	{
		perror("Failed to allocate memory for subscriber");
		return NULL;
	}

	subscriber->manager = manager;
	subscriber->subscribe_arg.target_publisher_id = publisher_id;
	subscriber->subscribe_arg.channel_key = channel_key;
	memset(
		subscriber->subscribe_arg.device_name, 0,
		sizeof(subscriber->subscribe_arg.device_name));
	subscriber->bytes_received = 0;
	subscriber->bytes_sent = 0;

	if (ioctl(manager->fd, IVC_SUBSCRIBE_CHANNEL, &subscriber->subscribe_arg) <
		0)
	{
		int subscribe_errno = errno;

		perror("Failed to subscribe to channel");
		free(subscriber);
		errno = subscribe_errno;
		return NULL;
	}

	subscriber->fd = open(subscriber->subscribe_arg.device_name, O_RDWR);
	if (subscriber->fd < 0)
	{
		int open_errno = errno;

		perror("Failed to open subscriber device");
		if (ioctl(
				manager->fd, IVC_UNSUBSCRIBE_CHANNEL,
				&subscriber->subscribe_arg) < 0)
		{
			perror(
				"Failed to roll back subscription after device open failure");
		}
		free(subscriber);
		errno = open_errno;
		return NULL;
	}

	manager->active_endpoints++;
	return subscriber;
}

int ivc_subscriber_recv(ivc_subscriber_p subscriber, void *buf, size_t count)
{
	int bytes_received;

	if (!subscriber || !buf || count == 0)
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid arguments for ivc_subscriber_recv\n");
		return -1;
	}
	if (count > INT_MAX)
	{
		errno = EMSGSIZE;
		fprintf(stderr, "Subscriber receive buffer is too large\n");
		return -1;
	}

	bytes_received = (int)read(subscriber->fd, buf, count);
	if (bytes_received < 0)
	{
		perror("Failed to receive on subscriber device");
	}
	else
	{
		subscriber->bytes_received += (uint64_t)bytes_received;
	}
	return bytes_received;
}

int ivc_subscriber_send(
	ivc_subscriber_p subscriber, const void *buf, size_t count)
{
	int bytes_sent;

	if (!subscriber || !buf)
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid arguments for ivc_subscriber_send\n");
		return -1;
	}
	if (count == 0)
	{
		errno = EOPNOTSUPP;
		fprintf(
			stderr, "Empty IVC messages are not representable by the "
					"read/write device ABI\n");
		return -1;
	}
	if (count > INT_MAX)
	{
		errno = EMSGSIZE;
		fprintf(stderr, "Subscriber message is too large\n");
		return -1;
	}

	/* One device write is one Message V1 message. Retrying a short write
	 * would create another logical message instead of completing this one. */
	bytes_sent = (int)write(subscriber->fd, buf, count);
	if (bytes_sent < 0)
	{
		perror("Failed to send on subscriber device");
		return -1;
	}
	if (bytes_sent != (int)count)
	{
		errno = EIO;
		fprintf(
			stderr, "Subscriber device returned a short message send: %d/%zu\n",
			bytes_sent, count);
		return -1;
	}
	subscriber->bytes_sent += (uint64_t)bytes_sent;
	return bytes_sent;
}

int ivc_unsubscribe(ivc_subscriber_p subscriber)
{
	int saved_errno = 0;

	if (!subscriber)
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid subscriber for ivc_unsubscribe\n");
		return -1;
	}

	/* Consume the endpoint only after attempting every independent cleanup
	 * step. Preserve the first error because it is closest to the root cause.
	 */
	if (close(subscriber->fd) < 0)
	{
		saved_errno = errno;
		perror("Failed to close subscriber device");
	}
	if (ioctl(
			subscriber->manager->fd, IVC_UNSUBSCRIBE_CHANNEL,
			&subscriber->subscribe_arg) < 0)
	{
		if (!saved_errno)
		{
			saved_errno = errno;
		}
		perror("Failed to unsubscribe from channel");
	}
	if (ivc_manager_release_endpoint(subscriber->manager, "unsubscribe") < 0 &&
		!saved_errno)
	{
		saved_errno = errno;
	}

	free(subscriber);
	if (saved_errno)
	{
		errno = saved_errno;
		return -1;
	}
	return 0;
}

ivc_publisher_p
ivc_publish(ivc_manager_p manager, uint64_t channel_key, uint64_t channel_size)
{
	ivc_publisher_p publisher;

	if (!ivc_manager_is_valid(manager))
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid IVC manager for publish\n");
		return NULL;
	}

	publisher = malloc(sizeof(*publisher));
	if (!publisher)
	{
		perror("Failed to allocate memory for publisher");
		return NULL;
	}

	publisher->manager = manager;
	publisher->publish_arg.channel_key = channel_key;
	publisher->publish_arg.channel_size = channel_size;
	memset(
		publisher->publish_arg.device_name, 0,
		sizeof(publisher->publish_arg.device_name));
	publisher->bytes_received = 0;
	publisher->bytes_sent = 0;

	if (ioctl(manager->fd, IVC_PUBLISH_CHANNEL, &publisher->publish_arg) < 0)
	{
		int publish_errno = errno;

		perror("Failed to publish channel");
		free(publisher);
		errno = publish_errno;
		return NULL;
	}

	publisher->fd = open(publisher->publish_arg.device_name, O_RDWR);
	if (publisher->fd < 0)
	{
		int open_errno = errno;

		perror("Failed to open publisher device");
		if (ioctl(manager->fd, IVC_UNPUBLISH_CHANNEL, &publisher->publish_arg) <
			0)
		{
			perror("Failed to roll back publication after device open failure");
		}
		free(publisher);
		errno = open_errno;
		return NULL;
	}

	manager->active_endpoints++;
	return publisher;
}

int ivc_publisher_recv(ivc_publisher_p publisher, void *buf, size_t count)
{
	int bytes_received;

	if (!publisher || !buf || count == 0)
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid arguments for ivc_publisher_recv\n");
		return -1;
	}
	if (count > INT_MAX)
	{
		errno = EMSGSIZE;
		fprintf(stderr, "Publisher receive buffer is too large\n");
		return -1;
	}

	bytes_received = (int)read(publisher->fd, buf, count);
	if (bytes_received < 0)
	{
		perror("Failed to receive on publisher device");
	}
	else
	{
		publisher->bytes_received += (uint64_t)bytes_received;
	}
	return bytes_received;
}

int ivc_publisher_send(ivc_publisher_p publisher, const void *buf, size_t count)
{
	int bytes_sent;

	if (!publisher || !buf)
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid arguments for ivc_publisher_send\n");
		return -1;
	}
	if (count == 0)
	{
		errno = EOPNOTSUPP;
		fprintf(
			stderr, "Empty IVC messages are not representable by the "
					"read/write device ABI\n");
		return -1;
	}
	if (count > INT_MAX)
	{
		errno = EMSGSIZE;
		fprintf(stderr, "Publisher message is too large\n");
		return -1;
	}

	/* One device write is one Message V1 message. Retrying a short write
	 * would create another logical message instead of completing this one. */
	bytes_sent = (int)write(publisher->fd, buf, count);
	if (bytes_sent < 0)
	{
		perror("Failed to send on publisher device");
		return -1;
	}
	if (bytes_sent != (int)count)
	{
		errno = EIO;
		fprintf(
			stderr, "Publisher device returned a short message send: %d/%zu\n",
			bytes_sent, count);
		return -1;
	}
	publisher->bytes_sent += (uint64_t)bytes_sent;
	return bytes_sent;
}

int ivc_unpublish(ivc_publisher_p publisher)
{
	int saved_errno = 0;

	if (!publisher)
	{
		errno = EINVAL;
		fprintf(stderr, "Invalid publisher for ivc_unpublish\n");
		return -1;
	}

	/* Consume the endpoint only after attempting every independent cleanup
	 * step. Preserve the first error because it is closest to the root cause.
	 */
	if (close(publisher->fd) < 0)
	{
		saved_errno = errno;
		perror("Failed to close publisher device");
	}
	if (ioctl(
			publisher->manager->fd, IVC_UNPUBLISH_CHANNEL,
			&publisher->publish_arg) < 0)
	{
		if (!saved_errno)
		{
			saved_errno = errno;
		}
		perror("Failed to unpublish channel");
	}
	if (ivc_manager_release_endpoint(publisher->manager, "unpublish") < 0 &&
		!saved_errno)
	{
		saved_errno = errno;
	}

	free(publisher);
	if (saved_errno)
	{
		errno = saved_errno;
		return -1;
	}
	return 0;
}
