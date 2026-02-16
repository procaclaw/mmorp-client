#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

class WebSocketClient {
 public:
  WebSocketClient();
  ~WebSocketClient();

  bool connect(const std::string& url, const std::string& jwt);
  void disconnect();
  bool sendText(const std::string& payload);

  std::vector<std::string> pollMessages();
  bool isConnected() const { return connected_.load(); }
  std::string lastStatus() const;

 private:
  using Client = websocketpp::client<websocketpp::config::asio_client>;

  void setStatus(const std::string& s);

  Client client_;
  websocketpp::connection_hdl hdl_;
  std::thread thread_;

  std::atomic<bool> running_{false};
  std::atomic<bool> connected_{false};
  mutable std::mutex statusMutex_;
  std::string status_ = "Disconnected";

  mutable std::mutex queueMutex_;
  std::deque<std::string> inbound_;
};
