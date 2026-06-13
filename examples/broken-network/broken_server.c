#include <time.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string.h>
#include <arpa/inet.h>

#define MSG_LEN 3

static int fd = -1;
static uint8_t rcvBuf[10];
static uint8_t sendBuf[10];

static int waitUSecsOrEvent(int usecs) {
	int ret;
	int nfds = 1;
	struct pollfd fds[1];
	struct timespec timeout = { .tv_sec = usecs / 1000000, .tv_nsec = (usecs % 1000000) * 1000};

	fds[0].fd = fd;
	fds[0].events = POLLIN;
	printf("Timeout = %lld.%09ld\n", timeout.tv_sec, timeout.tv_nsec);
	ret = ppoll(fds, nfds, &timeout, NULL);
	printf("ppoll() = %d\n", ret);

	return ret;
}

static int recvBytes(uint8_t *buf, int count) {
	int readCount = 0;

	int nfds = 1;
	struct pollfd fds[1];
	struct timespec timeout = {0,0};

	if (fd < 0) return 0;
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	int ret = ppoll(fds, nfds, &timeout, NULL);
	if (ret == -1) {
		perror("ppoll()");
	} else if (ret) {
		readCount = read(fd, buf, count);
		if (readCount < 0) {
			readCount = 0;
			perror("Error recvBytes: ");
		}
		else if (readCount == 0) {
			fd = -1;
		}
	}
	return readCount;
}

static int sendBytes(uint8_t *buf, int start, int end) {
	int writtenBytes = 0;

	int nfds = 1;
	struct pollfd fds[1];
	struct timespec timeout = {0,0};

	if (fd < 0) return 0;
	fds[0].fd = fd;
	fds[0].events = POLLOUT;
	int ret = ppoll(fds, nfds, &timeout, NULL);
	if (ret == -1) {
		perror("ppoll()");
	} else if (ret) {
		writtenBytes = write(fd, &buf[start], end - start);
		if (writtenBytes < 0) {
			writtenBytes = 0;
			perror("Error sendBytes no: ");
		}
		else if (writtenBytes == 0) {
			fd = -1;
		}
	}
	return writtenBytes;
}

static void setupConnection(void) {
	const int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	const int bool_true = 1;
	setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &bool_true, sizeof(bool_true));

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(9876);

	bind(tcp_socket, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	listen(tcp_socket, 1);
	fd = accept(tcp_socket, NULL, NULL);
}

static void processMessage(void) {
	int readd = 0;
	while (readd < MSG_LEN) {
		readd += recvBytes(&rcvBuf[readd], MSG_LEN - readd);
		usleep(100);
	}

	if (rcvBuf[0] == 0xfa && rcvBuf[1] == 0x1a && rcvBuf[2] == 0x00) {
		printf("Data in: %x%x%x\n", rcvBuf[0], rcvBuf[1], rcvBuf[2]);
		sendBuf[0] = 0xfa;
		sendBuf[1] = 0x1a;
		sendBuf[2] = 0x00;
		int bytesSent = 0;
		while (bytesSent < MSG_LEN) {
			bytesSent += sendBytes(sendBuf, bytesSent, MSG_LEN - bytesSent);
			usleep(100);
		}
	}
}

static void vmLoop(void) {
	int data_ready = 0;

	while (1) {
		int sleepUSecs = 2000000;
		if(waitUSecsOrEvent(sleepUSecs) > 0) {
			data_ready = 1;
		}
		if (data_ready) {
			processMessage();
			data_ready = 0;
		}
	}
}

int main(void) {
	setupConnection();
	vmLoop();

	return 1;
}
