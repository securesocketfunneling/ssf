#ifndef SSF_SYSTEM_BASIC_INTERFACES_COLLECTION_H_
#define SSF_SYSTEM_BASIC_INTERFACES_COLLECTION_H_

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

namespace ssf {
namespace system {

class BasicInterfacesCollection {
 public:
  using PropertyTree = boost::property_tree::ptree;
  using MountCallback =
      std::function<void(const boost::system::error_code&, const std::string&)>;

 public:
  virtual ~BasicInterfacesCollection() {}

  virtual std::string GetName() = 0;

  virtual void AsyncMount(boost::asio::io_service& io_service,
                          const PropertyTree& property_tree,
                          MountCallback mount_handler) = 0;

  virtual void RemountDownInterfaces() = 0;

  virtual void Umount(const std::string& interface_name) = 0;

  virtual void UmountAll() = 0;
};

}  // system
}  // ssf

#endif  // SSF_SYSTEM_BASIC_INTERFACES_COLLECTION_H_
