#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>

#define MSG_LEN 3
static int tcp_socket;

static void transferData() {
    unsigned char rcvBuf[10];
    unsigned char sendBuf[10];
    int recv = 0;
    int sent = 0;
    struct pollfd fds[1];
    struct timespec timeout;


    sendBuf[0] = 0xfa;
    sendBuf[1] = 0x1a;
    sendBuf[2] = 0x00;

    while (sent < MSG_LEN) {
        timeout.tv_sec = 30;
        timeout.tv_nsec = 0;
        memset(fds, 0, sizeof(fds));
        fds[0].fd = tcp_socket;
        fds[0].events = POLLOUT;
        int nfds = 1;

        int ret = ppoll(fds, nfds, &timeout, NULL);
        if (ret > 0) {
            if (fds[0].revents & POLLOUT) {
                int written = write(fds[0].fd, &sendBuf[sent], MSG_LEN - sent);
                if (written < 0) {
                    perror("write()");
                } else if (written) {
                    printf("%d = write(%d, %p, %d)\n", written, fds[0].fd, &sendBuf[sent], MSG_LEN - sent);
                    sent += written;
                }
            }
        }
    }

    while (recv < MSG_LEN) {
        timeout.tv_sec = 30;
        timeout.tv_nsec = 0;
        memset(fds, 0, sizeof(fds));
        fds[0].fd = tcp_socket;
        fds[0].events = POLLIN;
        int nfds = 1;

        int ret = ppoll(fds, nfds, &timeout, NULL);
        if (ret > 0) {
            if (fds[0].revents & POLLIN) {
                int readd = read(fds[0].fd, &rcvBuf[recv], MSG_LEN - recv);
                if (readd < 0) {
                    perror("read(): ");
                } else if (readd) {
                    printf("%d = read(%d, %p, %d)\n\n", readd, fds[0].fd, &rcvBuf[recv], MSG_LEN - recv);
                    recv += readd;
                }
            }
        }
    }

    printf("Received data: %x %x %x\n", rcvBuf[0], rcvBuf[1], rcvBuf[2]);
    usleep(2000000);
}

void dataLoop() {
    printf("TCP socket: %d\r\n", tcp_socket);
    while(1) {
        transferData();
    }
}

void setupTcpConnection(void) {
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr("10.0.0.1");
    saddr.sin_port = htons(9876);

    int ret = connect(tcp_socket, &saddr, sizeof(saddr));
    if (ret < 0) {
        perror(NULL);
        exit(-1);
    }
}

int main(void) {
    setupTcpConnection();
    dataLoop();

    return 0;
}

