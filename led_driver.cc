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

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/types/span.h"
#include "led_driver/led_mapping.pb.h"
#include "pixel_utils.h"
#include "projectm_controller.h"
#include "spi_driver.h"
#include "vc_capture_source.h"
#include "visual_interest_processor.h"

struct LedIntensity {
  LedIntensity(float intensity) : intensity(intensity) {}

  float intensity;
};

std::string AbslUnparseFlag(LedIntensity intensity) {
  return absl::UnparseFlag(intensity.intensity);
}

bool AbslParseFlag(const absl::string_view text, LedIntensity *intensity,
                   std::string *error) {
  if (!absl::ParseFlag(text, &intensity->intensity, error)) {
    return false;
  }
  if (intensity->intensity < 0 || intensity->intensity > 1) {
    *error = "intensity must be between 0 and 1";
    return false;
  }
  return true;
}

ABSL_FLAG(std::string, mapping_file, "mapping.binaryproto",
          "File containing the LED mapping");
ABSL_FLAG(LedIntensity, intensity, LedIntensity(1.0f),
          "Scale factor for LED intensity");
ABSL_FLAG(bool, enable_projectm_controller, true,
          "Whether to enable the ProjectM Controller");
ABSL_FLAG(ssize_t, calculation_period_ms, 1000,
          "Period in milliseconds for calculating the visual interest of a "
          "visualizer frame");
ABSL_FLAG(float, alpha, 0.7f,
          "Moving average decay factor for the visual interest calculator");
ABSL_FLAG(ssize_t, moving_average_minimum_invocations, 5,
          "Minimum number of calculations needed to check the moving average "
          "against the threshold");
ABSL_FLAG(float, visual_interest_threshold, 10,
          "Visual interest threshold below which the ProjectM Controller will "
          "advance the preset");
ABSL_FLAG(ssize_t, cooldown_duration, 10,
          "Calculation periods to wait after advancing the preset before "
          "beginning to calculate the moving average");
ABSL_FLAG(int, raster_width, 100, "Width of the source raster, in pixels");
ABSL_FLAG(int, raster_height, 100, "Height of the source raster, in pixels");
ABSL_FLAG(int, raster_x, 100, "X position of the source raster, in pixels");
ABSL_FLAG(int, raster_y, 100, "Y position of the source raster, in pixels");
ABSL_FLAG(int, flicker_threshold, 200,
          "Threshold against which to trigger flickering");
ABSL_FLAG(float, flicker_ratio, 0.8f,
          "Ratio of pixels that need to be above flicker_threshold");
ABSL_FLAG(bool, blank_display, false,
          "If set, clears the display and immediately exits");
ABSL_FLAG(int, indicate_progress, 0,
          "If set, configures the first N leds to Red.");
ABSL_FLAG(int, clamp_threshold, 0,
          "Pixel values with norm below this threshold will be clamped to 0.");

namespace led_driver {

namespace {

const std::string kDevice = "/dev/spidev0.0";
constexpr SpiDriver::ClockPolarity kClockPolarity =
    SpiDriver::ClockPolarity::IDLE_LOW;
constexpr SpiDriver::ClockPhase kClockPhase =
    SpiDriver::ClockPhase::SAMPLE_LEADING;
constexpr int kBitsPerWord = 8;
constexpr int kSpeedHz = 15600000;
constexpr int kDelayUs = 0;

using Coordinate = std::pair<ssize_t, ssize_t>;

}  // namespace

class SpiImageBufferReceiver : public ImageBufferReceiverInterface {
 public:
  SpiImageBufferReceiver(std::shared_ptr<SpiDriver> spi_driver,
                         std::vector<Coordinate> coordinates,
                         LedIntensity intensity, int flicker_threshold,
                         float flicker_ratio, int clamp_threshold)
      : spi_driver_(std::move(spi_driver)),
        coordinates_(std::move(coordinates)),
        intensity_(intensity),
        flicker_threshold_(flicker_threshold),
        flicker_ratio_(flicker_ratio),
        clamp_threshold_(clamp_threshold) {}
  void Receive(std::shared_ptr<ImageBuffer> image_buffer) override {
    std::vector<uint8_t> output_buffer_;
    output_buffer_.resize(kLedBufferLength + 2, 0);

    // LED data address + mode.
    output_buffer_[0] = 0x80;
    output_buffer_[1] = 0x00;

    auto end_iter = output_buffer_.begin() + 2;

    for (const auto coordinate : coordinates_) {
      ssize_t pixel_index = coordinate.first * kLedChannels +
                            coordinate.second * image_buffer->row_stride;

      absl::Span<uint8_t> pixel{image_buffer->buffer.data() + pixel_index,
                                kLedChannels};
      bool draw = false;
      for (auto value : pixel) {
        if (value >= clamp_threshold_) draw = true;
      }

      if (draw) {
        // Copy pixel from the sampling location to the end of the
        // output buffer
        std::copy(image_buffer->buffer.begin() + pixel_index,
                  image_buffer->buffer.begin() + pixel_index + kLedChannels,
                  end_iter);
      }

      end_iter += 3;
    }

    FullWhiteCompensate(
        absl::Span<uint8_t>(&output_buffer_[2], output_buffer_.size() - 2));

    ScalePixelValues(&(output_buffer_[2]), intensity_.intensity, kNumLeds);
    corrector_.CorrectPixelsInPlace(&(output_buffer_[2]), kNumLeds);
    TransposeRedGreen(&(output_buffer_[2]), kNumLeds);
    spi_driver_->Transfer(output_buffer_);
  }

 private:
  void FullWhiteCompensate(absl::Span<uint8_t> output_buffer) {
    ++flicker_counter_;

    int num_over_threshold = 0;
    for (int i = 0; i < kLedBufferLength; ++i) {
      if (output_buffer[i] > flicker_threshold_) {
        ++num_over_threshold;
      }
    }

    if (num_over_threshold >
        static_cast<int>(kLedBufferLength * flicker_ratio_)) {
      for (int i = 0; i < kNumLeds; ++i) {
        if ((i & kFlickerModulus) != (flicker_counter_ & kFlickerModulus)) {
          memset(&output_buffer[i * kLedChannels], 0, kLedChannels);
        }
      }
    }
  }

  constexpr static ssize_t kNumLeds = 900;
  constexpr static ssize_t kLedChannels = 3;
  constexpr static ssize_t kLedBufferLength = kNumLeds * kLedChannels;
  constexpr static uint32_t kFlickerModulus = 0x3;
  std::shared_ptr<SpiDriver> spi_driver_;
  std::vector<Coordinate> coordinates_;
  LedIntensity intensity_;
  int flicker_threshold_;
  float flicker_ratio_;
  int flicker_counter_;
  int clamp_threshold_;

  const ColorCorrector corrector_{
      {.gamma = {2.8f, 2.8f, 2.8f},
       .peak_brightness = {(390.0f + 420.0f) / 2, (660.0f + 720.0f) / 2,
                           (180.0f + 200.0f) / 2}}};
};

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);

  std::ifstream mapping_file;
  mapping_file.open(absl::GetFlag(FLAGS_mapping_file));
  ledsuit::mapping::Mapping mapping;
  mapping.ParseFromIstream(&mapping_file);

  std::vector<Coordinate> coordinates;
  for (const auto &sample : mapping.samples()) {
    if (!(sample.has_x() && sample.has_y())) {
      std::cerr << "Sample missing component";
      continue;
    }
    coordinates.emplace_back(
        static_cast<ssize_t>(sample.x() *
                             (absl::GetFlag(FLAGS_raster_width) - 1)),
        static_cast<ssize_t>(sample.y() *
                             (absl::GetFlag(FLAGS_raster_height) - 1)));
  }

  auto spi_driver = SpiDriver::Create(kDevice, kClockPolarity, kClockPhase,
                                      kBitsPerWord, kSpeedHz, kDelayUs);

  if (absl::GetFlag(FLAGS_blank_display)) {
    std::cout << "Clearing display" << std::endl;
    std::vector<uint8_t> empty_raster(3 * 900 + 2, 0);
    empty_raster[0] = 0x80;
    empty_raster[1] = 0x00;
    spi_driver->Transfer(empty_raster);
    return 0;
  }

  int indicate_progress = absl::GetFlag(FLAGS_indicate_progress);
  if (indicate_progress > 0) {
    std::cout << "Indicating progress" << std::endl;
    std::vector<uint8_t> empty_raster(3 * 900 + 2, 0);
    int index = 0;
    while (indicate_progress--) {
      empty_raster[2 + index * 3] = 100;
      ++index;
    }
    empty_raster[0] = 0x80;
    empty_raster[1] = 0x00;
    spi_driver->Transfer(empty_raster);
    return 0;
  }

  if (spi_driver == nullptr) {
    std::cerr << "Failed to create SPI driver" << std::endl;
    return 1;
  }

  auto image_buffer_receiver = std::make_shared<SpiImageBufferReceiver>(
      spi_driver, coordinates, absl::GetFlag(FLAGS_intensity),
      absl::GetFlag(FLAGS_flicker_threshold),
      absl::GetFlag(FLAGS_flicker_ratio), absl::GetFlag(FLAGS_clamp_threshold));

  if (image_buffer_receiver == nullptr) {
    std::cerr << "Failed to create image buffer receiver" << std::endl;
    return 1;
  }

  std::shared_ptr<VcCaptureSource> capture_source;
  if (absl::GetFlag(FLAGS_enable_projectm_controller)) {
    auto projectm_controller = ProjectmController::Create();

    if (projectm_controller == nullptr) {
      std::cerr << "Failed to create projectm controller" << std::endl;
      return 1;
    }

    VisualInterestProcessor::Config config;
    config.calculation_period_ms = absl::GetFlag(FLAGS_calculation_period_ms);
    config.alpha = absl::GetFlag(FLAGS_alpha);
    config.moving_average_minimum_invocations =
        absl::GetFlag(FLAGS_moving_average_minimum_invocations);
    config.visual_interest_threshold =
        absl::GetFlag(FLAGS_visual_interest_threshold);
    config.cooldown_duration = absl::GetFlag(FLAGS_cooldown_duration);
    auto visual_interest_processor =
        std::make_shared<VisualInterestProcessor>(config, projectm_controller);

    if (visual_interest_processor == nullptr) {
      std::cerr << "Failed to create visual interest processor" << std::endl;
    }

    auto image_buffer_receiver_multiplexer =
        std::shared_ptr<ImageBufferReceiverMultiplexer>(
            new ImageBufferReceiverMultiplexer(
                {image_buffer_receiver, visual_interest_processor}));

    capture_source = VcCaptureSource::Create(image_buffer_receiver_multiplexer);
  } else {
    capture_source = VcCaptureSource::Create(image_buffer_receiver);
  }

  if (capture_source == nullptr) {
    std::cerr << "Failed to create capture source" << std::endl;
    return 1;
  }

  capture_source->ConfigureCaptureRegion(
      absl::GetFlag(FLAGS_raster_x), absl::GetFlag(FLAGS_raster_y),
      absl::GetFlag(FLAGS_raster_width), absl::GetFlag(FLAGS_raster_height));

  while (1) {
    if (!capture_source->Capture()) {
      return 1;
    }
  }

  return 0;
}
}  // namespace led_driver

extern "C" {
int main(int argc, char *argv[]) { return led_driver::main(argc, argv); }
}
