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

#ifndef PIXEL_UTILS_H_
#define PIXEL_UTILS_H_

#include <cstdint>

namespace led_driver {

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

void ScalePixelValue(uint8_t *pixel, float scale) {
  if (scale < 0 || scale > 1) {
    return;
  }
  for (int i = 0; i < 3; ++i) {
    pixel[i] = (pixel[i] * scale);
  }
}

void ScalePixelValues(uint8_t *pixels, float scale, ssize_t num_pixels) {
  while (num_pixels--) {
    ScalePixelValue(pixels, scale);
    pixels += 3;
  }
}

} // namespace led_driver

#endif
