cmake_minimum_required(VERSION 3.16)
project(mmorp_client LANGUAGES CXX)

# For older dependencies compatibility
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Find SFML manually (for SFML 2.5.x compatibility)
set(SFML_ROOT $ENV{SFML_ROOT})
if(NOT SFML_ROOT)
  set(SFML_ROOT "C:/SFML")
endif()

find_path(SFML_INCLUDE_DIR 
  NAMES SFML/Graphics.hpp
  PATHS ${SFML_ROOT}/include
)

find_library(SFML_GRAPHICS_LIBRARY 
  NAMES sfml-graphics-2.5 sfml-graphics
  PATHS ${SFML_ROOT}/lib
)
find_library(SFML_WINDOW_LIBRARY 
  NAMES sfml-window-2.5 sfml-window
  PATHS ${SFML_ROOT}/lib
)
find_library(SFML_SYSTEM_LIBRARY 
  NAMES sfml-system-2.5 sfml-system
  PATHS ${SFML_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SFML 
  DEFAULT_MSG 
  SFML_INCLUDE_DIR 
  SFML_GRAPHICS_LIBRARY 
  SFML_WINDOW_LIBRARY 
  SFML_SYSTEM_LIBRARY
)

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
  ${SFML_INCLUDE_DIR}
  ${asio_SOURCE_DIR}/asio/include
  ${websocketpp_SOURCE_DIR}
  ${cpp_httplib_SOURCE_DIR}
)

target_compile_definitions(mmorp_client PRIVATE
  ASIO_STANDALONE
)

target_link_libraries(mmorp_client PRIVATE
  ${SFML_GRAPHICS_LIBRARY}
  ${SFML_WINDOW_LIBRARY}
  ${SFML_SYSTEM_LIBRARY}
  OpenGL::GL
  nlohmann_json::nlohmann_json
  Threads::Threads
)
