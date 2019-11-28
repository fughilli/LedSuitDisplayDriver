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
}

#include <iostream>

#include "vc_capture_source.h"

namespace led_driver {
extern "C" {

const char* device = "/dev/spidev0.0";
const uint8_t mode = 0;
const uint8_t bits = 8;
const uint32_t speed = 15600000;
const uint16_t delay = 0;

void pabort(const char* s) {
    perror(s);
    abort();
}

void init_spi(int* fd_) {
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

    //ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    //if (ret == -1) pabort("can't get spi mode");

    /*
     * bits per word
     */
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) pabort("can't set bits per word");

    //ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    //if (ret == -1) pabort("can't get bits per word");

    /*
     * max speed hz
     */
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1) pabort("can't set max speed hz");

    //ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    //if (ret == -1) pabort("can't get max speed hz");

    printf("spi mode: %d\n", mode);
    printf("bits per word: %d\n", bits);
    printf("max speed: %d Hz (%d KHz)\n", speed, speed / 1000);
}

void transfer(int fd, const uint8_t* data, size_t length) {
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

void transfer_start(int fd, const uint8_t* data, size_t length) {
    uint8_t* tx_data = reinterpret_cast<uint8_t*>(malloc(length + 2));
    tx_data[0] = 0x80;
    tx_data[1] = 0;
    memcpy(tx_data + 2, data, length);
    transfer(fd, tx_data, length + 2);
    free(tx_data);
}

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
}

class SpiImageBufferReceiver : public ImageBufferReceiverInterface {
   public:
    SpiImageBufferReceiver(int fd) : fd_(fd) {}
    void Receive(std::shared_ptr<ImageBuffer> image_buffer) override {
        uint8_t output_buffer[160 * 3];
        uint8_t* output_buffer_front = output_buffer;

        uint32_t span_factor = 199 / 11;

        for (int i = 0; i < 200; i += span_factor) {
            for (int j = 0; j < 200; j += span_factor) {
                int h_index = (((i / span_factor) % 2) == 0) ? j : (199 - j);
                memcpy(
                    output_buffer_front,
                    &image_buffer
                         ->buffer[h_index * 3 + i * image_buffer->row_stride],
                    3);
                output_buffer_front += 3;
            }
        }

        transpose_rg(output_buffer, 160);
        transfer_start(fd_, output_buffer, 160 * 3);
    }

   private:
    int fd_;
};

int main(int argc, char* argv[]) {
    int fd;

    printf("Initializing spi\n");

    init_spi(&fd);

    auto capture_source =
        VcCaptureSource::Create(std::make_shared<SpiImageBufferReceiver>(fd));

    if (capture_source == nullptr) {
        return 1;
    }

    capture_source->ConfigureCaptureRegion(0, 0, 200, 200);

    while (1) {
        if (!capture_source->Capture()) {
            return 1;
        }
    }

    return 0;
}
}  // namespace led_driver

extern "C" {
int main(int argc, char* argv[]) { return led_driver::main(argc, argv); }
}
