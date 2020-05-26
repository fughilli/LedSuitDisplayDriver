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

#ifndef PULSEAUDIO_INTERFACE_H_
#define PULSEAUDIO_INTERFACE_H_

#include <pulse/pulseaudio.h>

#include "absl/types/span.h"

#include <string>

namespace led_driver {

class PulseAudioInterface {
public:
  using SampleCallbackType = std::function<void(absl::Span<const float>)>;
  PulseAudioInterface(std::string server_name, std::string device_name,
                      std::string stream_name,
                      SampleCallbackType sample_callback)
      : server_name_(std::move(server_name)),
        device_name_(std::move(device_name)),
        stream_name_(std::move(stream_name)),
        sample_callback_(std::move(sample_callback)) {
    memset(&sample_spec_, 0, sizeof(sample_spec_));
    sample_spec_.format = PA_SAMPLE_FLOAT32LE;
    sample_spec_.rate = 44100;
    sample_spec_.channels = 2;
  }
  bool Initialize();
  bool Iterate();

private:
  void ContextStateCallback(pa_context *new_context);
  static void ContextStateCallbackStatic(pa_context *new_context,
                                         void *userdata) {
    reinterpret_cast<PulseAudioInterface *>(userdata)->ContextStateCallback(
        new_context);
  }

  void StreamStateCallback(pa_stream *new_stream);
  static void StreamStateCallbackStatic(pa_stream *new_stream, void *userdata) {
    reinterpret_cast<PulseAudioInterface *>(userdata)->StreamStateCallback(
        new_stream);
  }

  void StreamReadCallback(pa_stream *new_stream, size_t length);
  static void StreamReadCallbackStatic(pa_stream *new_stream, size_t length,
                                       void *userdata) {
    reinterpret_cast<PulseAudioInterface *>(userdata)->StreamReadCallback(
        new_stream, length);
  }

  std::string server_name_;
  std::string device_name_;
  std::string stream_name_;
  SampleCallbackType sample_callback_;
  pa_sample_spec sample_spec_;
  pa_context *context_;
  pa_stream *stream_;
  pa_mainloop_api *mainloop_api_;
  pa_mainloop *mainloop_;
};
} // namespace led_driver

#endif
