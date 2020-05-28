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
namespace {
std::ostream &operator<<(std::ostream &os, absl::Span<const uint8_t> span) {
  std::stringstream output_string;
  output_string << std::hex << "[";
  for (int i = 0; i < span.size(); ++i) {
    output_string << static_cast<const unsigned int>(span[i]);
    if (i < span.size() - 1) {
      output_string << ",";
    }
  }
  output_string << "]";
  return os << output_string.str();
}
template <typename T>
std::ostream &operator<<(std::ostream &os, absl::Span<const T> span) {
  std::stringstream output_string;
  output_string << std::hex << "[";
  for (int i = 0; i < span.size(); ++i) {
    output_string << span[i];
    if (i < span.size() - 1) {
      output_string << ",";
    }
  }
  output_string << "]";
  return os << output_string.str();
}
} // namespace

bool DisplayDriver::SendNakedCommand(uint8_t command) {
  std::cout << "Sending naked commands..." << std::endl;
  std::vector<uint8_t> command_buffer_data;
  command_buffer_data.push_back(0);
  command_buffer_data.push_back(command);
  std::cout << "Final commands buffer: "
            << absl::Span<const uint8_t>(command_buffer_data) << std::endl;
  return i2c_bus_->Write(kDeviceAddress, command_buffer_data);
}

bool DisplayDriver::SendCommand(Command command) {
  return SendCommand(absl::Span<const Command>({command}));
}

bool DisplayDriver::SendCommand(absl::Span<const Command> commands) {
  std::cout << "Sending commands..." << std::endl;
  std::vector<uint8_t> command_buffer_data;
  command_buffer_data.push_back(0);
  for (auto &command : commands) {
    std::cout << "Adding command: " << std::hex
              << static_cast<const unsigned int>(command.id) << std::endl;
    command_buffer_data.push_back(static_cast<const uint8_t>(command.id));
    std::cout << "Arguments: " << command.arguments << std::endl;
    command_buffer_data.insert(command_buffer_data.end(),
                               command.arguments.begin(),
                               command.arguments.end());
  }
  std::cout << "Final commands buffer: "
            << absl::Span<const uint8_t>(command_buffer_data) << std::endl;
  return i2c_bus_->Write(kDeviceAddress, command_buffer_data);
}

bool DisplayDriver::SendData(absl::Span<const uint8_t> data) {
  std::cout << "Sending data..." << std::endl;
  std::vector<uint8_t> data_send_buffer;
  data_send_buffer.push_back(0x40);
  data_send_buffer.insert(data_send_buffer.end(), data.begin(), data.end());
  std::cout << "Data: " << data_send_buffer << std::endl;
  return i2c_bus_->Write(kDeviceAddress, data_send_buffer);
}
bool DisplayDriver::Update() {
  std::vector<Command> display_update_commands = {
      {CommandId::kPageAddr, {0, 0xFF}},
      {CommandId::kColumnAddr, {0}},
  };
  CHECK_RETURN(SendCommand(display_update_commands));
  CHECK_RETURN(SendNakedCommand(kDisplayWidth - 1));

  return SendData(display_buffer_);
}

bool DisplayDriver::Initialize() {
  CHECK_RETURN(SendCommand({
      {CommandId::kDisplayOff, {}},
      {CommandId::kSetDisplayClockDiv, {0x80}},
      {CommandId::kSetMultiplex, {}},
  }));
  CHECK_RETURN(SendNakedCommand(kDisplayHeight - 1));
  CHECK_RETURN(SendCommand({
      {CommandId::kSetDisplayOffset, {0}},
      {CommandId::kSetStartLine, {}},
      {CommandId::kChargePump, {}},
  }));
  CHECK_RETURN(SendNakedCommand(0x14));
  CHECK_RETURN(SendCommand({
      {CommandId::kMemoryMode, {0}},
      {static_cast<CommandId>(static_cast<uint8_t>(CommandId::kSegRemap) |
                              0x01),
       {}},
      {CommandId::kComScanDec, {}},
  }));
  CHECK_RETURN(SendCommand({CommandId::kSetComPins, {}}));
  CHECK_RETURN(SendNakedCommand(0x02));
  CHECK_RETURN(SendCommand({CommandId::kSetContrast, {}}));
  CHECK_RETURN(SendNakedCommand(0x8F));
  CHECK_RETURN(SendCommand({CommandId::kSetPrecharge, {}}));
  CHECK_RETURN(SendNakedCommand(0xF1));

  return SendCommand({
      {CommandId::kSetVcomDetect, {0x40}},
      {CommandId::kDisplayAllOnResume, {}},
      {CommandId::kNormalDisplay, {}},
      {CommandId::kDeactivateScroll, {}},
      {CommandId::kDisplayOn, {}},
  });
}
} // namespace led_driver
