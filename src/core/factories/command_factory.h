#ifndef SSF_CORE_FACTORIES_COMMAND_FACTORY_H_
#define SSF_CORE_FACTORIES_COMMAND_FACTORY_H_

#include <cstdint>

#include <functional>
#include <mutex>
#include <string>
#include <map>

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
  using CommandExecuterType = std::function<std::string(
      boost::archive::text_iarchive&, Demux*, boost::system::error_code&)>;

  using CommandReplierType =
      std::function<std::string(boost::archive::text_iarchive&, Demux*,
                                const boost::system::error_code&, std::string)>;

 private:
  using CommandExecuterMap = std::map<uint32_t, CommandExecuterType>;
  using CommandReplierMap = std::map<uint32_t, CommandReplierType>;
  using CommandReplyIndexMap = std::map<uint32_t, uint32_t>;

 public:
  static bool RegisterOnReceiveCommand(uint32_t index,
                                       CommandExecuterType executer) {
    std::unique_lock<std::recursive_mutex> lock(executer_mutex_);
    auto inserted =
        command_executers_.insert(std::make_pair(index, std::move(executer)));
    return inserted.second;
  }

  static bool RegisterOnReplyCommand(uint32_t index,
                                     CommandReplierType replier) {
    std::unique_lock<std::recursive_mutex> lock(replier_mutex_);
    auto inserted =
        command_repliers_.insert(std::make_pair(index, std::move(replier)));
    return inserted.second;
  }

  static bool RegisterReplyCommandIndex(uint32_t index, uint32_t reply_index) {
    std::unique_lock<std::recursive_mutex> lock(command_reply_mutex_);
    auto inserted = command_reply_indexes.insert(
        std::make_pair(index, std::move(reply_index)));
    return inserted.second;
  }

  static CommandExecuterType* GetExecuter(uint32_t index) {
    std::unique_lock<std::recursive_mutex> lock(executer_mutex_);

    auto it = command_executers_.find(index);

    if (it != std::end(command_executers_)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

  static CommandReplierType* GetReplier(uint32_t index) {
    std::unique_lock<std::recursive_mutex> lock(replier_mutex_);

    auto it = command_repliers_.find(index);

    if (it != std::end(command_repliers_)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

  static uint32_t* GetReplyCommandIndex(uint32_t index) {
    std::unique_lock<std::recursive_mutex> lock(command_reply_mutex_);

    auto it = command_reply_indexes.find(index);

    if (it != std::end(command_reply_indexes)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

 private:
  static std::recursive_mutex executer_mutex_;
  static std::recursive_mutex replier_mutex_;
  static std::recursive_mutex command_reply_mutex_;
  static CommandExecuterMap command_executers_;
  static CommandReplierMap command_repliers_;
  static CommandReplyIndexMap command_reply_indexes;
};

template <typename Demux>
std::recursive_mutex CommandFactory<Demux>::executer_mutex_;
template <typename Demux>
std::recursive_mutex CommandFactory<Demux>::replier_mutex_;
template <typename Demux>
std::recursive_mutex CommandFactory<Demux>::command_reply_mutex_;

template <typename Demux>
typename CommandFactory<Demux>::CommandExecuterMap
    CommandFactory<Demux>::command_executers_;
template <typename Demux>
typename CommandFactory<Demux>::CommandReplierMap
    CommandFactory<Demux>::command_repliers_;
template <typename Demux>
typename CommandFactory<Demux>::CommandReplyIndexMap
    CommandFactory<Demux>::command_reply_indexes;

}  // ssf

#endif  // SSF_CORE_FACTORIES_COMMAND_FACTORY_H_
