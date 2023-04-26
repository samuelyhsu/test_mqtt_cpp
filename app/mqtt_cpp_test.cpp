// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <iomanip>
#include <iostream>
#include <map>
#include <math.h>
#include <mqtt_client_cpp.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <thread>
using namespace std::chrono;
constexpr auto _TOPIC = "hello_echo";
constexpr auto _QOS = MQTT_NS::qos::at_most_once;
constexpr auto _HOST = "localhost";
constexpr auto _PORT = 1883;

auto get_ms() {
  return std::chrono::steady_clock::now().time_since_epoch().count() * 1e-6;
}

boost::asio::io_context ioc;
auto c = MQTT_NS::make_sync_client(ioc, _HOST, _PORT);
int count = 0;
double total_latency = 0;
std::vector<double> arr;
auto logger = spdlog::logger(
    "echo", {std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                 "test_echo_cpp.log", true),
             std::make_shared<spdlog::sinks::stdout_color_sink_mt>()});

template <class T> T square(T v) { return v * v; }

void publish(int n) {
  std::string payload = std::to_string(get_ms());
  c->publish(_TOPIC, payload, _QOS);
  logger.debug("time {} ,topic published:{}", payload, n);
}

double stdev(std::vector<double> vec, double m, double n) {
  double sum = 0;
  double variance = 0;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    sum += square(*it - m);
  }
  variance = sqrt(sum / (n));
  return variance;
}

int main(int argc, char **argv) {
  logger.set_level(spdlog::level::debug);
  using packet_id_t =
      typename std::remove_reference_t<decltype(*c)>::packet_id_t;
  // Setup client
  c->set_client_id("cid1");
  c->set_keep_alive_sec(10);
  c->set_clean_session(true);
  // Setup handlers
  boost::asio::steady_timer tim(ioc);
  std::function<void()> reconnect;
  reconnect = [&] {
    // wait 3 seconds and connect again
    tim.expires_after(std::chrono::seconds(3));
    tim.async_wait([&](boost::system::error_code const &ec) {
      if (!ec) {
        // timer fired
        std::cout << "try connect again" << std::endl;
        mqtt::error_code ec;
        c->connect(ec); // connect again
        if (ec) {
          std::cout << "error " << ec.message() << std::endl;
          reconnect();
        }
      }
    });
  };

  c->set_connack_handler([](bool sp,
                            mqtt::connect_return_code connack_return_code) {
    std::cout << "Connack handler called" << std::endl;
    std::cout << "Session Present: " << std::boolalpha << sp << std::endl;
    std::cout << "Connack Return Code: " << connack_return_code << std::endl;
    if (connack_return_code == mqtt::connect_return_code::accepted) {
      c->subscribe(_TOPIC, _QOS);
    }
    return true;
  });
  c->set_suback_handler([&](packet_id_t packet_id,
                            std::vector<MQTT_NS::suback_return_code> results) {
    std::cout << "suback received. packet_id: " << packet_id << std::endl;
    for (auto const &e : results) {
      std::cout << "[client] subscribe result: " << e << std::endl;
    }
    publish(count);
    return true;
  });

  c->set_close_handler([&] {
    std::cout << "connection closed" << std::endl;
    reconnect();
  });
  c->set_error_handler([&](boost::system::error_code const &ec) {
    std::cout << "connection error " << ec.message() << std::endl;
    reconnect();
  });

  c->set_publish_handler([&](MQTT_NS::optional<packet_id_t> packet_id,
                             MQTT_NS::publish_options pubopts,
                             MQTT_NS::buffer topic_name,
                             MQTT_NS::buffer contents) {
    auto now = get_ms();
    auto delay = now - std::stod(contents.data());
    arr.push_back(delay);
    count++;
    total_latency += delay;
    double _mean = total_latency / (count);
    double _std = stdev(arr, _mean, count);
    logger.debug("time {} ,topic recieved:{} , time elapsed {} ms ,time "
                 "elapsed _mean {} ms ,time elapsed std  {}",
                 now, count - 1, delay, _mean, _std);
    publish(count);
    return true;
  });

  c->connect();
  ioc.run();
}
