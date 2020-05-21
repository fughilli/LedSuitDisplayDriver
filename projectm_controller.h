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
