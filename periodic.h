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

#ifndef PERIODIC_H_
#define PERIODIC_H_

namespace led_driver {

template <typename T> class Periodic {
public:
  Periodic(T period, T start)
      : period_(period), start_(start), next_firing_(start + period) {}

  bool IsDue(T current_time) {
    if (current_time < next_firing_) {
      return false;
    }

    T delta = current_time - next_firing_;
    next_firing_ += ((delta / period_) + 1) * period_;
    return true;
  }

private:
  T period_;
  T start_;
  T next_firing_;
};

} // namespace led_driver

#endif
