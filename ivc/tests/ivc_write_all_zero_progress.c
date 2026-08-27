#include <ivc/ulib.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static int write_calls;

ssize_t __wrap_write(int fd, const void *buf, size_t count)
{
	(void)fd;
	(void)buf;
	(void)count;

	write_calls++;
	return 0;
}

int main(void)
{
	ivc_publisher_t publisher = {
		.fd = 42,
		.bytes_sent = 0,
	};
	const char payload[] = "abc";
	int ret = ivc_publisher_send(&publisher, payload, sizeof(payload) - 1);

	if (ret != -1)
	{
		fprintf(stderr, "expected ivc_publisher_send to fail, got %d\n", ret);
		return 1;
	}
	if (write_calls != 1)
	{
		fprintf(stderr, "expected one write call, got %d\n", write_calls);
		return 1;
	}
	if (publisher.bytes_sent != 0)
	{
		fprintf(
			stderr, "expected zero recorded bytes, got %" PRIu64 "\n",
			publisher.bytes_sent);
		return 1;
	}

	return 0;
}
