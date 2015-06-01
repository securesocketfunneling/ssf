#ifndef SSF_SERVICES_ADMIN_ADMIN_COMMAND_H_
#define SSF_SERVICES_ADMIN_ADMIN_COMMAND_H_

#include <cstdint>

#include <string>
#include <memory>
#include <array>
#include <iostream>

#include <boost/asio/buffer.hpp>

namespace ssf { namespace services { namespace admin {

class AdminCommand {
public:
 AdminCommand(uint32_t serial, uint32_t command_id,
              uint32_t serialize_arguments_size, std::string serialized_arguments)
     : serial_(serial),
       command_id_(command_id),
       serialize_arguments_size_(serialize_arguments_size),
       serialized_arguments_(serialized_arguments) {}

  std::array<boost::asio::const_buffer, 4> const_buffers() const {
    std::array<boost::asio::const_buffer, 4> buf =
    {
      {
        boost::asio::buffer(&serial_, sizeof(serial_)),
        boost::asio::buffer(&command_id_,
                            sizeof(command_id_)),
        boost::asio::buffer(&serialize_arguments_size_,
                            sizeof(serialize_arguments_size_)),
        boost::asio::buffer(serialized_arguments_)
      }
    };

    return buf;
  }

private:
  uint32_t serial_;
  uint32_t command_id_;
  uint32_t serialize_arguments_size_;
  std::string serialized_arguments_;
};

}  // admin
}  // services
}  // ssf

#endif  // SSF_SERVICES_ADMIN_ADMIN_COMMAND_H_
