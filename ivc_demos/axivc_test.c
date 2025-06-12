#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <time.h>

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

int main()
{
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
		.channel_key = 0x12345678, // Example channel key
		.channel_size = 4096,	   // Example channel size
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

	// Example of writing to the publisher device
	snprintf(
		user_input, sizeof(user_input), "Hello from publisher at %ld\n",
		time(NULL));
	bytes_write = write(publisher_fd, user_input, strlen(user_input));
	if (bytes_write < 0)
	{
		perror("Failed to write to publisher device");
		close(publisher_fd);
		return 1;
	}
	printf(
		"Wrote %zd bytes to publisher device: %s\n", bytes_write, user_input);
	// Example of reading from the publisher device
	bytes_read = read(publisher_fd, buffer, sizeof(buffer) - 1);
	if (bytes_read < 0)
	{
		perror("Failed to read from publisher device");
		close(publisher_fd);
		return 1;
	}
	buffer[bytes_read] = '\0'; // Null-terminate the string
	printf("Read %zd bytes from publisher device: %s\n", bytes_read, buffer);
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