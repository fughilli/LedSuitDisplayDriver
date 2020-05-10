#ifndef PROJECTM_CONTROLLER_H_
#define PROJECTM_CONTROLLER_H_

#include <utility>
#include <vector>

extern "C" {
#include "xdo.h"
}

namespace led_driver {

class ProjectmController {
public:
  template <typename... A>
  static std::shared_ptr<ProjectmController> Create(A &&... args) {
    auto projectm_controller = std::shared_ptr<ProjectmController>(
        new ProjectmController(std::forward<A>(args)...));
    if (!projectm_controller->Initialize()) {
      return nullptr;
    }
    return projectm_controller;
  }

  bool TriggerNextPreset();

private:
  ProjectmController() {}

  bool Initialize();

  xdo_t *xdo_;
  std::vector<Window> projectm_windows_;
};

} // namespace led_driver

#endif
