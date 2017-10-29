#ifndef SSF_TESTS_TOOLS_H_
#define SSF_TESTS_TOOLS_H_

#include <cstdint>

#include <chrono>

#include "ssf/log/log.h"

class TimedScope {
 public:
  TimedScope() : creation_time_(std::chrono::high_resolution_clock::now()) {}

  virtual ~TimedScope() { PrintDuration(); }

  void ResetTime() {
    creation_time_ = std::chrono::high_resolution_clock::now();
  }

  std::chrono::nanoseconds Duration() const {
    return std::chrono::high_resolution_clock::now() - creation_time_;
  }

  double FloatSecondDuration() const {
    return this->Duration().count() / static_cast<double>(1000) /
           static_cast<double>(1000) / static_cast<double>(1000);
  }

  void PrintDuration() {
    SSF_LOG("test", debug, "Timed scope: {}", Duration().count());
  }

 private:
  std::chrono::time_point<std::chrono::high_resolution_clock> creation_time_;
};

class ScopedBandWidth : public TimedScope {
 public:
  ScopedBandWidth(uint64_t bits) : TimedScope(), bits_(bits) {}

  virtual ~ScopedBandWidth() { PrintBandWidth(); }

  void ResetBits() { bits_ = 0; }

  void ResetBits(uint64_t bits) { bits_ = bits; }

  void AddBits(uint64_t bits) { bits_ += bits; }

  double BandWidth() const { return bits_ / this->FloatSecondDuration(); }

  void PrintBandWidth() const {
    SSF_LOG("test", info,
            "Bandwidth: transferred {} bits in {} seconds ==> {}  Mbits/s",
            bits_, this->FloatSecondDuration(), (BandWidth() / 1000 / 1000));
  }

 private:
  uint64_t bits_;
};

#endif  // SSF_TESTS_TOOLS_H_
