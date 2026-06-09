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
uint16_t color = 0;

static int tftEnabled = false;

// Helper Functions

#define COLOR_888_TO_565(color) (((((color) >> 19) & 0x1f) << 11) \
                                |((((color) >> 10) & 0x3f)) \
                                |(((color) >> 3) & 0x1f) << 6)

void setRenderColor(uint32_t colorA) {
	color = COLOR_888_TO_565(colorA);
}

void tftClear() {
	tftInit();
	setRenderColor(0);
	// TODO: clear screen
}

void tftInit() {
	if (!tftEnabled) {
		fb_fd = open("/dev/fb0", O_RDWR);
		ioctl(fb_fd, FBIOGET_PLANEINFO, &pinfo);
		ioctl(fb_fd, FBIOGET_VIDEOINFO, &vinfo);
		fb_mem = mmap(NULL,
						pinfo.fblen,
						PROT_READ | PROT_WRITE,
						MAP_SHARED | MAP_FILE, fb_fd, 0);
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

	struct fb_area_s area;
	area.x = x;
	area.y = y;
	area.w = 1;
	area.h = 1;

	ioctl(fb_fd, FBIO_UPDATE, &area);
	return falseObj;
}

static OBJ primLine(int argCount, OBJ *args) {
	tftInit();
	int x0 = obj2int(args[0]);
	int y0 = obj2int(args[1]);
	int x1 = obj2int(args[2]);
	int y1 = obj2int(args[3]);
	setRenderColor(obj2int(args[4]));

	if (x1 < x0)
	{
		int tmp = x0;
		x0 = x1;
		x1 = tmp;

		tmp = y0;
		y0 = y1;
		y1 = tmp;
	}
	uint16_t *dst = (uint16_t *) fb_mem;

	int dx = x1 - x0;
	int dy = y1 - y0;
	int m = dy/dx;
	for (int x = x0; x <= x1; x++)
	{
		int y = m * (x - x0) + y0;
		dst[y * vinfo.xres + x] = color;
	}
	struct fb_area_s area;
	area.x = x0;
	area.y = y1 > y0 ? y0 : y1;
	area.w = x1 - x0;
	area.h = abs(y1 - y0);

	ioctl(fb_fd, FBIO_UPDATE, &area);
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
		if ((x == X) || (x == X + width - 1))
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
	struct fb_area_s area;
	area.x = X;
	area.y = Y;
	area.w = width;
	area.h = height;

	ioctl(fb_fd, FBIO_UPDATE, &area);
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
	struct fb_area_s area;
	area.x = originX - radius;
	area.y = originY - radius;
	area.w = 2 * radius + 1;
	area.h = 2 * radius + 1;

	ioctl(fb_fd, FBIO_UPDATE, &area);
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

	struct fb_area_s area;
	area.x = 0;
	area.y = 0;
	area.w = vinfo.xres;
	area.h = vinfo.yres;

	ioctl(fb_fd, FBIO_UPDATE, &area);
	return falseObj;
}

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
};

void addTFTPrims() {
	addPrimitiveSet(TFTPrims, "tft", sizeof(entries) / sizeof(PrimEntry), entries);
}
