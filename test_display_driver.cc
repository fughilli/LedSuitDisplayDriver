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

#include <iostream>
#include <memory>
#include <vector>

#include "HexapodController2/bus.h"
#include "display_driver.h"
#include "gfx.h"
#include "ugfx/boards/base/framebuffer_cpp_shim/ugfx_framebuffer.h"

namespace led_driver {

extern "C" int main(int argc, char *argv[]) {
  auto bus = std::make_shared<i2c::Bus>("/dev/i2c-1");
  auto display_driver = std::make_shared<DisplayDriver>(bus);
  auto framebuffer = std::make_shared<ugfx::UgfxFramebuffer>(128, 32);
  ugfx::RegisterUgfxFramebuffer(framebuffer);

  std::cout << "Initializing display: " << display_driver->Initialize()
            << std::endl;

  gfxInit();
  font_t font = gdispOpenFont("DejaVuSans10");
  gdispClear(Black);
  gdispDrawLine(10, 10, 25, 25, White);
  gdispDrawString(30, 10, "Hello world", font, White);

  framebuffer->CopyByPixels([&display_driver](int x, int y) {
    return display_driver->DrawPixel(x, y, DisplayDriver::Color::kWhite);
  });
  for (int i = 0; i < 100; ++i) {
    std::cout << "Updating display: " << display_driver->Update() << std::endl;
  }

  return 0;
}

} // namespace led_driver
