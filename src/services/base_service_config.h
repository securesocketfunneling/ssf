#ifndef SSF_SERVICES_BASE_SERVICE_CONFIG_H_
#define SSF_SERVICES_BASE_SERVICE_CONFIG_H_

namespace ssf {

class BaseServiceConfig {
 public:
  virtual ~BaseServiceConfig();

  inline bool enabled() const { return enabled_; }

  inline void set_enabled(bool enabled) { enabled_ = enabled; }

 protected:
  BaseServiceConfig(bool enabled);

 private:
  bool enabled_;
};

}  // ssf

#endif  // SSF_SERVICES_BASE_SERVICE_CONFIG_H_