#include "mqtt/client.h"
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

int main(int argc, char *argv[]) {
  const string SERVER_ADDRESS{"tcp://localhost:1883"};
  mqtt::client sub(SERVER_ADDRESS, "");
  mqtt::client pub(SERVER_ADDRESS, "");

  auto connOpts = mqtt::connect_options_builder()
                      .keep_alive_interval(seconds(30))
                      .automatic_reconnect(seconds(2), seconds(30))
                      .clean_session(true)
                      .finalize();

  const vector<string> TOPICS{"test_echo"};
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
      auto start = chrono::steady_clock::now();
      {
        auto msg = mqtt::make_message(TOPICS[0], "hello delay");
        msg->set_qos(0);
        pub.publish(msg);
        printf("time %lld,topic published: %d\n",
               chrono::steady_clock::now().time_since_epoch().count(), i);
      }
      auto msg = sub.consume_message();
      if (msg) {
        printf("time %lld,topic received: %d\n",
               chrono::steady_clock::now().time_since_epoch().count(), i);
        auto end = chrono::steady_clock::now();
        printf("time elapsed %lld ms : %d\n",
               chrono::duration_cast<chrono::milliseconds>(end - start).count(),
               i);
      }
      std::this_thread::sleep_for(1000ms);
    }

    // Disconnect
    cout << "\nDisconnecting from the MQTT server..." << flush;
    sub.disconnect();
    cout << "OK" << endl;
  } catch (const mqtt::exception &exc) {
    cerr << exc.what() << endl;
  }
}
