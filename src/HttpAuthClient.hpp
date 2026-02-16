#pragma once

#include <string>

struct AuthResult {
  bool ok = false;
  std::string token;
  std::string message;
};

class HttpAuthClient {
 public:
  explicit HttpAuthClient(std::string baseUrl);

  AuthResult login(const std::string& username, const std::string& password);
  AuthResult reg(const std::string& username, const std::string& password);

 private:
  AuthResult submit(const std::string& path, const std::string& username, const std::string& password);
  std::string host_;
  int port_ = 80;
};
