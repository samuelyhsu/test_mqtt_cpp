#include "catch.hpp"
#include "mqtt/client.h"
#include "spdlog/spdlog.h"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

TEST_CASE("echo", "[mqtt_cpp]") {
  const string SERVER_ADDRESS{"tcp://localhost:1883"};
  const string CLIENT_ID{"paho_cpp_sync_consume"};
  mqtt::client sub(SERVER_ADDRESS, CLIENT_ID);
  mqtt::client pub(SERVER_ADDRESS, "");

  auto connOpts = mqtt::connect_options_builder()
                      .keep_alive_interval(seconds(30))
                      .automatic_reconnect(seconds(2), seconds(30))
                      .clean_session(true)
                      .finalize();

  const vector<string> TOPICS{"hello_delay_rx"};
  const vector<int> QOS{0};
  try {
    cout << "Connecting to the MQTT server..." << flush;
    pub.connect(connOpts);
    mqtt::connect_response rsp = sub.connect(connOpts);
    cout << "OK\n" << endl;

    if (!rsp.is_session_present()) {
      std::cout << "Subscribing to topics..." << std::flush;
      sub.subscribe(TOPICS, QOS);
      std::cout << "OK" << std::endl;
    } else {
      cout << "Session already present. Skipping subscribe." << std::endl;
    }

    // Consume messages

    for (int i = 0; i < 100; ++i) {
      {
        auto msg = mqtt::make_message("hello_delay_tx", "hello delay");
        msg->set_qos(0);
        pub.publish(msg);
        spdlog::info("topic hello_delay_tx published: {}", i);
      }
      auto msg = sub.consume_message();
      if (msg) {
        spdlog::info("topic hello_delay_rx received: {}", i);
      }
      std::this_thread::sleep_for(50ms);
    }

    // Disconnect
    cout << "\nDisconnecting from the MQTT server..." << flush;
    sub.disconnect();
    cout << "OK" << endl;
  } catch (const mqtt::exception &exc) {
    cerr << exc.what() << endl;
  }
}
