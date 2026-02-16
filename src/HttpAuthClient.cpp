#include "HttpAuthClient.hpp"

#include <sstream>

#include "httplib.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace {
std::pair<std::string, int> parseHostPort(const std::string& baseUrl) {
  std::string stripped = baseUrl;
  constexpr const char* httpPrefix = "http://";
  if (stripped.rfind(httpPrefix, 0) == 0) {
    stripped = stripped.substr(7);
  }

  const auto slash = stripped.find('/');
  if (slash != std::string::npos) {
    stripped = stripped.substr(0, slash);
  }

  const auto colon = stripped.find(':');
  if (colon == std::string::npos) {
    return {stripped, 80};
  }

  int port = 80;
  try {
    port = std::stoi(stripped.substr(colon + 1));
  } catch (...) {
    port = 80;
  }
  return {stripped.substr(0, colon), port};
}

std::string extractToken(const json& body) {
  if (body.contains("token") && body["token"].is_string()) {
    return body["token"].get<std::string>();
  }
  if (body.contains("accessToken") && body["accessToken"].is_string()) {
    return body["accessToken"].get<std::string>();
  }
  if (body.contains("jwt") && body["jwt"].is_string()) {
    return body["jwt"].get<std::string>();
  }
  return {};
}
}  // namespace

HttpAuthClient::HttpAuthClient(std::string baseUrl) {
  auto [host, port] = parseHostPort(baseUrl);
  host_ = host;
  port_ = port;
}

AuthResult HttpAuthClient::login(const std::string& username, const std::string& password) {
  return submit("/v1/auth/login", username, password);
}

AuthResult HttpAuthClient::reg(const std::string& username, const std::string& password) {
  return submit("/v1/auth/register", username, password);
}

AuthResult HttpAuthClient::submit(const std::string& path, const std::string& username,
                                  const std::string& password) {
  httplib::Client client(host_, port_);
  client.set_connection_timeout(3, 0);
  client.set_read_timeout(5, 0);
  client.set_write_timeout(5, 0);

  json request{{"email", username}, {"password", password}};
  auto res = client.Post(path.c_str(), request.dump(), "application/json");

  if (!res) {
    return AuthResult{false, "", "Auth request failed"};
  }

  if (res->status < 200 || res->status >= 300) {
    std::ostringstream oss;
    oss << "Auth error HTTP " << res->status;
    return AuthResult{false, "", oss.str()};
  }

  try {
    json body = json::parse(res->body);
    const std::string token = extractToken(body);
    if (token.empty()) {
      return AuthResult{false, "", "Auth succeeded but token not found"};
    }
    return AuthResult{true, token, "Authenticated"};
  } catch (...) {
    return AuthResult{false, "", "Invalid auth response JSON"};
  }
}
