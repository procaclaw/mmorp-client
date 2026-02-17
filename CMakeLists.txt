cmake_minimum_required(VERSION 3.16)
project(mmorp_client LANGUAGES CXX)

# For older dependencies compatibility
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Prefer SFML package config from C:/SFML (or CMAKE_PREFIX_PATH / SFML_ROOT env).
set(SFML_ROOT_DEFAULT "C:/SFML")
if(DEFINED ENV{SFML_ROOT} AND NOT "$ENV{SFML_ROOT}" STREQUAL "")
  set(SFML_ROOT_DEFAULT "$ENV{SFML_ROOT}")
endif()
set(SFML_ROOT "${SFML_ROOT_DEFAULT}" CACHE PATH "Path to SFML root directory")

if(WIN32)
  # Helps find_package(SFML CONFIG) resolve <prefix>/lib/cmake/SFML when
  # users configure with -DCMAKE_PREFIX_PATH=C:/SFML.
  list(APPEND CMAKE_PREFIX_PATH "${SFML_ROOT}")
endif()

find_package(SFML 2.5 COMPONENTS graphics window system CONFIG QUIET)

if(NOT TARGET SFML::Graphics OR NOT TARGET SFML::Window OR NOT TARGET SFML::System)
  # Fallback for layouts without SFMLConfig.cmake.
  find_path(SFML_INCLUDE_DIR
    NAMES SFML/Graphics.hpp
    PATHS "${SFML_ROOT}/include"
  )

  find_library(SFML_GRAPHICS_LIBRARY_RELEASE
    NAMES sfml-graphics-2.5 sfml-graphics
    PATHS "${SFML_ROOT}/lib"
  )
  find_library(SFML_GRAPHICS_LIBRARY_DEBUG
    NAMES sfml-graphics-d-2.5 sfml-graphics-d
    PATHS "${SFML_ROOT}/lib"
  )
  find_library(SFML_WINDOW_LIBRARY_RELEASE
    NAMES sfml-window-2.5 sfml-window
    PATHS "${SFML_ROOT}/lib"
  )
  find_library(SFML_WINDOW_LIBRARY_DEBUG
    NAMES sfml-window-d-2.5 sfml-window-d
    PATHS "${SFML_ROOT}/lib"
  )
  find_library(SFML_SYSTEM_LIBRARY_RELEASE
    NAMES sfml-system-2.5 sfml-system
    PATHS "${SFML_ROOT}/lib"
  )
  find_library(SFML_SYSTEM_LIBRARY_DEBUG
    NAMES sfml-system-d-2.5 sfml-system-d
    PATHS "${SFML_ROOT}/lib"
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SFML
    REQUIRED_VARS
      SFML_INCLUDE_DIR
      SFML_GRAPHICS_LIBRARY_RELEASE
      SFML_WINDOW_LIBRARY_RELEASE
      SFML_SYSTEM_LIBRARY_RELEASE
  )

  if(NOT SFML_GRAPHICS_LIBRARY_DEBUG)
    set(SFML_GRAPHICS_LIBRARY_DEBUG "${SFML_GRAPHICS_LIBRARY_RELEASE}")
  endif()
  if(NOT SFML_WINDOW_LIBRARY_DEBUG)
    set(SFML_WINDOW_LIBRARY_DEBUG "${SFML_WINDOW_LIBRARY_RELEASE}")
  endif()
  if(NOT SFML_SYSTEM_LIBRARY_DEBUG)
    set(SFML_SYSTEM_LIBRARY_DEBUG "${SFML_SYSTEM_LIBRARY_RELEASE}")
  endif()

  add_library(SFML::Graphics UNKNOWN IMPORTED)
  set_target_properties(SFML::Graphics PROPERTIES
    IMPORTED_LOCATION_RELEASE "${SFML_GRAPHICS_LIBRARY_RELEASE}"
    IMPORTED_LOCATION_DEBUG "${SFML_GRAPHICS_LIBRARY_DEBUG}"
    INTERFACE_INCLUDE_DIRECTORIES "${SFML_INCLUDE_DIR}"
  )

  add_library(SFML::Window UNKNOWN IMPORTED)
  set_target_properties(SFML::Window PROPERTIES
    IMPORTED_LOCATION_RELEASE "${SFML_WINDOW_LIBRARY_RELEASE}"
    IMPORTED_LOCATION_DEBUG "${SFML_WINDOW_LIBRARY_DEBUG}"
    INTERFACE_INCLUDE_DIRECTORIES "${SFML_INCLUDE_DIR}"
  )

  add_library(SFML::System UNKNOWN IMPORTED)
  set_target_properties(SFML::System PROPERTIES
    IMPORTED_LOCATION_RELEASE "${SFML_SYSTEM_LIBRARY_RELEASE}"
    IMPORTED_LOCATION_DEBUG "${SFML_SYSTEM_LIBRARY_DEBUG}"
    INTERFACE_INCLUDE_DIRECTORIES "${SFML_INCLUDE_DIR}"
  )
endif()

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# ASIO standalone (no Boost required)
set(ASIO_STANDALONE ON CACHE BOOL "Use standalone Asio")
set(ASIO_NO_BOOST ON CACHE BOOL "Disable Boost support in Asio")

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

# asio + websocketpp are header-only in this project.
# Populate sources only to avoid websocketpp's optional Boost-driven CMake logic.
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
  FetchContent_Populate(asio)
endif()

FetchContent_GetProperties(websocketpp)
if(NOT websocketpp_POPULATED)
  FetchContent_Populate(websocketpp)
endif()

FetchContent_MakeAvailable(nlohmann_json cpp_httplib)

add_executable(mmorp_client
  src/main.cpp
  src/GameClient.cpp
  src/Renderer3D.cpp
  src/SpriteManager.cpp
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
  ASIO_NO_BOOST
  ASIO_NO_SSL
  _WEBSOCKETPP_CPP11_STL_
  _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
)

target_link_libraries(mmorp_client PRIVATE
  SFML::Graphics
  SFML::Window
  SFML::System
  OpenGL::GL
  nlohmann_json::nlohmann_json
  Threads::Threads
)
