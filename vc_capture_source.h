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

#ifndef VC_CAPTURE_SOURCE_H
#define VC_CAPTURE_SOURCE_H

#include <cstddef>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "bcm_host.h"

namespace led_driver {

// Wrapper for a raw buffer of image data.
struct ImageBuffer {
  std::vector<uint8_t> buffer;
  ssize_t row_stride;
  ssize_t bytes_per_pixel;
};

// Interface for objects that can receive image buffers from a
// `VcCaptureSource`.
struct ImageBufferReceiverInterface {
  virtual ~ImageBufferReceiverInterface() {}

  // Receive an image buffer.
  virtual void Receive(std::shared_ptr<ImageBuffer> image_buffer) = 0;
};

class ImageBufferReceiverMultiplexer : public ImageBufferReceiverInterface {
public:
  ImageBufferReceiverMultiplexer(
      std::initializer_list<std::shared_ptr<ImageBufferReceiverInterface>>
          receivers)
      : receivers_(receivers) {}

  void Receive(std::shared_ptr<ImageBuffer> image_buffer) override {
    for (auto &receiver : receivers_) {
      receiver->Receive(image_buffer);
    }
  }

private:
  std::vector<std::shared_ptr<ImageBufferReceiverInterface>> receivers_;
};

class VcCaptureSource {
public:
  bool ConfigureCaptureRegion(int x, int y, int width, int height);
  bool Capture();

  template <typename... A>
  static std::shared_ptr<VcCaptureSource> Create(A &&... args) {
    auto vc_capture_source = std::shared_ptr<VcCaptureSource>(
        new VcCaptureSource(std::forward<A>(args)...));
    if (!vc_capture_source->Initialize()) {
      return nullptr;
    }
    return vc_capture_source;
  }

  ~VcCaptureSource() {
    // Close the image buffer handle if we have one.
    if (vc_image_buffer_handle_) {
      vc_dispmanx_resource_delete(vc_image_buffer_handle_);
    }

    // Close the display handle if we have one.
    if (vc_display_handle_) {
      vc_dispmanx_display_close(vc_display_handle_);
    }
  }

private:
  VcCaptureSource(std::shared_ptr<ImageBufferReceiverInterface> receiver)
      : initialized_(false), receiver_(std::move(receiver)) {}

  // Initializes the capture source.
  bool Initialize();

  // Whether or not the source has been initialized.
  std::mutex initialized_mu_;
  bool initialized_;

  // Handles to VideoCore resources.
  DISPMANX_RESOURCE_HANDLE_T vc_image_buffer_handle_;
  uint32_t vc_image_buffer_ptr_;
  DISPMANX_DISPLAY_HANDLE_T vc_display_handle_;

  // Mode information for the display, retrieved from VideoCore.
  DISPMANX_MODEINFO_T mode_info_;

  // Receiver which will get buffers of image data whenever `Capture` is
  // invoked.
  std::shared_ptr<ImageBufferReceiverInterface> receiver_;

  // Whether or not the capture region has been configured.
  std::mutex capture_configured_mu_;
  bool capture_configured_;

  // The capture region rectangle.
  VC_RECT_T capture_rect_;

  // The image buffer to collect image bytes into.
  std::shared_ptr<ImageBuffer> capture_buffer_;
};
} // namespace led_driver

#endif // VC_CAPTURE_SOURCE_H
