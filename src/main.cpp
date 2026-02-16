#include "GameClient.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
constexpr const char* kDefaultHttpUrl = "http://localhost:8080";
constexpr const char* kDefaultWsUrl = "ws://localhost:8080/v1/world/ws";

std::string envOrDefault(const char* name, const char* fallback) {
  const char* value = std::getenv(name);
  if (value == nullptr || *value == '\0') {
    return fallback;
  }
  return value;
}
}  // namespace

int main(int argc, char** argv) {
  std::string httpUrl = envOrDefault("MMORP_HTTP_URL", kDefaultHttpUrl);
  std::string wsUrl = envOrDefault("MMORP_WS_URL", kDefaultWsUrl);

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--http-url" && i + 1 < argc) {
      httpUrl = argv[++i];
    } else if (arg.rfind("--http-url=", 0) == 0) {
      httpUrl = arg.substr(std::string("--http-url=").size());
    } else if (arg == "--ws-url" && i + 1 < argc) {
      wsUrl = argv[++i];
    } else if (arg.rfind("--ws-url=", 0) == 0) {
      wsUrl = arg.substr(std::string("--ws-url=").size());
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: mmorp-client [--http-url URL] [--ws-url URL]\n"
                << "Environment fallbacks:\n"
                << "  MMORP_HTTP_URL (default: " << kDefaultHttpUrl << ")\n"
                << "  MMORP_WS_URL   (default: " << kDefaultWsUrl << ")\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      std::cerr << "Use --help for usage.\n";
      return 1;
    }
  }

  GameClient client(httpUrl, wsUrl);
  client.run();
  return 0;
}
