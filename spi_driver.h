#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
}

namespace led_driver {

class SpiDriver {
   public:
    // Idle high = CLK_CPOL set
    enum class ClockPolarity : uint32_t { IDLE_LOW = 0, IDLE_HIGH };

    // Sample trailing = CLK_CPHA set
    enum class ClockPhase : uint32_t { SAMPLE_LEADING = 0, SAMPLE_TRAILING };

    // MSB first = zero
    enum class ByteOrder : uint32_t { MSB_FIRST = 0, LSB_FIRST };

    template <typename... A>
    static std::shared_ptr<SpiDriver> Create(A&&... args) {
        auto spi_driver =
            std::shared_ptr<SpiDriver>(new SpiDriver(std::forward<A>(args)...));
        if (!spi_driver->Initialize()) {
            return nullptr;
        }
        return spi_driver;
    }

    ~SpiDriver();

    // Transfers a buffer of data to the SPI slave device.
    bool Transfer(const std::vector<uint8_t>& buffer);

   private:
    SpiDriver(std::string device, ClockPolarity polarity, ClockPhase phase,
              int bits_per_word, int speed_hz, int delay_us)
        : device_(std::move(device)),
          mode_(((polarity == ClockPolarity::IDLE_HIGH) ? SPI_CPOL : 0) |
                ((phase == ClockPhase::SAMPLE_TRAILING) ? SPI_CPHA : 0)),
          bits_per_word_(bits_per_word),
          speed_hz_(speed_hz),
          delay_us_(delay_us) {}

    // Initializes the SPI driver.
    bool Initialize();

    // The file descriptor of the underlying devfs SPI device.
    int fd_;

    // The devfs node path name for the underlying SPI device.
    std::string device_;

    // The SPI mode for this SPI device.
    uint8_t mode_;
    // The bits per word for this SPI device.
    uint8_t bits_per_word_;
    // The maximum transfer speed, in Hz, for this SPI device.
    uint32_t speed_hz_;
    // The number of microseconds to delay in between transactions for this SPI
    // device.
    uint16_t delay_us_;
};

}  // namespace led_driver

#endif
