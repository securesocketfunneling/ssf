#ifndef SSF_LAYER_PHYSICAL_HOST_H_
#define SSF_LAYER_PHYSICAL_HOST_H_

#include <string>

#include "ssf/layer/parameters.h"

namespace ssf {
namespace layer {
namespace physical {

class Host {
 public:
  Host();
  Host(const LayerParameters& parameters);

  inline const std::string& addr() const { return addr_; }

  inline const std::string& port() const { return port_; }

 private:
  std::string addr_;
  std::string port_;
};

}  // physical
}  // layer
}  // ssf

#endif  // SSF_LAYER_PHYSICAL_HOST_H_
