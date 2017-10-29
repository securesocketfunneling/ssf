#include <gtest/gtest.h>

#include <cstdint>

#include <atomic>
#include <chrono>
#include <future>
#include <functional>
#include <set>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/use_future.hpp>
//#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include "ssf/layer/queue/async_queue.h"
#include "ssf/layer/queue/send_queued_datagram_socket.h"

#include "ssf/log/log.h"

TEST(QueueTest, async_queue_limit_test) {
  boost::asio::io_service io_service;

  typedef ssf::layer::queue::basic_async_queue<
      uint8_t, std::queue<uint8_t>, 1, 1> LimitQueueTest;

  LimitQueueTest queue(io_service);

  boost::system::error_code main_ec;

  auto pushed = [&](const boost::system::error_code& ec) {
    EXPECT_EQ(0, ec.value());
  };
  auto failed_pushed = [&](const boost::system::error_code& ec) {
    EXPECT_NE(0, ec.value());
  };

  auto failed_got = [&](const boost::system::error_code& ec,
                        uint8_t element) { EXPECT_EQ(true, !!ec); };
  auto got3 = [&](const boost::system::error_code& ec, uint8_t element) {
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(0, queue.size());
    queue.push(4, main_ec);
    EXPECT_EQ(0, main_ec.value());
    EXPECT_EQ(1, queue.size());
    queue.clear();
    EXPECT_EQ(0, queue.size());
    queue.close(main_ec);
    EXPECT_EQ(0, main_ec.value());
  };
  auto got2 = [&](const boost::system::error_code& ec, uint8_t element) {
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(0, queue.size());
    queue.async_get(got3);        // op queued because get_op limit not reached
    queue.async_get(failed_got);  // op not queued because get_op limit reached
    queue.push(4, main_ec);
  };
  auto got = [&](const boost::system::error_code& ec, uint8_t element) {
    EXPECT_EQ(0, ec.value());
    queue.async_get(got2);  // op queued because get_op limit not reached
  };

  EXPECT_EQ(0, queue.size());
  queue.get(main_ec);  // fails because the queue is empty
  EXPECT_NE(0, main_ec.value());
  EXPECT_EQ(0, queue.size());
  queue.push(1, main_ec);  // succeed because the queue limit was not reached
  EXPECT_EQ(0, main_ec.value());
  EXPECT_EQ(1, queue.size());
  queue.push(1, main_ec);  // fails because the queue limit was reached
  EXPECT_NE(0, main_ec.value());
  EXPECT_EQ(1, queue.size());
  queue.get(main_ec);  // succeed because the queue is not empty
  EXPECT_EQ(0, main_ec.value());
  EXPECT_EQ(0, queue.size());
  queue.push(2, main_ec);  // succeed because the queue limit was not reached
  EXPECT_EQ(0, main_ec.value());
  EXPECT_EQ(1, queue.size());
  uint8_t push_element3 = 3;
  queue.async_push(push_element3,
                   pushed);  // op queued because push_op limit not reached
  EXPECT_EQ(1, queue.size());
  queue.async_push(
      push_element3,
      failed_pushed);  // op not queued because push_op limit reached
  EXPECT_EQ(1, queue.size());
  queue.async_get(got);  // op queued because get_op limit not reached
  EXPECT_EQ(1, queue.size());

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

TEST(QueueTest, async_queue_clear_test) {
  boost::asio::io_service io_service;

  typedef ssf::layer::queue::basic_async_queue<
      uint8_t, std::queue<uint8_t>, 3, 3> LimitQueueTest;

  LimitQueueTest queue(io_service);

  boost::system::error_code main_ec;

  auto pushed = [&](const boost::system::error_code& ec) {
    EXPECT_EQ(0, ec.value());
  };

  queue.push(1, main_ec);
  queue.push(1, main_ec);
  queue.push(1, main_ec);
  queue.async_push(1, pushed);
  queue.async_push(2, pushed);
  queue.async_push(3, pushed);

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  queue.clear();
  
  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  EXPECT_EQ(3, queue.size());
  queue.clear();
}

TEST(QueueTest, async_queue_close_test) {
  boost::asio::io_service io_service;

  typedef ssf::layer::queue::basic_async_queue<
      uint8_t, std::queue<uint8_t>, 3, 3> LimitQueueTest;

  LimitQueueTest queue(io_service);

  boost::system::error_code main_ec;

  auto pushed = [&](const boost::system::error_code& ec) {
    EXPECT_EQ(true, !!ec);
  };

  queue.push(1, main_ec);
  queue.push(1, main_ec);
  queue.push(1, main_ec);
  queue.async_push(1, pushed);
  queue.async_push(2, pushed);
  queue.async_push(3, pushed);

  std::vector<std::thread> threads;
  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  queue.close(main_ec);
  EXPECT_EQ(0, main_ec.value());

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  EXPECT_EQ(0, queue.size());
  queue.clear();
}

struct EmptyLog {
  EmptyLog() {}

  EmptyLog(uint8_t i) {}

  ~EmptyLog() {}
};

uint8_t nb_of_elements = 3;
uint8_t nb_of_pushing_threads = 2;

TEST(QueueTest, async_queue_future_test) {
  boost::asio::io_service io_service;
  auto p_work = std::unique_ptr<boost::asio::io_service::work>(
      new boost::asio::io_service::work(io_service));

  typedef ssf::layer::queue::basic_async_queue<
      EmptyLog, std::queue<EmptyLog>, 3, 3> LimitQueueTest;

  LimitQueueTest queue(io_service);

  boost::system::error_code main_ec;

  auto pushing_lambda = [&]() {
    try {
      for (uint8_t i = 0; i < nb_of_elements; ++i) {
        auto pushed = queue.async_push(i, boost::asio::use_future);
        pushed.get();
      }
    }
    catch (const std::exception& e) {
      std::cout << "exception: " << e.what() << std::endl;
    }
  };
  auto getting_lambda = [&]() {
    try {
      for (uint8_t i = 0; i < nb_of_elements * nb_of_pushing_threads; ++i) {
        auto got = queue.async_get(boost::asio::use_future);
        auto element = got.get();
      }
      p_work.reset();
    }
    catch (const std::exception& e) {
      std::cout << "exception: " << e.what() << std::endl;
      p_work.reset();
    }
  };

  std::vector<std::thread> threads;
  threads.emplace_back(getting_lambda);

  for (uint8_t i = 0; i < nb_of_pushing_threads; ++i) {
    threads.emplace_back(pushing_lambda);
  }

  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  queue.close(main_ec);
  EXPECT_EQ(0, main_ec.value());

  EXPECT_EQ(0, queue.size());
  queue.clear();
}

//TEST(QueueTest, async_queue_spawn_test) {
//  boost::asio::io_service io_service;
//  auto p_work = std::unique_ptr<boost::asio::io_service::work>(
//      new boost::asio::io_service::work(io_service));
//
//  typedef ssf::layer::queue::basic_async_queue<
//      EmptyLog, std::queue<EmptyLog>, 3, 3> LimitQueueTest;
//
//  LimitQueueTest queue(io_service);
//
//  boost::system::error_code main_ec;
//
//  auto pushing_lambda = [&](boost::asio::yield_context context) {
//    try {
//      for (uint8_t i = 0; i < nb_of_elements; ++i) {
//        queue.async_push(i, context);
//      }
//    }
//    catch (const std::exception& e) {
//      std::cout << "exception: " << e.what() << std::endl;
//    }
//  };
//  auto getting_lambda = [&](boost::asio::yield_context context) {
//    try {
//      for (uint8_t i = 0; i < nb_of_elements * nb_of_pushing_threads; ++i) {
//        auto got = queue.async_get(context);
//      }
//      p_work.reset();
//    }
//    catch (const std::exception& e) {
//      std::cout << "exception: " << e.what() << std::endl;
//      p_work.reset();
//    }
//  };
//
//  boost::asio::spawn(io_service, getting_lambda);
//
//  for (uint8_t i = 0; i < nb_of_pushing_threads; ++i) {
//    boost::asio::spawn(io_service, pushing_lambda);
//  }
//
//  std::vector<std::thread> threads;
//  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
//    threads.emplace_back([&io_service]() { io_service.run(); });
//  }
//
//  for (auto& thread : threads) {
//    if (thread.joinable()) {
//      thread.join();
//    }
//  }
//
//  queue.close(main_ec);
//  EXPECT_EQ(0, main_ec.value());
//
//  EXPECT_EQ(0, queue.size());
//  queue.clear();
//}

TEST(QueueTest, send_queued_datagram_socket) {
  static const uint32_t number_of_senders = 100;
  static const uint32_t number_of_sends = 1000;

  std::array<uint8_t, 512> send_buffer;
  std::array<uint8_t, 512> read_buffer;

  typedef boost::asio::ip::udp::socket UdpSocket;
  typedef ssf::layer::queue::basic_send_queued_datagram_socket<
      UdpSocket, number_of_sends * number_of_senders> BufferedUdpSocket;

  std::atomic<uint32_t> received(0);
  std::atomic<uint32_t> fail_sent(0);
  std::promise<bool> done;
  std::atomic<bool> timeout(false);
  uint32_t max_sending = number_of_sends * number_of_senders;

  boost::asio::io_service io_service;
  boost::asio::steady_timer timeout_timer(io_service);
  std::unique_ptr<boost::asio::io_service::work> p_worker(
    new boost::asio::io_service::work(io_service));
  boost::system::error_code ec;
  boost::asio::ip::udp::resolver resolver(io_service);

  boost::asio::ip::udp::resolver::query query1("127.0.0.1", "10000");
  boost::asio::ip::udp::resolver::iterator iterator1(
      resolver.resolve(query1, ec));
  boost::asio::ip::udp::endpoint endpoint1(*iterator1);

  boost::asio::ip::udp::resolver::query query2("127.0.0.1", "10001");
  boost::asio::ip::udp::resolver::iterator iterator2(
    resolver.resolve(query2, ec));
  boost::asio::ip::udp::endpoint endpoint2(*iterator2);

  UdpSocket receive_socket(io_service, endpoint1);
  receive_socket.connect(endpoint2, ec);
  ASSERT_EQ(0, ec.value()) << "Connect received socket failed";
  BufferedUdpSocket send_socket(io_service, endpoint2);
  send_socket.next_layer().connect(endpoint1, ec);
  ASSERT_EQ(0, ec.value()) << "Connect sent socket failed";

  auto timeout_handler = [&] (const boost::system::error_code& ec) {
    if (!ec && !timeout) {
      timeout = true;
      SSF_LOG("test", info, "Timeout : all packets not received {}/{}",
              received.load(), max_sending);
      done.set_value(true);
    }
  };

  auto sender_lambda = [&]() {
    for (std::size_t i = 0; i < number_of_sends; ++i) {
      send_socket.async_send(
          boost::asio::buffer(send_buffer),
          [&](const boost::system::error_code& ec, std::size_t length) {
        if (ec) {
          ++fail_sent;
        }
      });
    }
  };

  std::function<void(const boost::system::error_code&, std::size_t)>
      receiver_lambda;

  receiver_lambda = [&](const boost::system::error_code& ec, std::size_t length) {
    if (!ec) {
      boost::system::error_code timer_ec;
      timeout_timer.cancel(timer_ec);
      ++received;
      if (received.load() <= max_sending) {
        receive_socket.async_receive(boost::asio::buffer(read_buffer),
                                   receiver_lambda);
        timeout_timer.expires_from_now(std::chrono::seconds(2));
        timeout_timer.async_wait(timeout_handler);
      } else {
        done.set_value(true);
      }
    }
  };

  std::vector<std::thread> threads;

  for (uint16_t i = 1; i <= std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&io_service]() { io_service.run(); });
  }

  threads.emplace_back(std::bind(receiver_lambda, ec, 0));

  for (uint16_t i = 0; i < number_of_senders; ++i) {
    threads.emplace_back(sender_lambda);
  }

  done.get_future().wait();

  send_socket.close();
  receive_socket.close();
  p_worker.reset();

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}
