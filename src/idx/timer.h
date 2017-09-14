#pragma once

#include <chrono>

namespace hana {

class Timer {
private:
  using high_resolution_clock = std::chrono::high_resolution_clock;
  using milliseconds = std::chrono::milliseconds;

public:
  explicit Timer(bool run=false)
  {
    if (run) {
      reset();
    }
  }

  void reset()
  {
    start_ = high_resolution_clock::now();
  }

  double elapsed() const
  {
    using namespace std::chrono;
    auto e = high_resolution_clock::now() - start_;
    return duration_cast<duration<double>>(e).count();
  }

private:
  high_resolution_clock::time_point start_;
};

}
