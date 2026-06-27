/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Copyright 2020 John Maloney, Bernat Romagosa, and Jens Mönig

// linuxTftPrims.cpp - Microblocks TFT screen primitives simulated on an SDL window
// Bernat Romagosa, February 2021

#include <stdio.h>
#include <stdlib.h>
#include <nuttx/config.h>
#include <stdio.h>
#include <nuttx/video/fb.h>
#include <nuttx/input/touchscreen.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <math.h>

#include "mem.h"
#include "interp.h"

static int fb_fd;
static struct fb_planeinfo_s pinfo;
static struct fb_videoinfo_s vinfo;
static uint8_t *fb_mem;
static uint16_t color = 0;

static bool tftEnabled = false;

// Helper Functions

#define COLOR_888_TO_565(color) (((((color) >> 19) & 0x1f) << 11) \
                                |((((color) >> 10) & 0x3f) << 5)  \
                                |(((color) >> 3) & 0x1f))

static void setRenderColor(uint32_t colorA) {
	color = COLOR_888_TO_565(colorA);
}

void tftClear() {
	tftInit();
	setRenderColor(0);
	// TODO: clear screen
}

void tftInit() {
	if (!tftEnabled) {
		int ret = open("/dev/fb0", O_RDWR);
		if (ret < 0) {
			perror("Cannot open /dev/fb0: ");
			abort();
		}
		fb_fd = ret;
		ret = ioctl(fb_fd, FBIOGET_PLANEINFO, &pinfo);
		if (ret < 0) {
			perror("Cannot get plane info: ");
			close(fb_fd);
			abort();
		}
		if (pinfo.bpp != 16) {
			printf("Bits per pixel != 16\n");
			close(fb_fd);
			abort();
		}
		ret = ioctl(fb_fd, FBIOGET_VIDEOINFO, &vinfo);
		if (ret < 0) {
			perror("Cannot get video info: ");
			close(fb_fd);
			abort();
		}
		fb_mem = mmap(NULL,
						pinfo.fblen,
						PROT_READ | PROT_WRITE,
						MAP_SHARED | MAP_FILE, fb_fd, 0);
		if (fb_mem == MAP_FAILED) {
			perror("Cannot map framebuffer: ");
			close(fb_fd);
			abort();
		}
		tftEnabled = true;
	}
}

// TFT Primitives

static OBJ primEnableDisplay(int argCount, OBJ *args) {
	if (trueObj == args[0]) {
		tftInit();
	} else {
		//TODO: paint screen black to clear it
		//tftEnabled = false;
	}
	return falseObj;
}

static OBJ primGetWidth(int argCount, OBJ *args) {
	int w;
	tftInit();
	w = vinfo.xres;
	return int2obj(w);
}

static OBJ primGetHeight(int argCount, OBJ *args) {
	int h;
	tftInit();
	h = vinfo.yres;
	return int2obj(h);
}

static OBJ primSetPixel(int argCount, OBJ *args) {
	tftInit();
	int x = obj2int(args[0]);
	int y = obj2int(args[1]);
	setRenderColor(obj2int(args[2]));

	uint16_t *dst = (uint16_t*) fb_mem;
	dst[y * vinfo.xres + x] = color;

#ifdef CONFIG_FB_UPDATE
	struct fb_area_s area;
	area.x = x;
	area.y = y;
	area.w = 1;
	area.h = 1;

	ioctl(fb_fd, FBIO_UPDATE, &area);
#endif
	return falseObj;
}

static OBJ primLine(int argCount, OBJ *args) {
	tftInit();
	int x0 = obj2int(args[0]);
	int y0 = obj2int(args[1]);
	int x1 = obj2int(args[2]);
	int y1 = obj2int(args[3]);
	setRenderColor(obj2int(args[4]));

	uint16_t *dst = (uint16_t *) fb_mem;

	double dx = x1 - x0;
	double dy = y1 - y0;
	double m = dy/dx;
	if (x0 != x1) {
		int x0_s = x0;
		int x1_s = x1;
		if (x1_s < x0_s)
		{
			int tmp = x0_s;
			x0_s = x1_s;
			x1_s = tmp;
		}
		for (int x = x0_s; x <= x1_s; x++)
		{
			int y = (int)(m * ((double)x - (double)x0)) + y0;
			dst[y * vinfo.xres + x] = color;
		}
	} else {
		int y0_s = y0;
		int y1_s = y1;
		if (y1_s < y0_s)
		{
			int tmp = y0_s;
			y0_s = y1_s;
			y1_s = tmp;
		}
		for (int y = y0_s; y <= y1_s; y++)
		{
			dst[y * vinfo.xres + x0] = color;
		}
	}
#ifdef CONFIG_FB_UPDATE
	struct fb_area_s area;
	area.x = x0_s;
	area.y = y1 > y0 ? y0 : y1;
	area.w = x1_s - x0_s;
	area.h = abs(y1 - y0);

	ioctl(fb_fd, FBIO_UPDATE, &area);
#endif
	return falseObj;
}

static OBJ primRect(int argCount, OBJ *args) {
	tftInit();
	int X = obj2int(args[0]);
	int Y = obj2int(args[1]);
	int width = obj2int(args[2]);
	int height = obj2int(args[3]);
	int fill = (argCount > 5) ? (trueObj == args[5]) : true;
	setRenderColor(obj2int(args[4]));
	uint16_t *dst = (uint16_t *) fb_mem;

	for (int x = X; x < X + width; x++)
	{
		if ((x == X) || (x == X + width - 1) || fill)
		{
			for (int y = Y; y < Y + height; y++)
			{
				dst[y * vinfo.xres + x] = color;
			}
		}
		else
		{
			int y = Y;
			dst[y * vinfo.xres + x] = color;
			y = Y + height - 1;
			dst[y * vinfo.xres + x] = color;
		}
	}
#ifdef CONFIG_FB_UPDATE
	struct fb_area_s area;
	area.x = X;
	area.y = Y;
	area.w = width;
	area.h = height;

	ioctl(fb_fd, FBIO_UPDATE, &area);
#endif
	return falseObj;
}

static OBJ primCircle(int argCount, OBJ *args) {
	tftInit();
	int originX = obj2int(args[0]);
	int originY = obj2int(args[1]);
	int radius = obj2int(args[2]);
	setRenderColor(obj2int(args[3]));
	int fill = (argCount > 4) ? (trueObj == args[4]) : true;
	uint16_t *dst = (uint16_t *) fb_mem;

	for (int x = originX - radius; x <= originX + radius; x++)
	{
		int y = originY + sqrt(radius * radius - (x - originX) * (x - originX));
		dst[y * vinfo.xres + x] = color;
		y = originY - sqrt(radius * radius - (x - originX) * (x - originX));
		dst[y * vinfo.xres + x] = color;
		if (fill)
		{
			for (int y_tmp = originY - sqrt(radius * radius - (x - originX) * (x - originX)); \
				y_tmp < originY + sqrt(radius * radius - (x - originX) * (x - originX)); \
				y_tmp++)
				{
					dst[y_tmp * vinfo.xres + x] = color;
				}
		}
	}
#ifdef CONFIG_FB_UPDATE
	struct fb_area_s area;
	area.x = originX - radius;
	area.y = originY - radius;
	area.w = 2 * radius + 1;
	area.h = 2 * radius + 1;

	ioctl(fb_fd, FBIO_UPDATE, &area);
#endif
	return falseObj;
}

static OBJ primClear(int argCount, OBJ *args) {
	tftInit();
	uint16_t *dst = (uint16_t *) fb_mem;

	for (int x = 0; x < vinfo.xres; x++) {
		for (int y = 0; y < vinfo.yres; y++) {
			dst[y * vinfo.xres + x] = 0;
		}
	}

#ifdef CONFIG_FB_UPDATE
	struct fb_area_s area;
	area.x = 0;
	area.y = 0;
	area.w = vinfo.xres;
	area.h = vinfo.yres;

	ioctl(fb_fd, FBIO_UPDATE, &area);
#endif
	return falseObj;
}

static bool touchEnabled = false;
static int touch_fd;
static int touchX;
static int touchY;
static bool touchDown = false;

static void initTouch(void) {
	if (!touchEnabled) {
		int ret = open("/dev/input0", O_RDONLY | O_NONBLOCK);
		if (ret < 0) {
			perror("Failed to open /dev/input0: ");
			abort();
		}
		touch_fd = ret;
		touchEnabled = true;
	}
}

static void getTouchSample(void) {
	struct touch_sample_s sample;

	int ret = read(touch_fd, &sample, sizeof(struct touch_sample_s));
	if (ret < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
		perror("Failed to read() touch sample: ");
		abort();
	} else if (ret > 0) {
		touchX = sample.point[0].x;
		touchY = sample.point[0].y;
		if ((sample.point[0].flags & TOUCH_DOWN) || (sample.point[0].flags & TOUCH_MOVE)) {
			touchDown = true;
		} else {
			touchDown = false;
		}
	}
}

static OBJ primTftTouched(int argCount, OBJ *args) {
	initTouch();
	getTouchSample();
	return touchDown ? trueObj : falseObj;
}

static OBJ primTftTouchX(int argCount, OBJ *args) {
//	initTouch();
//	getTouchSample();
	return int2obj(touchDown ? touchX : -1);
}

static OBJ primTftTouchY(int argCount, OBJ *args) {
//	initTouch();
//	getTouchSample();
	return int2obj(touchDown ? touchY : -1);}


// Primitives

static PrimEntry entries[] = {
	{"enableDisplay", primEnableDisplay},
	{"getWidth", primGetWidth},
	{"getHeight", primGetHeight},
	{"setPixel", primSetPixel},
	{"line", primLine},
	{"rect", primRect},
	{"circle", primCircle},
	{"clear", primClear},
	{"tftTouched", primTftTouched},
	{"tftTouchX", primTftTouchX},
	{"tftTouchY", primTftTouchY},
};

void addTFTPrims() {
	addPrimitiveSet(TFTPrims, "tft", sizeof(entries) / sizeof(PrimEntry), entries);
}
