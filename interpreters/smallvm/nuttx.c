/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Copyright 2018 John Maloney, Bernat Romagosa, and Jens Mönig

// nuttx.c - Microblocks for NuttX

// John Maloney, December 2017
// Bernat Romagosa, February 2018
// Martin Vajnar, November 2025

#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> // still needed?
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h> // still needed?
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <ctype.h>
#include <nuttx/config.h>
#include <poll.h>
#include <nuttx/leds/userled.h>
#include <nuttx/input/buttons.h>


#include "mem.h"
#include "interp.h"
#include "persist.h"

// Timing Functions

static int startSecs = 0;

static void initTimers() {
	struct timeval now;
	gettimeofday(&now, NULL);
	startSecs = now.tv_sec;
}

uint32 microsecs() {
	struct timeval now;
	gettimeofday(&now, NULL);

	return (1000000 * (now.tv_sec - startSecs)) + now.tv_usec;
}

uint32 millisecs() {
	struct timeval now;
	gettimeofday(&now, NULL);

	return (1000 * (now.tv_sec - startSecs)) + (now.tv_usec / 1000);
}

uint64 totalMicrosecs() {
        // Returns a 64-bit integer containing microseconds since start.
	struct timeval now;
	gettimeofday(&now, NULL);
	return (1000000 * (now.tv_sec - startSecs)) + now.tv_usec;
}

void delay(int ms) {
	clock_t start = millisecs();
	while (millisecs() < start + ms);
}


// Communication/System Functions

#ifdef DEBUG
static void print_hex_dump(const unsigned char *buffer, size_t length) {
    for (size_t i = 0; i < length; i += 16) {
        // Print the hex values
        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                printf("%02X ", buffer[i + j]);
            } else {
                printf("   "); // Print spaces for missing bytes
            }
        }

        // Print the ASCII representation
        printf(" |");
        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                printf("%c", isprint(buffer[i + j]) ? buffer[i + j] : '.');
            }
        }
        printf("|\n");
    }
}
#endif

static int listen_fd = -1;
static int fd = -1; // pseudo terminal used for communication with the IDE

int serialConnected() {
	return fd > -1;
}

int waitUSecsOrEvent(int usecs) {
	int ret;
	int nfds = 2;
	struct pollfd fds[2];
	struct timespec timeout = { .tv_sec = usecs / 1000000, .tv_nsec = (usecs % 1000000) * 1000};

	memset(&fds, 0, sizeof(fds));
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	fds[1].fd = listen_fd;
	fds[1].events = POLLIN;
	if (bytesToOutput()) { fds[0].events |= POLLOUT; }
	printf("Timeout = %lld.%09ld\n", timeout.tv_sec, timeout.tv_nsec);
	ret = ppoll(fds, nfds, &timeout, NULL);
	printf("ppoll() = %d\n", ret);

	return ret;
}

int recvBytes(uint8 *buf, int count) {
	int readCount = 0;

	int nfds = 1;
	struct pollfd fds[1];
	struct timespec timeout = {0,0};

	if (fd < 0) {
		int ret = accept(listen_fd, NULL, NULL);
		if (ret < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				perror("Error on accept(), will retry: ");
				return 0;
			}
		} else if (ret > 0) {
			fd = ret;
		}
	}
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	int ret = ppoll(fds, nfds, &timeout, NULL);
	if (ret == -1) {
		perror("ppoll()");
	} else if (ret) {
		if ((fds[0].revents & POLLHUP) || (fds[0].revents & POLLERR)) {
			close(fd);
			fd = -1;
		} else if (fds[0].revents & POLLIN) {
			readCount = read(fd, buf, count);
			if (readCount < 0) {
				if (errno == EPIPE) {
					close(fd);
					fd = -1;
				}
				readCount = 0;
				perror("Error recvBytes: ");
			}
#ifdef CONFIG_INTERPRETERS_SMALLVM_TCP
			else if (readCount == 0) {
				close(fd);
				fd = -1;
			}
#endif
#ifdef DEBUG
			else if (readCount > 0) {
				printf("recvBytes: buf = %p, readCount = %d, count = %d\n", buf, readCount, count);
				print_hex_dump(buf, readCount);
			}
#endif
		}
	}
	return readCount;
}

int sendBytes(uint8 *buf, int start, int end) {
	int writtenBytes = 0;

	int nfds = 1;
	struct pollfd fds[1];
	struct timespec timeout = {0,0};

	if (fd < 0) {
		int ret = accept(listen_fd, NULL, NULL);
		if (ret < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				perror("Error on accept(), will retry: ");
				return 0;
			}
		} else if (ret > 0) {
			fd = ret;
		}
	}
	fds[0].fd = fd;
	fds[0].events = POLLOUT;
	int ret = ppoll(fds, nfds, &timeout, NULL);
	if (ret == -1) {
		perror("ppoll()");
	} else if (ret) {
		if ((fds[0].revents & POLLHUP) || (fds[0].revents & POLLERR)) {
			close(fd);
			fd = -1;
		} else if (fds[0].revents & POLLOUT) {
			writtenBytes = write(fd, &buf[start], end - start);
			if (writtenBytes < 0) {
				if (errno == EPIPE) {
					close(fd);
					fd = -1;
				}
				writtenBytes = 0;
				perror("Error sendBytes no: ");
			}
	#ifdef CONFIG_INTERPRETERS_SMALLVM_TCP
			else if (writtenBytes == 0) {
				close(fd);
				fd = -1;
			}
	#endif
	#ifdef DEBUG
			else if (writtenBytes > 0) {
				printf("sendBytes: &buf[start] = %p, start = %d, end = %d, writtenBytes = %d, to write() = %d\n", &buf[start], start, end, writtenBytes, end - start);
				print_hex_dump(&buf[start], writtenBytes);
			}
	#endif
		}
	}
	return writtenBytes;
}

int ideConnected() {
	return serialConnected();
}

// System Functions

const char * boardType() {
	return "NuttX";
}

static bool ledEnabled = false;
static int led_fd;

void primSetUserLED(OBJ *args) {
	int ret;
	if (!ledEnabled) {
		ret = open("/dev/led0", O_WRONLY);
		if (ret < 0) {
			perror("Failed to open /dev/led0: ");
			abort();
		}
		led_fd = ret;
		ledEnabled = true;
	}
	struct userled_s led;
	led.ul_led = 0;
	if (trueObj == args[0]) {
		led.ul_on = true;
	} else {
		led.ul_on = false;
	}
	ret = ioctl(led_fd, ULEDIOC_SETLED, &led);
	if (ret < 0) {
		perror("Failed to set LED state: ");
	}
}

static bool buttonEnable = false;
static int button_fd;

OBJ primButtonA(OBJ *args) {
	if (!buttonEnable) {
		int ret = open("/dev/buttons", O_RDONLY | O_NONBLOCK);
		if (ret < 0) {
			perror("ERROR: Failed to open /dev/buttons: ");
			abort();
		}
		button_fd = ret;
		buttonEnable = true;
	}
	btn_buttonset_t sample;
	int nbytes = read(button_fd, (void *)&sample, sizeof(btn_buttonset_t));
	if (nbytes > 0) {
		return (sample & 1) ? trueObj : falseObj;
	}
	return falseObj;
}

OBJ primButtonB(OBJ *args) {
	if (!buttonEnable) {
		int ret = open("/dev/buttons", O_RDONLY | O_NONBLOCK);
		if (ret < 0) {
			perror("ERROR: Failed to open /dev/buttons: ");
			abort();
		}
		button_fd = ret;
		buttonEnable = true;
	}
	btn_buttonset_t sample;
	int nbytes = read(button_fd, (void *)&sample, sizeof(btn_buttonset_t));
	if (nbytes > 0) {
		return (sample & 2) ? trueObj : falseObj;
	}
	return falseObj;
}

// Stubs

int useTFT = 0;

void turnOffInternalNeoPixels() { }
OBJ primMBDisplayOff(int argCount, OBJ *args) { return falseObj; }
void stopTone() { }
OBJ primI2cGet(OBJ *args) { return int2obj(0); }
OBJ primI2cSet(OBJ *args) { return int2obj(0); }
OBJ primSPISend(OBJ *args) { return int2obj(0); }
OBJ primSPIRecv(OBJ *args) { return int2obj(0); }
void updateMicrobitDisplay() { }
void resetRadio() { }
void BLE_setEnabled(int enableFlag) { }
void handleMicosecondClockWrap() { }

// Stubs for IO primitives

OBJ primAnalogPins(OBJ *args) { return int2obj(0); }
OBJ primDigitalPins(OBJ *args) { return int2obj(0); }
OBJ primAnalogRead(int argCount, OBJ *args) { return int2obj(0); }
void primAnalogWrite(OBJ *args) { }
OBJ primDigitalRead(int argCount, OBJ *args) { return int2obj(0); }
void primDigitalWrite(OBJ *args) { }
void primDigitalSet(int pinNum, int flag) { };

// Stubs for other functions not used on Linux

void processFileMessage(int msgType, int dataSize, char *data) {}
void resetServos() {}
void stopPWM() {}
void systemReset() {}
void turnOffPins() {}
void stopServos() {}

// Persistence support

char *codeFileName = "/w25/ublockscode";
FILE *codeFile;

int initCodeFile(uint8 *flash, int flashByteCount) {
	codeFile = fopen(codeFileName, "ab+");
	fseek(codeFile, 0 , SEEK_END);
	long fileSize = ftell(codeFile);

	// read code file into simulated Flash:
	fseek(codeFile, 0L, SEEK_SET);
	long bytesRead = fread((char*) flash, 1, flashByteCount, codeFile);
	if (bytesRead != fileSize) {
		outputString("initCodeFile did not read entire file");
	}
	return bytesRead;
}

void writeCodeFile(uint8 *code, int byteCount) {
	fwrite(code, 1, byteCount, codeFile);
	fflush(codeFile);
	sync();
	printf("Written %d bytes to persistent storage.\n", byteCount);
}

void writeCodeFileWord(int word) {
	fwrite(&word, 1, 4, codeFile);
	fflush(codeFile);
	sync();
	printf("Written %d bytes to persistent storage.\n", 4);
}

void clearCodeFile(int ignore) {
	fclose(codeFile);
	remove(codeFileName);
	codeFile = fopen(codeFileName, "ab+");
	uint32 cycleCount = ('S' << 24) | 1; // Header record, version 1
	fwrite((uint8 *) &cycleCount, 1, 4, codeFile);
	fflush(codeFile);
	sync();
	printf("Written %d bytes to persistent storage.\n", 4);
}
// Debug

static void exitGracefully() {
	close(fd);
}

void segfault() {
	printf("-- VM crashed --\n");
	exitGracefully();
}

#ifdef CONFIG_INTERPRETERS_SMALLVM_SERIAL
void setupConnection(void) {
	fd = open(CONFIG_INTERPRETERS_SMALLVM_SERIAL_DEVICE,
		O_NOCTTY | O_NONBLOCK | O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("setupConnection: open()");
		exit(-1);
	}

	struct termios settings;
	memset(&settings, 0, sizeof(settings));

	tcgetattr(fd, &settings);
	cfmakeraw(&settings);
	cfsetispeed(&settings, B115200);
	cfsetospeed(&settings, B115200);
	settings.c_cc[VMIN] = 0;
	settings.c_cc[VTIME] = 0;
	tcsetattr(fd, TCSANOW, &settings);
	tcflush(fd, TCIOFLUSH);
}
#endif

#ifdef CONFIG_INTERPRETERS_SMALLVM_TCP
void setupConnection(void) {
	listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	const int bool_true = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &bool_true, sizeof(bool_true));

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(CONFIG_INTERPRETERS_SMALLVM_TCP_PORT);

	bind(listen_fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
	listen(listen_fd, 1);
	int ret = accept(listen_fd, NULL, NULL);
	if (ret < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("Error on accept(), will retry: ");
		}
	} else if (ret > 0) {
		fd = ret;
	}
//	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &bool_true, sizeof(bool_true));
//	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &bool_true, sizeof(bool_true));
//	int flags = fcntl(fd, F_GETFL, 0);
//	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
#endif

// NuttX Main

int main(int argc, char *argv[]) {
	signal(SIGSEGV, segfault);
	signal(SIGINT, exit);
	signal(SIGPIPE, SIG_IGN);
	atexit(exitGracefully);
	setupConnection();
	printf("Starting NuttX MicroBlocks...\n");
	initTimers();
	memInit();
	primsInit();
	outputString("Welcome to uBlocks for NuttX!");
	restoreScripts();
	startAll();
	vmLoop();
	return 0;
}
