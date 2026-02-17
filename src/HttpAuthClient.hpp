#pragma once

#include <string>
#include <vector>

struct AuthResult {
  bool ok = false;
  std::string token;
  std::string message;
};

struct CharacterInfo {
  std::string id;
  std::string name;
  std::string className;
};

class HttpAuthClient {
 public:
  explicit HttpAuthClient(std::string baseUrl);

  AuthResult login(const std::string& username, const std::string& password);
  AuthResult reg(const std::string& username, const std::string& password);
  std::vector<CharacterInfo> fetchCharacters(const std::string& jwt);

 private:
  AuthResult submit(const std::string& path, const std::string& username, const std::string& password);
  std::string host_;
  int port_ = 80;
};
