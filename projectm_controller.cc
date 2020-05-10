#include "projectm_controller.h"

#include <iostream>

#include "absl/types/span.h"

namespace led_driver {

bool ProjectmController::Initialize() {
  xdo_ = xdo_new(nullptr);
  if (xdo == nullptr) {
    std::cerr << "Connection to X server failed" << std::endl;
    return false;
  }

  xdo_search_t search_params = {0};
  search_params.winname = "projectM";
  search_params.max_depth = 2;
  search_params.searchmask = SEARCH_NAME | SEARCH_ONLYVISIBLE;
  search_params.limit = 0;

  Window *return_windows;
  unsigned int num_return_windows;

  if (xdo_search_windows(xdo, &search_params, &return_windows,
                         &num_return_windows)) {
    std::cerr << "Failed to search windows" << std::endl;
    return false;
  }

  std::cout << "Found " << num_return_windows << " windows" << std::endl;

  projectm_windows_.resize(num_return_windows);
  std::copy(return_windows, return_windows + num_return_windows,
            projectm_windows_.begin());

  return true;
}

bool TriggerNextPreset() {
  for (auto &window : projectm_windows_) {
    std::cerr << "Sending stroke to " << window << std::endl;
    if (xdo_focus_window(xdo, window)) {
      std::cerr << "Failed to focus window";
      return false;
    }
    if (xdo_send_keysequence_window(xdo, window, "n", 12000)) {
      std::cerr << "Failed to send key sequence";
      return false;
    }
    return true;
  }
}

} // namespace led_driver
