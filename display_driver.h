//
// LED Suit Driver - Embedded host driver software for Kevin's LED suit
// controller. Copyright (C) 2019-2020 Kevin Balke
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

#ifndef DISPLAY_DRIVER_H_
#define DISPLAY_DRIVER_H_

#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "HexapodController2/bus.h"
#include "absl/types/span.h"

namespace led_driver {
class DisplayDriver {
public:
  enum Color { kWhite = 0, kBlack, kInvert };

  DisplayDriver(std::shared_ptr<i2c::Bus> i2c_bus)
      : i2c_bus_(std::move(i2c_bus)) {
    Clear();
  }
  DisplayDriver(std::string i2c_dev)
      : DisplayDriver(std::make_shared<i2c::Bus>(i2c_dev)) {}

  bool Initialize() const;
  bool RenderImage(absl::Span<const uint8_t> image);
  bool RenderImage(const std::vector<uint8_t> image);
  std::pair<int, int> GetSize() const;
  bool DrawPixel(int x, int y, Color color);
  void Clear();
  bool Update() const;

private:
  constexpr static int kDisplayWidth = 128;
  constexpr static int kDisplayHeight = 32;
  constexpr static uint8_t kDeviceAddress = 0x3c;

  enum class CommandId : uint8_t {
    kMemoryMode = 0x20,
    kColumnAddr = 0x21,
    kPageAddr = 0x22,
    kSetContrast = 0x81,
    kChargePump = 0x8D,
    kSegRemap = 0xA0,
    kDisplayAllOnResume = 0xA4,
    kDisplayAllOn = 0xA5,
    kNormalDisplay = 0xA6,
    kInvertDisplay = 0xA7,
    kSetMultiplex = 0xA8,
    kDisplayOff = 0xAE,
    kDisplayOn = 0xAF,
    kComScanInc = 0xC0,
    kComScanDec = 0xC8,
    kSetDisplayOffset = 0xD3,
    kSetDisplayClockDiv = 0xD5,
    kSetPrecharge = 0xD9,
    kSetComPins = 0xDA,
    kSetVcomDetect = 0xDB,
    kSetLowColumn = 0x00,
    kSetHighColumn = 0x10,
    kSetStartLine = 0x40,
    kExternalVcc = 0x01,
    kSwitchCapVcc = 0x02,
    kRightHorizontalScroll = 0x26,
    kLeftHorizontalScroll = 0x27,
    kVerticalAndRightHorizontalScroll = 0x29,
    kVerticalAndLeftHorizontalScroll = 0x2A,
    kDeactivateScroll = 0x2E,
    kActivateScroll = 0x2F,
    kSetVerticalScrollArea = 0xA3,
  };

  struct Command {
    CommandId id;
    std::vector<uint8_t> arguments;
  };

  bool SendCommand(Command command) const;
  bool SendCommand(absl::Span<const Command> commands) const;
  bool SendData(absl::Span<const uint8_t> data) const;

  std::shared_ptr<i2c::Bus> i2c_bus_;
  std::array<uint8_t, kDisplayWidth * kDisplayHeight / 8> display_buffer_;
};
} // namespace led_driver

#endif
