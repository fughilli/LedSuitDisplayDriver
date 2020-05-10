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
