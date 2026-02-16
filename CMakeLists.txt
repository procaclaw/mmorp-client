cmake_minimum_required(VERSION 3.16)
project(mmorp_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-30-2
)

FetchContent_Declare(
  websocketpp
  GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
  GIT_TAG 0.8.2
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_Declare(
  cpp_httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.16.3
)

FetchContent_MakeAvailable(asio websocketpp nlohmann_json cpp_httplib)

add_executable(mmorp_client
  src/main.cpp
  src/GameClient.cpp
  src/Renderer3D.cpp
  src/WebSocketClient.cpp
  src/HttpAuthClient.cpp
)

target_include_directories(mmorp_client PRIVATE
  src
  ${asio_SOURCE_DIR}/asio/include
  ${websocketpp_SOURCE_DIR}
  ${cpp_httplib_SOURCE_DIR}
)

target_compile_definitions(mmorp_client PRIVATE
  ASIO_STANDALONE
)

target_link_libraries(mmorp_client PRIVATE
  SFML::Graphics
  SFML::Window
  SFML::System
  OpenGL::GL
  nlohmann_json::nlohmann_json
  Threads::Threads
)
