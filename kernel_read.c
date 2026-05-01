#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define KMSG_PATH "/dev/kmsg"

int main() {
	int fd = open(KMSG_PATH, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		perror("Failed to open /dev/kmsg (are you root?)");
		return 1;
	}

	// Seek to the end to see only NEW messages
	lseek(fd, 0, SEEK_END);

	char buf[8192];
	printf("Listening for kernel messages... (CTRL+C to stop)\n");

	while(1) {
		ssize_t ret = read(fd, buf, sizeof(buf) - 1);
		if (ret > 0) {
			buf[ret] = '\0';
			printf("%s", buf);
		} else if (ret < 0 && errno != EAGAIN) {
			perror("Read error");
			break;
		}
		usleep(100000); // Small sleep to be CPU friendly
	}

	close(fd);
	return 0;
}
