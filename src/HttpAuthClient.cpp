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

std::vector<CharacterInfo> HttpAuthClient::fetchCharacters(const std::string& jwt) {
  std::vector<CharacterInfo> chars;
  httplib::Client client(host_, port_);
  client.set_connection_timeout(3, 0);
  client.set_read_timeout(5, 0);

  auto res = client.Get("/v1/characters", {{"Authorization", "Bearer " + jwt}});

  if (!res || res->status != 200) {
    return chars;
  }

  try {
    json body = json::parse(res->body);
    if (body.contains("items") && body["items"].is_array()) {
      for (const auto& item : body["items"]) {
        CharacterInfo ci;
        ci.id = item.value("id", "");
        ci.name = item.value("name", "");
        ci.className = item.value("class", "");
        if (!ci.id.empty()) {
          chars.push_back(ci);
        }
      }
    }
  } catch (...) {
    // ignore parse errors
  }
  return chars;
}

std::optional<CharacterInfo> HttpAuthClient::createCharacter(const std::string& jwt, const std::string& name, const std::string& className) {
  httplib::Client client(host_, port_);
  client.set_connection_timeout(3, 0);
  client.set_read_timeout(5, 0);
  client.set_write_timeout(5, 0);

  json request = {{"name", name}, {"class", className}};
  httplib::Headers headers = {{"Authorization", "Bearer " + jwt}};
  auto res = client.Post("/v1/characters", headers, request.dump(), "application/json");

  if (!res || (res->status != 200 && res->status != 201)) {
    return std::nullopt;
  }

  try {
    json body = json::parse(res->body);
    CharacterInfo ci;
    ci.id = body.value("id", "");
    ci.name = body.value("name", "");
    ci.className = body.value("class", "");
    if (!ci.id.empty()) {
      return ci;
    }
  } catch (...) {
    // ignore parse errors
  }
  return std::nullopt;
}
