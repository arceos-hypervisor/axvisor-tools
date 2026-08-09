#include <ivc/ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char message[1024];
int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <channel_key>\n", argv[0]);
		return 1;
	}
	uint64_t channel_key = strtoull(argv[1], NULL, 0);

	int ret = 0;

	ivc_manager_p manager = ivc_open_manager();
	if (!manager)
	{
		fprintf(stderr, "Failed to open IVC manager\n");
		return 1;
	}

	ivc_publisher_p publisher = ivc_publish(manager, channel_key, 16 * 1024);
	if (!publisher)
	{
		fprintf(stderr, "Failed to publish channel\n");
		ret = 2;
		goto close_manager;
	}

	while (scanf("%1023s", message) == 1)
	{
		if (ivc_publisher_send(publisher, message, strlen(message)) < 0)
		{
			fprintf(stderr, "Failed to send message on publisher\n");
			ret = 3;
			break;
		}
		printf(
			"Sent: %s, total sent: %lu\n", message, publisher->bytes_sent);
	}

	if (ivc_unpublish(publisher) < 0)
	{
		fprintf(stderr, "Failed to unpublish publisher\n");
		ret = 4;
	}
close_manager:
	if (ivc_close_manager(manager) < 0)
	{
		fprintf(stderr, "Failed to close IVC manager\n");
		ret = 5;
	}
	printf("IVC publisher example finished.\n");
	return ret;
}
