#ifndef SSF_COMMON_UTILS_CLEANER_H_
#define SSF_COMMON_UTILS_CLEANER_H_

#include <functional>

class ScopeCleaner {
 private:
  typedef std::function<void()> Handler;

 public:
  ScopeCleaner(Handler handler) : handler_(std::move(handler)) {}
  ~ScopeCleaner() {
    try {
      handler_();
    } catch (...) {
      // Swallows exceptions
    }
  }

 private:
  Handler handler_;
};

#endif  // SSF_COMMON_UTILS_CLEANER_H_
