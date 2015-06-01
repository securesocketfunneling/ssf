#ifndef SSF_CORE_FACTORIES_COMMAND_FACTORY_H_
#define SSF_CORE_FACTORIES_COMMAND_FACTORY_H_

#include <cstdint>

#include <functional>
#include <string>
#include <map>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/system/error_code.hpp>


namespace boost {
namespace archive { 
  class text_iarchive;
}  // archive
}  // boost


namespace ssf {
template <typename Demux>
class CommandFactory {
 public:
  typedef std::function<std::string (boost::archive::text_iarchive&, 
                              Demux*,
                              boost::system::error_code&)> CommandExecuterType;

  typedef std::function<std::string (boost::archive::text_iarchive&, 
                              Demux*,
                              const boost::system::error_code&,
                              std::string)> CommandReplierType;
 private:
  typedef std::map<uint32_t, CommandExecuterType> CommandExecuterMap;
  typedef std::map<uint32_t, CommandReplierType> CommandReplierMap;
  typedef std::map<uint32_t, uint32_t> CommandReplyIndexMap;

 public:
  static bool RegisterOnReceiveCommand(uint32_t index, 
                                       CommandExecuterType executer) {
    boost::recursive_mutex::scoped_lock lock(executer_mutex_);
    auto inserted =
      command_executers_.insert(std::make_pair(index, std::move(executer)));
    return inserted.second;
  }

  static bool RegisterOnReplyCommand(uint32_t index,
                                     CommandReplierType replier) {
    boost::recursive_mutex::scoped_lock lock(replier_mutex_);
    auto inserted =
      command_repliers_.insert(std::make_pair(index, std::move(replier)));
    return inserted.second;
  }

  static bool RegisterReplyCommandIndex(uint32_t index,
                                        uint32_t reply_index) {
    boost::recursive_mutex::scoped_lock lock(command_reply_mutex_);
    auto inserted = command_reply_indexes.insert(
        std::make_pair(index, std::move(reply_index)));
    return inserted.second;
  }

  static CommandExecuterType* GetExecuter(uint32_t index) {
    boost::recursive_mutex::scoped_lock lock(executer_mutex_);

    auto it = command_executers_.find(index);

    if (it != std::end(command_executers_)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

  static CommandReplierType* GetReplier(uint32_t index) {
    boost::recursive_mutex::scoped_lock lock(replier_mutex_);

    auto it = command_repliers_.find(index);

    if (it != std::end(command_repliers_)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

  static uint32_t* GetReplyCommandIndex(uint32_t index) {
    boost::recursive_mutex::scoped_lock lock(command_reply_mutex_);

    auto it = command_reply_indexes.find(index);

    if (it != std::end(command_reply_indexes)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

 private:
  static boost::recursive_mutex executer_mutex_;
  static boost::recursive_mutex replier_mutex_;
  static boost::recursive_mutex command_reply_mutex_;
  static CommandExecuterMap command_executers_;
  static CommandReplierMap command_repliers_;
  static CommandReplyIndexMap command_reply_indexes;
};


template <typename Demux>
boost::recursive_mutex CommandFactory<Demux>::executer_mutex_;
template <typename Demux>
boost::recursive_mutex CommandFactory<Demux>::replier_mutex_;
template <typename Demux>
boost::recursive_mutex CommandFactory<Demux>::command_reply_mutex_;

template <typename Demux>
typename CommandFactory<Demux>::CommandExecuterMap CommandFactory<Demux>::command_executers_;
template <typename Demux>
typename CommandFactory<Demux>::CommandReplierMap CommandFactory<Demux>::command_repliers_;
template <typename Demux>
typename CommandFactory<Demux>::CommandReplyIndexMap CommandFactory<Demux>::command_reply_indexes;

}  // ssf
#endif  // SSF_CORE_FACTORIES_COMMAND_FACTORY_H_
