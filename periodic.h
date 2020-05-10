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
