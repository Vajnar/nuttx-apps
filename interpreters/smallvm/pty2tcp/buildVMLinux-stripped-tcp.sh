#!/bin/sh
# Build TCP to pseudo-terminal communication with IDE MicroBlocks

gcc -std=c99 -Wall -O3 \
	pty2tcp.c \
	-o pty2tcp
