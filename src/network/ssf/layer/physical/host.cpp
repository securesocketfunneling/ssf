#include "ssf/layer/physical/host.h"

#include "ssf/utils/map_helpers.h"

namespace ssf {
namespace layer {
namespace physical {

Host::Host() : addr_(""), port_("") {}

Host::Host(const LayerParameters& parameters)
    : addr_(ssf::helpers::GetField<std::string>("addr", parameters)),
      port_(ssf::helpers::GetField<std::string>("port", parameters)) {}

}  // physical
}  // layer
}  // ssf