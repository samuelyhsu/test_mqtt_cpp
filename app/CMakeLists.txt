set(TARGET_NAME paho_mqtt_cpp_test)
add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE ${PAHO_MQTT_CPP})

set(TARGET_NAME MQTTAsync_publish_time)
add_executable(${TARGET_NAME} ${TARGET_NAME}.c)
target_link_libraries(${TARGET_NAME} PRIVATE ${PAHO_MQTT_C})

set(TARGET_NAME MQTTAsync_publish)
add_executable(${TARGET_NAME} ${TARGET_NAME}.c)
target_link_libraries(${TARGET_NAME} PRIVATE ${PAHO_MQTT_C})

set(TARGET_NAME MQTTAsync_subscribe)
add_executable(${TARGET_NAME} ${TARGET_NAME}.c)
target_link_libraries(${TARGET_NAME} PRIVATE ${PAHO_MQTT_C})

set(TARGET_NAME mqtt_cpp_test)
add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE ${MQTT_CPP} spdlog::spdlog)

set(TARGET_NAME long_lived_client)
add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE ${MQTT_CPP})

set(TARGET_NAME mqtt_cpp_2thread)
add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE ${MQTT_CPP} spdlog::spdlog)
