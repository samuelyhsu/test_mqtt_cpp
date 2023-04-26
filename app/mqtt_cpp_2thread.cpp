// Copyright Wouter van Kleunen 2020
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "mqtt_client_cpp.hpp"
#include "spdlog/spdlog.h"
#include <chrono>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

constexpr auto _TOPIC = "hello_echo";
constexpr auto _QOS = MQTT_NS::qos::at_most_once;
constexpr auto _HOST = "localhost";
constexpr auto _PORT = 1883;

auto get_ms() {
  return std::chrono::steady_clock::now().time_since_epoch().count() * 1e-6;
}

struct Msg {
  std::string topic;
  std::string payload;
  MQTT_NS::qos qos;
};

using msgs_t = std::vector<Msg>;

msgs_t all_msgs;
std::mutex mutex;

void push_msg(Msg &&msg) {
  std::lock_guard lock(mutex);
  all_msgs.push_back(msg);
}

msgs_t take_all_msgs() {
  msgs_t res;
  {
    std::lock_guard lock(mutex);
    res.swap(all_msgs);
  }
  return res;
}

std::atomic_bool running = true;
std::atomic_int signal_status;
void signal_handler(int signal) {
  running = false;
  signal_status = signal;
}

void run_ioc(boost::asio::io_context *ioc) {
#if 0
  ioc->run();
#else
  while (running) {
    ioc->run_one();
  }
#endif
}

template <typename C>
void reconnect_client(boost::asio::steady_timer &timer, C &c) {
  std::cout << "Start reconnect timer" << std::endl;
  // Set an expiry time relative to now.
  timer.expires_after(std::chrono::seconds(1));
  timer.async_wait([&timer, &c](const boost::system::error_code &error) {
    if (error != boost::asio::error::operation_aborted) {
      std::cout << "Reconnect now !!" << std::endl;

      // Connect
      c->async_connect(
          // [optional] checking underlying layer completion code
          [&timer, &c](MQTT_NS::error_code ec) {
            std::cout << "async_connect callback: " << ec.message()
                      << std::endl;
            if (ec && ec != boost::asio::error::operation_aborted) {
              reconnect_client(timer, c);
            }
          });
    }
  });
}

void sub_thread_entry() {
  static auto log = spdlog::default_logger()->clone("sub");
  boost::asio::io_context ioc;
  boost::asio::steady_timer reconnect_timer(ioc);
  auto c = MQTT_NS::make_async_client(ioc, _HOST, _PORT);
  using packet_id_t =
      typename std::remove_reference_t<decltype(*c)>::packet_id_t;

  c->set_client_id(log->name());
  c->set_keep_alive_sec(10);
  c->set_clean_session(true);

  // Setup handlers
  c->set_connack_handler([&](bool sp,
                             MQTT_NS::connect_return_code connack_return_code) {
    log->info("Session Present: {}", sp);
    log->info("Connack Return Code: {}",
              MQTT_NS::connect_return_code_to_str(connack_return_code));
    c->async_subscribe(c->acquire_unique_packet_id(), {{_TOPIC, _QOS}},
                       // [optional] checking async_subscribe completion code
                       [](MQTT_NS::error_code ec) {
                         log->info("async_subscribe callback: {}",
                                   ec.message());
                       });
    return true;
  });
  c->set_suback_handler([&](packet_id_t packet_id,
                            std::vector<MQTT_NS::suback_return_code> results) {
    log->info("suback received. packet_id: {}", packet_id);
    for (auto const &e : results) {
      log->info("[client] subscribe result: {}", int(e));
    }
    return true;
  });
  c->set_close_handler([]() { log->info("closed."); });

  c->set_error_handler([&](MQTT_NS::error_code ec) {
    log->error("{}", ec.message());
    reconnect_client(reconnect_timer, c);
  });

  c->set_publish_handler([&](MQTT_NS::optional<packet_id_t> packet_id,
                             MQTT_NS::publish_options pubopts,
                             MQTT_NS::buffer topic_name,
                             MQTT_NS::buffer contents) {
    static int cnt;
    log->info("{}, time elapsed : {} ms", ++cnt,
              get_ms() - std::stod(contents.to_string()));
    return true;
  });

  // Connect
  c->async_connect(
      // Initial connect should succeed, otherwise we shutdown
      [&](MQTT_NS::error_code ec) {
        log->info("async_connect callback: {}", ec.message());
        if (ec) {
          reconnect_client(reconnect_timer, c);
        }
      });
  run_ioc(&ioc);
}

template <typename C> void publish_msg(boost::asio::steady_timer &timer, C &c) {
  // Publish a message every 5 seconds
  timer.expires_after(1ms);
  timer.async_wait([&timer, &c](boost::system::error_code const &error) {
    if (error != boost::asio::error::operation_aborted) {
#if 0
      while (1) {
        auto msgs = take_all_msgs();
        if (msgs.empty()) {
          break;
        }
        for (const auto &[topic, payload, qos] : msgs) {
          c->async_publish(topic, std::to_string(get_ms()), qos);
        }
      }
#else
      c->async_publish(_TOPIC, std::to_string(get_ms()), _QOS);
#endif
      publish_msg(timer, c);
    }
  });
}
void pub_thread_entry() {
  static auto log = spdlog::default_logger()->clone("pub");
  boost::asio::io_context ioc;
  boost::asio::steady_timer publish_timer(ioc);
  boost::asio::steady_timer reconnect_timer(ioc);

  auto c = MQTT_NS::make_async_client(ioc, _HOST, _PORT);
  using packet_id_t =
      typename std::remove_reference_t<decltype(*c)>::packet_id_t;

  c->set_client_id(log->name());
  c->set_keep_alive_sec(10);
  c->set_clean_session(true);

  // Setup handlers
  c->set_connack_handler(
      [&](bool sp, MQTT_NS::connect_return_code connack_return_code) {
        log->info("Session Present: {}", sp);
        log->info("Connack Return Code: {}",
                  MQTT_NS::connect_return_code_to_str(connack_return_code));
        publish_msg(publish_timer, c);
        return true;
      });
  c->set_close_handler([]() { log->info("closed."); });
  c->set_error_handler([&](MQTT_NS::error_code ec) {
    log->error("{}", ec.message());
    reconnect_client(reconnect_timer, c);
  });

  // Connect
  c->async_connect(
      // Initial connect should succeed, otherwise we shutdown
      [&](MQTT_NS::error_code ec) {
        log->info("async_connect callback: {}", ec.message());
      });
  run_ioc(&ioc);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_handler);
  std::thread sub_thread(sub_thread_entry);
  std::thread pub_thread(pub_thread_entry);
  sub_thread.join();
  pub_thread.join();
  spdlog::info("quit with {}", signal_status);
  return 0;
}
