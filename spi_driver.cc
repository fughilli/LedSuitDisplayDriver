//
// LED Suit Driver - Embedded host driver software for Kevin's LED suit controller.
// Copyright (C) 2019-2020 Kevin Balke
//
// This file is part of LED Suit Driver.
//
// LED Suit Driver is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// LED Suit Driver is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with LED Suit Driver.  If not, see <http://www.gnu.org/licenses/>.
//

#include "spi_driver.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

extern "C" {
#include <fcntl.h>
}

namespace led_driver {

namespace {
// The mode to open the devfs node with.
constexpr uint32_t kDeviceOpenMode = O_RDWR;
}  // namespace

bool SpiDriver::Initialize() {
    fd_ = open(device_.c_str(), kDeviceOpenMode);
    if (fd_ < 0) {
        std::cerr << "Failed to open device " << device_ << "; `open` returned "
                  << fd_;
        return false;
    }

    uint8_t mode;
    uint8_t bits_per_word;
    uint32_t speed_hz;

    if (ioctl(fd_, SPI_IOC_WR_MODE, &mode_) == -1) {
        std::cerr << "Failed to set SPI mode";
        return false;
    }

    if (ioctl(fd_, SPI_IOC_RD_MODE, &mode) == 0) {
        std::cout << "Configured SPI with mode " << static_cast<int>(mode)
                  << std::endl;
    }

    if (ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word_) == -1) {
        std::cerr << "Failed to set SPI bytes per word";
        return false;
    }

    if (ioctl(fd_, SPI_IOC_RD_BITS_PER_WORD, &bits_per_word) == 0) {
        std::cout << "Configured SPI bits per word = "
                  << static_cast<int>(bits_per_word) << std::endl;
    }

    if (ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz_) == -1) {
        std::cerr << "Failed to set SPI max speed";
        return false;
    }

    if (ioctl(fd_, SPI_IOC_RD_MAX_SPEED_HZ, &speed_hz) == 0) {
        std::cout << "Configured SPI max speed as " << speed_hz << " Hz"
                  << std::endl;
    }

    return true;
}

SpiDriver::~SpiDriver() {
    // Dispose of the open devfs file descriptor.
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool SpiDriver::Transfer(const std::vector<uint8_t>& buffer) {
    // Configure the SPI transfer IOCTL block.
    struct spi_ioc_transfer transfer_config;
    memset(&transfer_config, 0, sizeof(transfer_config));
    transfer_config.tx_buf = reinterpret_cast<unsigned long>(buffer.data());
    transfer_config.rx_buf = 0;
    transfer_config.len = buffer.size();
    transfer_config.delay_usecs = delay_us_;
    transfer_config.speed_hz = speed_hz_;
    transfer_config.bits_per_word = bits_per_word_;

    // Perform the transfer.
    if (ioctl(fd_, SPI_IOC_MESSAGE(1), &transfer_config) < 1) {
        std::cerr << "Failed to transfer";
        return false;
    }

    return true;
}

}  // namespace led_driver
