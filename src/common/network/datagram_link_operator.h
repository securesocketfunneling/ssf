#ifndef SSF_COMMON_NETWORK_DATAGRAM_LINK_OPERATOR_H_
#define SSF_COMMON_NETWORK_DATAGRAM_LINK_OPERATOR_H_

#include <map>
#include <memory>
#include <set>

#include <boost/thread/recursive_mutex.hpp>
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
  typedef DatagramLink<RemoteEndpointRight, RightEndSocket,
                       RemoteEndpointLeft, LeftEndSocket> DatagramLinkSpec;
  typedef std::shared_ptr<DatagramLinkSpec> DatagramLinkPtr;

public:
  static std::shared_ptr<DatagramLinkOperator> Create(RightEndSocket& right_end_socket) {
    return std::shared_ptr<DatagramLinkOperator>(new DatagramLinkOperator(right_end_socket));
  }

 ~DatagramLinkOperator() {}

 /// Function called when data is received from the right end socket
 bool Feed(RemoteEndpointRight received_from, boost::asio::const_buffer buffer,
           size_t length) {

   boost::recursive_mutex::scoped_lock lock(links_mutex_);
   
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

   boost::recursive_mutex::scoped_lock lock(links_mutex_);
   boost::recursive_mutex::scoped_lock lock2(endpoints_mutex_);

   auto link = DatagramLinkSpec::Create(
       right_end_socket_, std::move(left), right_endpoint, left_endpoint,
       io_service, this->shared_from_this());
   links_[right_endpoint] = link;
   endpoints_[link] = right_endpoint;
   link->ExternalFeed();
 }

 /// Function to stop a DataLink
 void Stop(DatagramLinkPtr link) {
   link->Stop();

   boost::recursive_mutex::scoped_lock lock(links_mutex_);
   boost::recursive_mutex::scoped_lock lock2(endpoints_mutex_);
   if (endpoints_.count(link)) {
     links_.erase(endpoints_[link]);
     endpoints_.erase(link);
   }
 }

 /// Function to stop all DataLinks
 void StopAll() {
   {
     boost::recursive_mutex::scoped_lock lock(links_mutex_);
     for (auto& link : links_) {
       link.second->Stop();
     }
     links_.clear();
   }

   boost::recursive_mutex::scoped_lock lock2(endpoints_mutex_);
   endpoints_.clear();
 }

private:
  DatagramLinkOperator(RightEndSocket& right_end_socket)
    : right_end_socket_(right_end_socket) {
  }

  boost::recursive_mutex endpoints_mutex_;
  std::map<DatagramLinkPtr, RemoteEndpointRight> endpoints_;
  boost::recursive_mutex links_mutex_;
  std::map<RemoteEndpointRight, DatagramLinkPtr> links_;
  RightEndSocket& right_end_socket_;
};

}  // ssf

#endif  // SSF_COMMON_NETWORK_DATAGRAM_LINK_OPERATOR_H_