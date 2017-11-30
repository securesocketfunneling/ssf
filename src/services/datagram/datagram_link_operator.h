#ifndef SSF_SERVICES_DATAGRAM_DATAGRAM_LINK_OPERATOR_H_
#define SSF_SERVICES_DATAGRAM_DATAGRAM_LINK_OPERATOR_H_

#include <map>
#include <memory>
#include <mutex>
#include <set>

#include <boost/asio/ip/udp.hpp>

namespace ssf {

template <class RemoteEndpointRight, class RightEndSocket,
          class RemoteEndpointLeft, class LeftEndSocket>
struct DatagramLink;

/// Class to manager DatagramLinks
template <typename RemoteEndpointRight, typename RightEndSocket,
          typename RemoteEndpointLeft, typename LeftEndSocket>
class DatagramLinkOperator
    : public std::enable_shared_from_this<
          DatagramLinkOperator<RemoteEndpointRight, RightEndSocket,
                               RemoteEndpointLeft, LeftEndSocket>> {
 private:
  // Types for DatagramLinks
  using DatagramLinkSpec = DatagramLink<RemoteEndpointRight, RightEndSocket,
                                        RemoteEndpointLeft, LeftEndSocket>;
  using DatagramLinkPtr = std::shared_ptr<DatagramLinkSpec>;

 public:
  static std::shared_ptr<DatagramLinkOperator> Create(
      RightEndSocket& right_end_socket) {
    return std::shared_ptr<DatagramLinkOperator>(
        new DatagramLinkOperator(right_end_socket));
  }

  ~DatagramLinkOperator() {}

  /// Function called when data is received from the right end socket
  bool Feed(RemoteEndpointRight received_from, boost::asio::const_buffer buffer,
            size_t length) {
    std::unique_lock<std::recursive_mutex> lock(links_mutex_);

    // If the endpoint is registered, transfer the data to its DataLink
    if (links_.count(received_from)) {
      links_[received_from]->Feed(buffer, length);
      return true;
    } else {
      return false;
    }
  }

  /// Function to add a new DataLink
  void AddLink(LeftEndSocket left, RemoteEndpointRight right_endpoint,
               RemoteEndpointLeft left_endpoint,
               boost::asio::io_service& io_service) {
    std::unique_lock<std::recursive_mutex> lock(links_mutex_);

    auto link = DatagramLinkSpec::Create(right_end_socket_, std::move(left),
                                         right_endpoint, left_endpoint,
                                         io_service, this->shared_from_this());
    links_[right_endpoint] = link;
    endpoints_[link] = right_endpoint;
    link->ExternalFeed();
  }

  /// Function to stop a DataLink
  void Stop(DatagramLinkPtr link) {
    std::unique_lock<std::recursive_mutex> lock(links_mutex_);
    if (endpoints_.count(link)) {
      links_.erase(endpoints_[link]);
      endpoints_.erase(link);
      link->Stop();
    }
  }

  /// Function to stop all DataLinks
  void StopAll() {
    std::unique_lock<std::recursive_mutex> lock(links_mutex_);
    while (!links_.empty()) {
      auto link_it = links_.begin();
      auto link = link_it->second;
      links_.erase(link_it->first);
      endpoints_.erase(link);
      link->Stop();
    }
  }

 private:
  DatagramLinkOperator(RightEndSocket& right_end_socket)
      : right_end_socket_(right_end_socket) {}
  
  std::recursive_mutex links_mutex_;
  std::map<DatagramLinkPtr, RemoteEndpointRight> endpoints_;
  std::map<RemoteEndpointRight, DatagramLinkPtr> links_;
  RightEndSocket& right_end_socket_;
};

}  // ssf

#endif  // SSF_SERVICES_DATAGRAM_DATAGRAM_LINK_OPERATOR_H_