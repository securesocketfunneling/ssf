#ifndef SSF_SERVICES_ADMIN_COMMAND_FACTORY_H_
#define SSF_SERVICES_ADMIN_COMMAND_FACTORY_H_

#include <cstdint>

#include <functional>
#include <map>
#include <mutex>
#include <string>

#include <boost/system/error_code.hpp>

namespace ssf {

template <typename Demux>
class CommandFactory {
 public:
  using CommandExecuterType = std::function<std::string(
      const std::string&, Demux*, boost::system::error_code&)>;

  using CommandReplierType =
      std::function<std::string(const std::string&, Demux*,
                                const boost::system::error_code&, std::string)>;

 private:
  using CommandExecuterMap = std::map<uint32_t, CommandExecuterType>;
  using CommandReplierMap = std::map<uint32_t, CommandReplierType>;
  using CommandReplyIndexMap = std::map<uint32_t, uint32_t>;

 public:
  template <class Command>
  bool Register() {
    return Command::RegisterOnReceiveCommand(this) &&
           Command::RegisterOnReplyCommand(this) &&
           Command::RegisterReplyCommandIndex(this);
  }

  bool RegisterOnReceiveCommand(uint32_t index, CommandExecuterType executer) {
    std::unique_lock<std::recursive_mutex> lock(executer_mutex_);
    auto inserted =
        command_executers_.insert(std::make_pair(index, std::move(executer)));
    return inserted.second;
  }

  bool RegisterOnReplyCommand(uint32_t index, CommandReplierType replier) {
    std::unique_lock<std::recursive_mutex> lock(replier_mutex_);
    auto inserted =
        command_repliers_.insert(std::make_pair(index, std::move(replier)));
    return inserted.second;
  }

  bool RegisterReplyCommandIndex(uint32_t index, uint32_t reply_index) {
    std::unique_lock<std::recursive_mutex> lock(command_reply_mutex_);
    auto inserted = command_reply_indexes.insert(
        std::make_pair(index, std::move(reply_index)));
    return inserted.second;
  }

  CommandExecuterType* GetExecuter(uint32_t index) {
    std::unique_lock<std::recursive_mutex> lock(executer_mutex_);

    auto it = command_executers_.find(index);

    if (it != std::end(command_executers_)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

  CommandReplierType* GetReplier(uint32_t index) {
    std::unique_lock<std::recursive_mutex> lock(replier_mutex_);

    auto it = command_repliers_.find(index);

    if (it != std::end(command_repliers_)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

  uint32_t* GetReplyCommandIndex(uint32_t index) {
    std::unique_lock<std::recursive_mutex> lock(command_reply_mutex_);

    auto it = command_reply_indexes.find(index);

    if (it != std::end(command_reply_indexes)) {
      return &(it->second);
    } else {
      return nullptr;
    }
  }

 private:
  std::recursive_mutex executer_mutex_;
  std::recursive_mutex replier_mutex_;
  std::recursive_mutex command_reply_mutex_;
  CommandExecuterMap command_executers_;
  CommandReplierMap command_repliers_;
  CommandReplyIndexMap command_reply_indexes;
};

}  // ssf

#endif  // SSF_SERVICES_ADMIN_COMMAND_FACTORY_H_
