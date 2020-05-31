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

#include "display_driver.h"

#include <iostream>
#include <sstream>

#define CHECK_RETURN(value)                                                    \
  do {                                                                         \
    if (!(value)) {                                                            \
      return false;                                                            \
    }                                                                          \
  } while (0)

namespace led_driver {
constexpr int DisplayDriver::kDisplayWidth;
constexpr int DisplayDriver::kDisplayHeight;

bool DisplayDriver::SendCommand(Command command) const {
  return SendCommand(absl::Span<const Command>({command}));
}

bool DisplayDriver::SendCommand(absl::Span<const Command> commands) const {
  std::vector<uint8_t> command_buffer_data;
  command_buffer_data.push_back(0);
  for (auto &command : commands) {
    command_buffer_data.push_back(static_cast<const uint8_t>(command.id));
    command_buffer_data.insert(command_buffer_data.end(),
                               command.arguments.begin(),
                               command.arguments.end());
  }
  return i2c_bus_->Write(kDeviceAddress, command_buffer_data);
}

bool DisplayDriver::SendData(absl::Span<const uint8_t> data) const {
  std::vector<uint8_t> data_send_buffer;
  data_send_buffer.push_back(0x40);
  data_send_buffer.insert(data_send_buffer.end(), data.begin(), data.end());
  return i2c_bus_->Write(kDeviceAddress, data_send_buffer);
}

bool DisplayDriver::Update() const {
  CHECK_RETURN(SendCommand({
      {CommandId::kPageAddr, {0, 0xFF}},
      {CommandId::kColumnAddr, {0, kDisplayWidth - 1}},
  }));
  return SendData(display_buffer_);
}

bool DisplayDriver::Initialize() const {
  return SendCommand({
      {CommandId::kDisplayOff, {}},
      {CommandId::kSetDisplayClockDiv, {0x80}},
      {CommandId::kSetMultiplex, {kDisplayHeight - 1}},
      {CommandId::kSetDisplayOffset, {0}},
      {CommandId::kSetStartLine, {}},
      {CommandId::kChargePump, {0x14}},
      {CommandId::kMemoryMode, {0}},
      {static_cast<CommandId>(static_cast<uint8_t>(CommandId::kSegRemap) |
                              0x01),
       {}},
      {CommandId::kComScanDec, {}},
      {CommandId::kSetComPins, {0x02}},
      {CommandId::kSetContrast, {0x8F}},
      {CommandId::kSetPrecharge, {0xF1}},
      {CommandId::kSetVcomDetect, {0x40}},
      {CommandId::kDisplayAllOnResume, {}},
      {CommandId::kNormalDisplay, {}},
      {CommandId::kDeactivateScroll, {}},
      {CommandId::kDisplayOn, {}},
  });
}

std::pair<int, int> DisplayDriver::GetSize() const {
  return std::make_pair(kDisplayWidth, kDisplayHeight);
}

bool DisplayDriver::RenderImage(absl::Span<const uint8_t> image) {
  if (image.size() != display_buffer_.size()) {
    return false;
  }

  std::copy(image.begin(), image.end(), display_buffer_.begin());
  return true;
}

bool DisplayDriver::RenderImage(const std::vector<uint8_t> image) {
  return RenderImage(absl::Span<const uint8_t>(image.data(), image.size()));
}

bool DisplayDriver::DrawPixel(int x, int y, Color color) {
  if (x < 0 || x >= kDisplayWidth || y < 0 || y >= kDisplayHeight) {
    return false;
  }
  constexpr int kBlockWidth = 4;
  constexpr int kBlockHeight = 8;
  constexpr int kBlockCountX = kDisplayWidth / kBlockWidth;
  constexpr int kBlockCountY = kDisplayHeight / kBlockHeight;

  int block_index = (x / kBlockWidth) + kBlockCountX * (y / kBlockHeight);
  int block_pixel_index = kBlockHeight * (x % kBlockWidth) + (y % kBlockHeight);
  int bit_offset =
      block_index * (kBlockWidth * kBlockHeight) + block_pixel_index;
  int byte_offset = bit_offset / 8;
  bit_offset %= 8;
  switch (color) {
  case Color::kBlack:
    display_buffer_[byte_offset] &= ~(1 << bit_offset);
    break;
  case Color::kWhite:
    display_buffer_[byte_offset] |= (1 << bit_offset);
    break;
  case Color::kInvert:
    display_buffer_[byte_offset] ^= (1 << bit_offset);
    break;
  }
  return true;
}

} // namespace led_driver
