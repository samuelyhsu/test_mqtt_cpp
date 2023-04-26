cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

project(ips_gateway)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # /utf-8 (Windows only): Without a byte order mark (BOM), Visual Studio assumes that the source file is encoded using the current user code page, so we specify UTF-8.
  add_compile_options(/utf-8)

  # /W4 (Windows only): Set warning level to 4.
  add_compile_options(/W4)

  # /permissive- (Windows only): Disables a number of compiler extensions that are incompatible with the C++ standard.
  add_compile_options(/permissive-)

  # /wd4201 (Windows only): Disable the warning C4201: nonstandard extension used: nameless struct/union.
  add_compile_options(/wd4201)

  # /wd4200 (Windows only): Disable the warning C4200: nonstandard extension used: zero-sized array in struct/union.
  add_compile_options(/wd4200)

  add_compile_options(/bigobj)

  # NOMINMAX: Do not define the min and max macros, which can cause conflicts.
  add_compile_definitions(NOMINMAX)
else()
  # - Wall: Enable all warnings.
  # - Wextra: Enable extra warnings.
  # - pedantic: Enable strict ISO C and ISO C++ compliance.
  # - Werror: Treat warnings as errors.
  add_compile_options(-Wall -Wextra -pedantic)
endif()

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(PahoMqttCpp REQUIRED)

if(MSVC)
  set(PAHO_MQTT_CPP PahoMqttCpp::paho-mqttpp3)
  set(PAHO_MQTT_C eclipse-paho-mqtt-c::paho-mqtt3a)
else()
  set(PAHO_MQTT_CPP PahoMqttCpp::paho-mqttpp3-static)
  set(PAHO_MQTT_C eclipse-paho-mqtt-c::paho-mqtt3a-static)
endif()

add_compile_definitions(MQTT_STD_VARIANT)
find_package(mqtt_cpp_iface CONFIG REQUIRED)
set(MQTT_CPP mqtt_cpp_iface::mqtt_cpp_iface)

add_subdirectory(app)
