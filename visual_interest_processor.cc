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

#include <iostream>

#include "visual_interest_processor.h"

namespace led_driver {

VisualInterestProcessor::~VisualInterestProcessor() {
  {
    const std::lock_guard<std::mutex> write_lock(write_mutex_);
    quit_thread_ = true;
  }
  data_ready_.notify_one();
  calculator_thread_.join();
}

void VisualInterestProcessor::Receive(
    std::shared_ptr<ImageBuffer> image_buffer) {
  if (periodic_timer_.IsDue(absl::ToUnixMillis(absl::Now()))) {
    if (write_mutex_.try_lock()) {
      if (current_image_.empty()) {
        current_image_ = image_buffer->buffer;
      }
      if (!calculator_thread_.joinable()) {
        std::cerr << "Creating calculation thread" << std::endl;
        calculator_thread_ = std::thread(std::bind(
            &VisualInterestProcessor::CalculateVisualInterestThread, this));
      }
      data_ready_.notify_one();
      write_mutex_.unlock();
    }
  }
}

float VisualInterestProcessor::CalculateVisualInterest(
    std::vector<uint8_t> &raw_image) {
  if (previous_image_.size() != raw_image.size()) {
    previous_image_ = raw_image;
    return 0.0f;
  }
  int64_t delta_energy = 0;
  for (int i = 0; i < raw_image.size(); ++i) {
    delta_energy += std::sqrt(std::abs(raw_image[i] - previous_image_[i]));
  }

  previous_image_ = raw_image;
  return static_cast<float>(delta_energy) / raw_image.size();
}

void VisualInterestProcessor::CalculateVisualInterestThread() {
  while (1) {
    float visual_interest = 0;
    {
      std::unique_lock<std::mutex> write_lock(write_mutex_);
      data_ready_.wait(write_lock,
                       [this]() { return !current_image_.empty(); });
      if (quit_thread_) {
        std::cerr << "Signaled to quit calculation thread";
        return;
      }
      if (cooldown_counter_ < config_.cooldown_duration) {
        std::cerr << "Cooldown over in "
                  << (config_.cooldown_duration - cooldown_counter_) << "..."
                  << std::endl;
        ++cooldown_counter_;
        continue;
      }
      visual_interest = CalculateVisualInterest(current_image_);
      current_image_.clear();
    }
    float average_interest = CalculateMovingAverage(visual_interest);
    std::cerr << "Visual interest is " << visual_interest << "; average is "
              << average_interest << std::endl;

    if (average_interest < config_.visual_interest_threshold) {
      std::cerr << "Average is below threshold; advancing to next preset."
                << std::endl;
      projectm_controller_->TriggerNextPreset();
      ResetMovingAverage();
      cooldown_counter_ = 0;
    }
  }
}

float VisualInterestProcessor::CalculateMovingAverage(float value) {
  if (moving_average_invocations_ <
      config_.moving_average_minimum_invocations) {
    ++moving_average_invocations_;
    return config_.visual_interest_threshold;
  }
  moving_average_ =
      value * (1.0f - config_.alpha) + moving_average_ * config_.alpha;
  return moving_average_;
}

void VisualInterestProcessor::ResetMovingAverage() {
  moving_average_ = config_.visual_interest_threshold;
  moving_average_invocations_ = 0;
}

} // namespace led_driver
