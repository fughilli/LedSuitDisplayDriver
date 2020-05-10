#ifndef VISUAL_INTEREST_PROCESSOR_H_
#define VISUAL_INTEREST_PROCESSOR_H_

#include <cstdint>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

#include "absl/time/clock.h"
#include "periodic.h"
#include "projectm_controller.h"
#include "vc_capture_source.h"

namespace led_driver {
class VisualInterestProcessor : public ImageBufferReceiverInterface {
public:
  struct Config {
    // How often to calculate the visual interest.
    int64_t calculation_period_ms = 1000;

    // The moving average decay factor.
    float alpha = 0.7f;

    // Moving average minimum invocations. The moving average will be calculated
    // as `visual_interest_threshold` until this many calculations have elapsed.
    ssize_t moving_average_minimum_invocations = 5;

    // The visual interest threshold. If the moving average drops below this
    // value, the processor will tell the ProjectmController to advance the
    // preset.
    float visual_interest_threshold = 10;

    // Cooldown duration after advancing the preset. When the preset is
    // advanced, the moving average buffer will be cleared and will not have new
    // values pushed until this many calculation periods have elapsed.
    ssize_t cooldown_duration = 10;
  };

  VisualInterestProcessor(
      Config config, std::shared_ptr<ProjectmController> projectm_controller)
      : config_(std::move(config)),
        projectm_controller_(std::move(projectm_controller)),
        periodic_timer_(config_.calculation_period_ms,
                        absl::ToUnixMillis(absl::Now())),
        cooldown_counter_(0), quit_thread_(false) {
    ResetMovingAverage();
  }

  ~VisualInterestProcessor() override;

  void Receive(std::shared_ptr<ImageBuffer> image_buffer) override;

private:
  float CalculateVisualInterest(std::vector<uint8_t> &raw_image);

  void CalculateVisualInterestThread();

  float CalculateMovingAverage(float value);
  void ResetMovingAverage();

  const Config config_;
  std::shared_ptr<ProjectmController> projectm_controller_;
  Periodic<int64_t> periodic_timer_;

  float moving_average_;
  int moving_average_invocations_;
  int cooldown_counter_;

  std::mutex write_mutex_;
  // `current_image_` and `previous_image_` are guarded by `write_mutex_`.
  bool quit_thread_;
  std::vector<uint8_t> current_image_;
  std::vector<uint8_t> previous_image_;

  std::thread calculator_thread_;
  std::condition_variable data_ready_;
};

} // namespace led_driver

#endif
