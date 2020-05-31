%module DisplayDriver
%include <std_string.i>
%include <std_pair.i>
%{
#include "led_driver/display_driver.h"
%}

namespace led_driver {
class DisplayDriver {
public:
  enum Color;

  DisplayDriver(std::string i2c_dev);
  bool Initialize() const;
  std::pair<int, int> GetSize() const;
  bool DrawPixel(int x, int y, Color color);
  bool Update() const;
};
} // namespace led_driver
