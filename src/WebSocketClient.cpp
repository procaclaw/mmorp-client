#define ASIO_STANDALONE
#define ASIO_NO_BOOST
#include "WebSocketClient.hpp"

#include <utility>

WebSocketClient::WebSocketClient() {
  client_.clear_access_channels(websocketpp::log::alevel::all);
  client_.clear_error_channels(websocketpp::log::elevel::all);
  client_.init_asio();
  client_.start_perpetual();

  client_.set_open_handler([this](websocketpp::connection_hdl hdl) {
    hdl_ = hdl;
    connected_.store(true);
    setStatus("Connected to world socket");
  });

  client_.set_close_handler([this](websocketpp::connection_hdl) {
    connected_.store(false);
    setStatus("World socket closed");
  });

  client_.set_fail_handler([this](websocketpp::connection_hdl hdl) {
    connected_.store(false);
    auto con = client_.get_con_from_hdl(hdl);
    setStatus(std::string("WebSocket fail: ") + con->get_ec().message());
  });

  client_.set_message_handler(
      [this](websocketpp::connection_hdl, Client::message_ptr msg) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        inbound_.push_back(msg->get_payload());
      });
}

WebSocketClient::~WebSocketClient() {
  disconnect();
}

bool WebSocketClient::connect(const std::string& url, const std::string& jwt) {
  disconnect();

  std::string wsUrl = url;
  if (!jwt.empty()) {
    wsUrl += (wsUrl.find('?') == std::string::npos) ? "?" : "&";
    wsUrl += "token=" + jwt;
  }

  websocketpp::lib::error_code ec;
  auto con = client_.get_connection(wsUrl, ec);
  if (ec) {
    setStatus(std::string("Connection build failed: ") + ec.message());
    return false;
  }

  if (!jwt.empty()) {
    con->append_header("Authorization", "Bearer " + jwt);
  }

  client_.connect(con);

  running_.store(true);
  thread_ = std::thread([this]() {
    try {
      client_.run();
    } catch (const std::exception& e) {
      setStatus(std::string("WebSocket exception: ") + e.what());
    }
    running_.store(false);
  });

  return true;
}

void WebSocketClient::disconnect() {
  connected_.store(false);

  if (thread_.joinable()) {
    websocketpp::lib::error_code ec;
    client_.close(hdl_, websocketpp::close::status::normal, "bye", ec);
    client_.stop_perpetual();
    client_.stop();
    thread_.join();

    client_.reset();
    client_.start_perpetual();
  }

  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    inbound_.clear();
  }

  setStatus("Disconnected");
}

bool WebSocketClient::sendText(const std::string& payload) {
  if (!connected_.load()) {
    return false;
  }

  websocketpp::lib::error_code ec;
  client_.send(hdl_, payload, websocketpp::frame::opcode::text, ec);
  if (ec) {
    setStatus(std::string("Send failed: ") + ec.message());
    return false;
  }
  return true;
}

std::vector<std::string> WebSocketClient::pollMessages() {
  std::vector<std::string> out;
  std::lock_guard<std::mutex> lock(queueMutex_);
  out.reserve(inbound_.size());
  while (!inbound_.empty()) {
    out.push_back(std::move(inbound_.front()));
    inbound_.pop_front();
  }
  return out;
}

void WebSocketClient::setStatus(const std::string& s) {
  std::lock_guard<std::mutex> lock(statusMutex_);
  status_ = s;
}

std::string WebSocketClient::lastStatus() const {
  std::lock_guard<std::mutex> lock(statusMutex_);
  return status_;
}
