#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
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
		fprintf(stderr, "Usage: %s <channel_key> <channel_size>\n", argv[0]);
		return 1;
	}

	unsigned long long channel_key = strtoull(argv[1], NULL, 0);
	unsigned long long channel_size = strtoull(argv[2], NULL, 0);

	const char *device_path = "/dev/axivc";
	char buffer[128];
	char user_input[128];
	ssize_t bytes_read, bytes_write;
	int publisher_fd;

	int manager_fd = open(device_path, O_RDWR);
	if (manager_fd < 0)
	{
		perror("Failed to open device");
		return 1;
	}

	ivc_publish_arg_t publish_arg = {
		.channel_key = channel_key,
		.channel_size = channel_size,
	};

	if (ioctl(manager_fd, IVC_PUBLISH_CHANNEL, &publish_arg) < 0)
	{
		perror("Failed to publish channel");
		close(manager_fd);
		return 1;
	}

	printf(
		"Published channel with key: 0x%lx, size: 0x%lu\n",
		publish_arg.channel_key, publish_arg.channel_size);
	printf("Get Device name: %s\n", publish_arg.device_name);

	publisher_fd = open(publish_arg.device_name, O_RDWR);

	if (publisher_fd < 0)
	{
		perror("Failed to open publisher device");
		return 1;
	}

	printf("Opened publisher device: %s\n", publish_arg.device_name);

	// Enter a loop to receive user input and write to the publisher device
	signal(SIGINT, handle_signal);

	while (keep_running)
	{
		// Get the current timestamp
		time_t current_time = time(NULL);
		if (current_time == -1)
		{
			perror("Failed to get current time");
			break;
		}

		struct tm *local_time = localtime(&current_time);
		if (local_time == NULL)
		{
			perror("Failed to convert time to local time");
			break;
		}

		char timestamp[64];
		if (strftime(
				timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",
				local_time) == 0)
		{
			fprintf(stderr, "Failed to format timestamp\n");
			break;
		}

		// Prompt user for input
		printf("Enter message to publish (type 'exit' to quit): ");
		if (fgets(user_input, sizeof(user_input), stdin) == NULL)
		{
			perror("Failed to read user input");
			break;
		}

		// Remove newline character from user input
		user_input[strcspn(user_input, "\n")] = '\0';

		// Check for exit condition
		if (strcmp(user_input, "exit") == 0)
		{
			printf("Exiting...\n");
			break;
		}

		// Combine user input with timestamp
		char message[256];
		snprintf(message, sizeof(message), "%s at %s", user_input, timestamp);

		// Write to the publisher device
		bytes_write = write(publisher_fd, message, strlen(message));
		if (bytes_write < 0)
		{
			printf("Errno : %ld\n", bytes_write);
			perror("Failed to write to publisher device");
			break;
		}
		printf(
			"Wrote %zd bytes to publisher device: %s\n", bytes_write, message);
	}

	close(publisher_fd);
	printf("Closed publisher device: %s\n", publish_arg.device_name);

	if (ioctl(manager_fd, IVC_UNPUBLISH_CHANNEL, &publish_arg) < 0)
	{
		perror("Failed to unpublish channel");
		close(manager_fd);
		return 1;
	}

	close(manager_fd);

	return 0;
}