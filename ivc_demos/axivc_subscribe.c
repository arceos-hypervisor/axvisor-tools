#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

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

volatile bool keep_running = true;

void handle_signal(int sig)
{
	if (sig == SIGINT)
	{
		keep_running = false;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		fprintf(
			stderr, "Usage: %s <target_publisher_id> <channel_key>\n", argv[0]);
		return 1;
	}

	unsigned long long target_publisher_id = strtoull(argv[1], NULL, 0);
	unsigned long long channel_key = strtoull(argv[2], NULL, 0);

	const char *device_path = "/dev/axivc";
	char buffer[4096];
	ssize_t bytes_read;
	int subscriber_fd, manager_fd;

	manager_fd = open(device_path, O_RDWR);
	if (manager_fd < 0)
	{
		perror("Failed to open device");
		return 1;
	}

	struct ivc_subscribe_arg subscribe_arg = {
		.target_publisher_id = target_publisher_id, .channel_key = channel_key};

	if (ioctl(manager_fd, IVC_SUBSCRIBE_CHANNEL, &subscribe_arg) < 0)
	{
		perror("Failed to subscribe to channel");
		close(manager_fd);
		return 1;
	}

	printf(
		"Subscribed to channel with publisher ID: %lu, key: 0x%lx\n",
		subscribe_arg.target_publisher_id, subscribe_arg.channel_key);
	printf("Get Device name: %s\n", subscribe_arg.device_name);
	subscriber_fd = open(subscribe_arg.device_name, O_RDONLY);

	if (subscriber_fd < 0)
	{
		perror("Failed to open subscriber device");
		close(manager_fd);
		return 1;
	}

	printf("Opened subscriber device: %s\n", subscribe_arg.device_name);

	signal(SIGINT, handle_signal);

	while (keep_running)
	{
		int bytes_to_read;
		printf("Enter the number of bytes to read: ");
		if (scanf("%d", &bytes_to_read) != 1 || bytes_to_read <= 0)
		{
			fprintf(
				stderr, "Invalid input. Please enter a positive integer.\n");
			break;
		}

		bytes_read = read(subscriber_fd, buffer, bytes_to_read);
		if (bytes_read < 0)
		{
			if (errno == EINTR) // Check if interrupted by a signal
			{
				printf("Read operation interrupted, exiting...\n");
				break;
			}
			perror("Failed to read from device");
			break;
		}
		else if (bytes_read == 0)
		{
			printf("Publisher's shared memory is empty, waiting...\n");
		}
		else
		{
			buffer[bytes_read] = '\0';
			printf("Read %zd bytes from device: [%s]\n", bytes_read, buffer);
		}
	}
	close(subscriber_fd);

	printf("Unsubscribing from channel...\n");
	if (ioctl(manager_fd, IVC_UNSUBSCRIBE_CHANNEL, &subscribe_arg) < 0)
	{
		perror("Failed to unsubscribe from channel");

		close(manager_fd);
		return 1;
	}

	close(manager_fd);
	return 0;
}