#include <iostream>
#include <utility>
#include <vector>

#include "spi_driver.h"
#include "vc_capture_source.h"

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
constexpr int kRasterWidth = 100;
constexpr int kRasterHeight = 100;
constexpr int kLedGridWidth = 12;
constexpr int kLedGridHeight = 13;

void TransposeRedGreenPixel(uint8_t *pixel) {
  uint8_t temp = *pixel;
  *pixel = *(pixel + 1);
  *(pixel + 1) = temp;
}

void TransposeRedGreen(uint8_t *pixels, ssize_t num_pixels) {
  while (num_pixels--) {
    TransposeRedGreenPixel(pixels);
    pixels += 3;
  }
}

using Coordinate = std::pair<ssize_t, ssize_t>;

std::vector<Coordinate> ComputeCoordinatesSerpentine() {
  std::vector<Coordinate> coordinates;

  ssize_t pixel_stride_x = (kRasterWidth - 1) / (kLedGridWidth - 1);
  ssize_t pixel_stride_y = (kRasterHeight - 1) / (kLedGridHeight - 1);

  for (int i = 0; i < kRasterHeight; i += pixel_stride_y) {
    for (int j = 0; j < kRasterWidth; j += pixel_stride_x) {
      // Serpentine sampling, since the LED strip is placed zigzag to
      // make a 2D raster
      int h_index =
          (((i / pixel_stride_y) % 2) == 0) ? j : ((kRasterWidth - 1) - j);

      coordinates.emplace_back(h_index, i);
    }
  }

  return coordinates;
}

std::vector<Coordinate> ComputeCoordinatesLinear() {
  std::vector<Coordinate> coordinates;
  for (int i = 0; i < 100; ++i) {
    coordinates.emplace_back(i, 0);
  }
  return coordinates;
}

} // namespace

class SpiImageBufferReceiver : public ImageBufferReceiverInterface {
public:
  SpiImageBufferReceiver(std::shared_ptr<SpiDriver> spi_driver)
      : spi_driver_(std::move(spi_driver)) {}
  void Receive(std::shared_ptr<ImageBuffer> image_buffer) override {
    output_buffer_.resize(kLedBufferLength + 2, 0);

    // LED data address + mode.
    output_buffer_[0] = 0x80;
    output_buffer_[1] = 0x00;

    auto end_iter = output_buffer_.begin() + 2;

    for (const auto coordinate : ComputeCoordinatesLinear()) {
      ssize_t pixel_index = coordinate.first * kLedChannels +
                            coordinate.second * image_buffer->row_stride;

      // Copy pixel from the sampling location to the end of the
      // output buffer
      std::copy(image_buffer->buffer.begin() + pixel_index,
                image_buffer->buffer.begin() + pixel_index + kLedChannels,
                end_iter);

      end_iter += 3;
    }

    TransposeRedGreen(&(output_buffer_[2]), kNumLeds);
    spi_driver_->Transfer(output_buffer_);
  }

private:
  constexpr static ssize_t kNumLeds = 180;
  constexpr static ssize_t kLedChannels = 3;
  constexpr static ssize_t kLedBufferLength = kNumLeds * kLedChannels;
  std::shared_ptr<SpiDriver> spi_driver_;
  std::vector<uint8_t> output_buffer_;
};

int main(int argc, char *argv[]) {
  auto spi_driver = SpiDriver::Create(kDevice, kClockPolarity, kClockPhase,
                                      kBitsPerWord, kSpeedHz, kDelayUs);

  if (spi_driver == nullptr) {
    return 1;
  }

  auto capture_source = VcCaptureSource::Create(
      std::make_shared<SpiImageBufferReceiver>(spi_driver));

  if (capture_source == nullptr) {
    return 1;
  }

  capture_source->ConfigureCaptureRegion(0, 0, kRasterWidth, kRasterHeight);

  while (1) {
    if (!capture_source->Capture()) {
      return 1;
    }
  }

  return 0;
}
} // namespace led_driver

extern "C" {
int main(int argc, char *argv[]) { return led_driver::main(argc, argv); }
}
