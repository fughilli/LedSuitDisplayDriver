#include "vc_capture_source.h"

#include <iostream>

namespace led_driver {

namespace {
// Capture buffers from VideoCore in RGB888 format.
constexpr static VC_IMAGE_TYPE_T kImageType = VC_IMAGE_RGB888;
constexpr static uint32_t kDisplayNumber = 0;
constexpr static ssize_t kImageBytesPerPixel = 3;
constexpr static int kVcBufferAlignment = 16;

template <typename T>
T AlignTo(T value, T alignment) {
    T multiplier = (value + alignment - 1) / alignment;
    return multiplier * alignment;
}
};  // namespace

bool VcCaptureSource::ConfigureCaptureRegion(int x, int y, int width,
                                             int height) {
    const std::lock_guard capture_configured_mu_lock(capture_configured_mu_);
    int32_t result = vc_dispmanx_rect_set(&capture_rect_, x, y, width, height);
    if (result != 0) {
        std::cerr << "Failed to set the capture rectangle; "
                     "`vc_dispmanx_rect_set` returned "
                  << result << std::endl;
        return false;
    }

    capture_buffer_ = std::make_shared<ImageBuffer>();
    capture_buffer_->row_stride =
        kImageBytesPerPixel * AlignTo(mode_info_.width, kVcBufferAlignment);
    capture_buffer_->buffer.resize(capture_buffer_->row_stride * height, 0);
    capture_buffer_->bytes_per_pixel = kImageBytesPerPixel;

    capture_configured_ = true;
    return true;
}

bool VcCaptureSource::Capture() {
    // Make sure we're initialized.
    if (!Initialize()) {
        std::cerr << "Failed to capture; capture source isn't initialized"
                  << std::endl;
        return false;
    }

    const std::lock_guard capture_configured_mu_lock(capture_configured_mu_);
    if (!capture_configured_) {
        std::cerr << "Failed to capture; capture region isn't configured"
                  << std::endl;
        return false;
    }

    // Capture a frame, unrotated.
    int32_t result = vc_dispmanx_snapshot(
        vc_display_handle_, vc_image_buffer_handle_, DISPMANX_NO_ROTATE);

    if (result != 0) {
        std::cerr << "Failed to capture; `vc_dispmanx_snapshot` returned "
                  << result << std::endl;
        return false;
    }

    result = vc_dispmanx_resource_read_data(
        vc_image_buffer_handle_, &capture_rect_, capture_buffer_->buffer.data(),
        capture_buffer_->row_stride);

    if (result != 0) {
        std::cerr
            << "Failed to capture; `vc_dispmanx_resource_read_data` returned "
            << result << std::endl;
        return false;
    }

    receiver_->Receive(capture_buffer_);
    return true;
}

bool VcCaptureSource::Initialize() {
    const std::lock_guard<std::mutex> initialized_mu_lock(initialized_mu_);
    if (initialized_) {
        return true;
    }

    // Init the Broadcom host library.
    bcm_host_init();

    // Open the display.
    vc_display_handle_ = vc_dispmanx_display_open(kDisplayNumber);

    if (vc_display_handle_ == 0) {
        std::cerr << "Failed to open display " << kDisplayNumber << ": "
                  << vc_display_handle_ << std::endl;
        return false;
    }

    // Grab the mode info.
    if (vc_dispmanx_display_get_info(vc_display_handle_, &mode_info_) != 0) {
        std::cerr << "Unable to read display information" << std::endl;
        return false;
    }

    vc_image_buffer_handle_ = vc_dispmanx_resource_create(
        kImageType, mode_info_.width, mode_info_.height, &vc_image_buffer_ptr_);

    if (vc_image_buffer_handle_ == 0) {
        std::cerr << "Failed to create image buffer; "
                  << "`vc_dispmanx_resource_create` returned "
                  << vc_image_buffer_handle_ << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}
}  // namespace led_driver
