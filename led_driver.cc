//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2014 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

extern "C" {

#include <fcntl.h>
#include <getopt.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <zlib.h>

#include "bcm_host.h"

static const char* device = "/dev/spidev0.0";
static uint8_t mode = 0;
static uint8_t bits = 8;
static uint32_t speed = 15600000;
static uint16_t delay = 0;
static const VC_IMAGE_TYPE_T imageType = VC_IMAGE_RGB888;
static const int8_t imageBufferBytesPerPixel = 3;

static void pabort(const char* s) {
    perror(s);
    abort();
}

static void init_spi(int* fd_) {
    int ret = 0;
    int fd;

    fd = open(device, O_RDWR);
    *fd_ = fd;
    if (fd < 0) pabort("can't open device");

    /*
     * spi mode
     */
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1) pabort("can't set spi mode");

    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (ret == -1) pabort("can't get spi mode");

    /*
     * bits per word
     */
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) pabort("can't set bits per word");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1) pabort("can't get bits per word");

    /*
     * max speed hz
     */
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1) pabort("can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1) pabort("can't get max speed hz");

    printf("spi mode: %d\n", mode);
    printf("bits per word: %d\n", bits);
    printf("max speed: %d Hz (%d KHz)\n", speed, speed / 1000);
}

static void transfer(int fd, const uint8_t* data, size_t length) {
    int ret;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)data,
        .rx_buf = 0,
        .len = length,
        .delay_usecs = delay,
        .speed_hz = speed,
        .bits_per_word = bits,
    };

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) pabort("can't send spi message");
}

static void transfer_start(int fd, const uint8_t* data, size_t length) {
    uint8_t* tx_data = reinterpret_cast<uint8_t*>(malloc(length + 2));
    tx_data[0] = 0x80;
    tx_data[1] = 0;
    memcpy(tx_data + 2, data, length);
    transfer(fd, tx_data, length + 2);
    free(tx_data);
}

//-----------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x) ((x + 15) & ~15)
#endif

//-----------------------------------------------------------------------

#define DEFAULT_DELAY 0
#define DEFAULT_DISPLAY_NUMBER 0
#define DEFAULT_NAME "snapshot.png"

//-----------------------------------------------------------------------

const char* program = NULL;

//-----------------------------------------------------------------------

void usage(void) {
    fprintf(stderr, "Usage: %s [--pngname name]", program);
    fprintf(stderr, " [--width <width>] [--height <height>]");
    fprintf(stderr, " [--compression <level>]");
    fprintf(stderr, " [--delay <delay>] [--display <number>]");
    fprintf(stderr, " [--stdout] [--help]\n");

    fprintf(stderr, "\n");

    fprintf(stderr, "    --pngname,-p - name of png file to create ");
    fprintf(stderr, "(default is %s)\n", DEFAULT_NAME);

    fprintf(stderr, "    --height,-h - image height ");
    fprintf(stderr, "(default is screen height)\n");

    fprintf(stderr, "    --width,-w - image width ");
    fprintf(stderr, "(default is screen width)\n");

    fprintf(stderr, "    --compression,-c - PNG compression level ");
    fprintf(stderr, "(0 - 9)\n");

    fprintf(stderr, "    --delay,-d - delay in seconds ");
    fprintf(stderr, "(default %d)\n", DEFAULT_DELAY);

    fprintf(stderr, "    --display,-D - Raspberry Pi display number ");
    fprintf(stderr, "(default %d)\n", DEFAULT_DISPLAY_NUMBER);

    fprintf(stderr, "    --stdout,-s - write file to stdout\n");

    fprintf(stderr, "    --help,-H - print this usage information\n");

    fprintf(stderr, "\n");
}

//-----------------------------------------------------------------------

void transpose_rg_pixel(uint8_t* pixel) {
    uint8_t temp = *pixel;
    *pixel = *(pixel + 1);
    *(pixel + 1) = temp;
}
void transpose_rg(uint8_t* data, size_t num_pixels) {
    while (num_pixels--) {
        transpose_rg_pixel(data);
        data += 3;
    }
}

int main(int argc, char* argv[]) {
    uint32_t displayNumber = DEFAULT_DISPLAY_NUMBER;

    //-------------------------------------------------------------------

    bcm_host_init();

    //-------------------------------------------------------------------
    //
    // When the display is rotate (either 90 or 270 degrees) we need to
    // swap the width and height of the snapshot
    //

    char response[1024];
    int displayRotated = 0;

    if (vc_gencmd(response, sizeof(response), "get_config int") == 0) {
        vc_gencmd_number_property(response, "display_rotate", &displayRotated);
    }

    DISPMANX_DISPLAY_HANDLE_T displayHandle =
        vc_dispmanx_display_open(displayNumber);

    if (displayHandle == 0) {
        fprintf(stderr, "%s: unable to open display %d\n", program,
                displayNumber);

        exit(EXIT_FAILURE);
    }

    DISPMANX_MODEINFO_T modeInfo;
    int32_t result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

    if (result != 0) {
        fprintf(stderr, "%s: unable to get display information\n", program);
        exit(EXIT_FAILURE);
    }

    int32_t displayWidth = modeInfo.width;
    int32_t displayHeight = modeInfo.height;

    int32_t captureWidth = 200;
    int32_t captureHeight = 200;

    printf("Display dimensions: %d x %d\n", displayWidth, displayHeight);

    int32_t capturePitch = imageBufferBytesPerPixel * ALIGN_TO_16(displayWidth);

    uint8_t* captureBuffer =
        reinterpret_cast<uint8_t*>(malloc(capturePitch * captureHeight));

    if (captureBuffer == NULL) {
        fprintf(stderr, "%s: unable to allocate image buffer\n", program);
        exit(EXIT_FAILURE);
    }

    //-------------------------------------------------------------------

    uint32_t vcImagePtr = 0;
    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    resourceHandle = vc_dispmanx_resource_create(imageType, displayWidth,
                                                 displayHeight, &vcImagePtr);

    int fd;

    printf("Initializing spi\n");

    init_spi(&fd);

    VC_RECT_T rect;
    result = vc_dispmanx_rect_set(&rect, 0, 0, captureWidth, captureHeight);

    if (result != 0) {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        fprintf(stderr, "%s: vc_dispmanx_rect_set() failed\n", program);
        exit(EXIT_FAILURE);
    }

    while (1) {
        result = vc_dispmanx_snapshot(displayHandle, resourceHandle,
                                      DISPMANX_NO_ROTATE);

        if (result != 0) {
            vc_dispmanx_resource_delete(resourceHandle);
            vc_dispmanx_display_close(displayHandle);

            fprintf(stderr, "%s: vc_dispmanx_snapshot() failed\n", program);
            exit(EXIT_FAILURE);
        }

        result = vc_dispmanx_resource_read_data(resourceHandle, &rect,
                                                captureBuffer, capturePitch);

        if (result != 0) {
            vc_dispmanx_resource_delete(resourceHandle);
            vc_dispmanx_display_close(displayHandle);

            fprintf(stderr, "%s: vc_dispmanx_resource_read_data() failed\n",
                    program);

            exit(EXIT_FAILURE);
        }

        uint8_t output_buffer[160 * 3];
        uint8_t* output_buffer_front = output_buffer;

        uint32_t span_factor = 199 / 11;

        for (int i = 0; i < 200; i += span_factor) {
            for (int j = 0; j < 200; j += span_factor) {
                int h_index = (((i / span_factor) % 2) == 0) ? j : (199 - j);
                memcpy(output_buffer_front,
                       &captureBuffer[h_index * 3 + i * capturePitch], 3);
                output_buffer_front += 3;
            }
        }

        transpose_rg(output_buffer, 160);
        transfer_start(fd, output_buffer, 160 * 3);
    }

    vc_dispmanx_resource_delete(resourceHandle);
    vc_dispmanx_display_close(displayHandle);
    free(captureBuffer);

    return 0;
}
}
