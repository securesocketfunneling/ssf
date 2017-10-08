#ifndef SSF_IO_HANDLER_HELPER_H_
#define SSF_IO_HANDLER_HELPER_H_

#include <boost/asio/detail/bind_handler.hpp>

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

namespace ssf {
namespace io {

template <class Poster, class Handler, class Arg1>
void PostHandler(Poster& poster, Handler&& handler, Arg1&& arg1) {
  poster.post(boost::asio::detail::bind_handler(std::forward<Handler>(handler),
                                                std::forward<Arg1>(arg1)));
}

template <class Poster, class Handler, class Arg1, class Arg2>
void PostHandler(Poster& poster, Handler&& handler, Arg1&& arg1, Arg2&& arg2) {
  poster.post(boost::asio::detail::bind_handler(std::forward<Handler>(handler),
                                                std::forward<Arg1>(arg1),
                                                std::forward<Arg2>(arg2)));
}

template <class Poster, class Handler, class Arg1, class Arg2, class Arg3>
void PostHandler(Poster& poster, Handler&& handler, Arg1&& arg1, Arg2&& arg2,
                 Arg3&& arg3) {
  poster.post(boost::asio::detail::bind_handler(
      std::forward<Handler>(handler), std::forward<Arg1>(arg1),
      std::forward<Arg2>(arg2), std::forward<Arg3>(arg3)));
}

template <class Poster, class Handler, class Arg1, class Arg2, class Arg3,
          class Arg4>
void PostHandler(Poster& poster, Handler&& handler, Arg1&& arg1, Arg2&& arg2,
                 Arg3&& arg3, Arg4&& arg4) {
  poster.post(boost::asio::detail::bind_handler(
      std::forward<Handler>(handler), std::forward<Arg1>(arg1),
      std::forward<Arg2>(arg2), std::forward<Arg3>(arg3),
      std::forward<Arg4>(arg4)));
}

template <class Poster, class Handler, class Arg1, class Arg2, class Arg3,
          class Arg4, class Arg5>
void PostHandler(Poster& poster, Handler&& handler, Arg1&& arg1, Arg2&& arg2,
                 Arg3&& arg3, Arg4&& arg4, Arg5&& arg5) {
  poster.post(boost::asio::detail::bind_handler(
      std::forward<Handler>(handler), std::forward<Arg1>(arg1),
      std::forward<Arg2>(arg2), std::forward<Arg3>(arg3),
      std::forward<Arg4>(arg4), std::forward<Arg5>(arg5)));
}

}  // io
}  // ssf

#endif  // SSF_IO_HANDLER_HELPER_H_
