#include <iostream>

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
constexpr int kRasterWidth = 200;
constexpr int kRasterHeight = 200;

void TransposeRedGreenPixel(uint8_t* pixel) {
    uint8_t temp = *pixel;
    *pixel = *(pixel + 1);
    *(pixel + 1) = temp;
}

void TransposeRedGreen(std::vector<uint8_t> pixels) {
    auto pixel_ptr = pixels.begin();
    while (pixel_ptr < pixels.end()) {
        TransposeRedGreenPixel(&(*pixel_ptr));
        pixel_ptr += 3;
    }
}
}  // namespace

class SpiImageBufferReceiver : public ImageBufferReceiverInterface {
   public:
    SpiImageBufferReceiver(std::shared_ptr<SpiDriver> spi_driver)
        : spi_driver_(spi_driver) {}
    void Receive(std::shared_ptr<ImageBuffer> image_buffer) override {
        std::vector<uint8_t> output_buffer;
        output_buffer.reserve(kLedBufferLength + 2);

        // LED data address + mode.
        output_buffer.push_back(0x80);
        output_buffer.push_back(0x00);

        ssize_t pixel_stride_x = (kRasterWidth - 1) / (kLedGridWidth - 1);
        ssize_t pixel_stride_y = (kRasterHeight - 1) / (kLedGridHeight - 1);

        for (int i = 0; i < kRasterHeight; i += pixel_stride_y) {
            for (int j = 0; j < kRasterWidth; j += pixel_stride_x) {
                // Serpentine sampling, since the LED strip is placed zigzag to
                // make a 2D raster
                int h_index = (((i / pixel_stride_y) % 2) == 0)
                                  ? j
                                  : ((kRasterWidth - 1) - j);

                ssize_t pixel_index =
                    h_index * kLedChannels +
                    (i / pixel_stride_y) * image_buffer->row_stride;

                // Copy pixel from the sampling location to the end of the
                // output buffer
                std::copy(
                    output_buffer.end(),
                    image_buffer->buffer.begin() + pixel_index,
                    image_buffer->buffer.begin() + pixel_index + kLedChannels);
            }
        }

        TransposeRedGreen(output_buffer);
        spi_driver_->Transfer(output_buffer);
    }

   private:
    constexpr static ssize_t kNumLeds = 160;
    constexpr static ssize_t kLedChannels = 3;
    constexpr static ssize_t kLedBufferLength = kNumLeds * kLedChannels;
    constexpr static ssize_t kLedGridWidth = 12;
    constexpr static ssize_t kLedGridHeight = 12;
    std::shared_ptr<SpiDriver> spi_driver_;
};

int main(int argc, char* argv[]) {
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
}  // namespace led_driver

extern "C" {
int main(int argc, char* argv[]) { return led_driver::main(argc, argv); }
}
