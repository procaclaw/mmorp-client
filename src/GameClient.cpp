#include "GameClient.hpp"

#include <SFML/OpenGL.hpp>

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace {
std::optional<Vec3> parsePosition(const json& j) {
  if (j.contains("position") && j["position"].is_object()) {
    const auto& p = j["position"];
    return Vec3{p.value("x", 0.0f), p.value("y", 0.5f), p.value("z", 0.0f)};
  }
  if (j.contains("x") || j.contains("y") || j.contains("z")) {
    return Vec3{j.value("x", 0.0f), j.value("y", 0.5f), j.value("z", 0.0f)};
  }
  return std::nullopt;
}

std::optional<PlayerState> parsePlayer(const json& j) {
  if (!j.is_object()) {
    return std::nullopt;
  }

  PlayerState player;
  if (j.contains("id") && j["id"].is_string()) {
    player.id = j["id"].get<std::string>();
  } else if (j.contains("playerId") && j["playerId"].is_string()) {
    player.id = j["playerId"].get<std::string>();
  } else if (j.contains("username") && j["username"].is_string()) {
    player.id = j["username"].get<std::string>();
  }

  player.character = j.value("character", j.value("class", "Unknown"));

  const auto pos = parsePosition(j);
  if (pos.has_value()) {
    player.position = pos.value();
  }

  if (player.id.empty()) {
    return std::nullopt;
  }
  return player;
}

std::string maskPassword(const std::string& p) {
  return std::string(p.size(), '*');
}
}  // namespace

GameClient::GameClient(std::string httpUrl, std::string wsUrl)
    : window_(sf::VideoMode(1280, 720), "MMORPG SFML Client", sf::Style::Default,
              sf::ContextSettings(24, 8, 0, 2, 1)),
      authClient_(std::move(httpUrl)),
      wsUrl_(std::move(wsUrl)) {
  window_.setVerticalSyncEnabled(true);

  std::vector<std::string> fontCandidates = {
      "assets/fonts/DejaVuSans.ttf",
      "assets/fonts/FreeSans.ttf",
      "fonts/DejaVuSans.ttf",
      "fonts/FreeSans.ttf",
  };

#if defined(_WIN32)
  const char* windir = std::getenv("WINDIR");
  const std::string windowsDir = (windir != nullptr) ? windir : "C:/Windows";
  fontCandidates.push_back(windowsDir + "/Fonts/segoeui.ttf");
  fontCandidates.push_back(windowsDir + "/Fonts/arial.ttf");
  fontCandidates.push_back(windowsDir + "/Fonts/tahoma.ttf");
#elif defined(__APPLE__)
  fontCandidates.push_back("/System/Library/Fonts/Supplemental/Arial.ttf");
  fontCandidates.push_back("/System/Library/Fonts/Supplemental/Helvetica.ttf");
  fontCandidates.push_back("/Library/Fonts/Arial.ttf");
#else
  fontCandidates.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
  fontCandidates.push_back("/usr/share/fonts/dejavu/DejaVuSans.ttf");
  fontCandidates.push_back("/usr/share/fonts/truetype/freefont/FreeSans.ttf");
#endif

  for (const auto& path : fontCandidates) {
    if (font_.loadFromFile(path)) {
      fontLoaded_ = true;
      break;
    }
  }

  renderer_.initGL();
  renderer_.resize(static_cast<int>(window_.getSize().x), static_cast<int>(window_.getSize().y));

  world_.localPlayer.id = "local";
  world_.localPlayer.character = characters_[selectedCharacterIndex_];
  world_.localPlayer.position = Vec3{0.0f, 0.5f, 0.0f};
}

void GameClient::run() {
  sf::Clock clock;
  while (window_.isOpen()) {
    const float dt = clock.restart().asSeconds();
    processEvents();
    update(dt);
    render();
  }

  leaveWorldSession();
}

void GameClient::processEvents() {
  sf::Event event;
  while (window_.pollEvent(event)) {
    if (event.type == sf::Event::Closed) {
      window_.close();
      continue;
    }

    if (event.type == sf::Event::Resized) {
      renderer_.resize(static_cast<int>(event.size.width), static_cast<int>(event.size.height));
    }

    if (screen_ == ScreenState::Auth) {
      handleAuthEvent(event);
    } else if (screen_ == ScreenState::CharacterSelect) {
      handleCharacterSelectEvent(event);
    } else if (screen_ == ScreenState::World) {
      if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        leaveWorldSession();
        screen_ = ScreenState::Auth;
        statusText_ = "Disconnected from world";
      }
    }
  }
}

void GameClient::handleAuthEvent(const sf::Event& event) {
  if (event.type == sf::Event::KeyPressed) {
    if (event.key.code == sf::Keyboard::Tab) {
      authField_ = (authField_ == AuthField::Username) ? AuthField::Password : AuthField::Username;
    } else if (event.key.code == sf::Keyboard::F1) {
      authMode_ = (authMode_ == AuthMode::Login) ? AuthMode::Register : AuthMode::Login;
      statusText_ = (authMode_ == AuthMode::Login) ? "Mode: Login" : "Mode: Register";
    } else if (event.key.code == sf::Keyboard::Enter) {
      submitAuth();
    } else if (event.key.code == sf::Keyboard::BackSpace) {
      std::string& field = (authField_ == AuthField::Username) ? username_ : password_;
      if (!field.empty()) {
        field.pop_back();
      }
    }
  } else if (event.type == sf::Event::TextEntered) {
    const auto code = event.text.unicode;
    if (code >= 32 && code < 127) {
      std::string& field = (authField_ == AuthField::Username) ? username_ : password_;
      field.push_back(static_cast<char>(code));
    }
  }
}

void GameClient::handleCharacterSelectEvent(const sf::Event& event) {
  if (event.type != sf::Event::KeyPressed) {
    return;
  }

  if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
    if (selectedCharacterIndex_ == 0) {
      selectedCharacterIndex_ = characters_.size() - 1;
    } else {
      --selectedCharacterIndex_;
    }
  } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
    selectedCharacterIndex_ = (selectedCharacterIndex_ + 1) % characters_.size();
  } else if (event.key.code == sf::Keyboard::Enter) {
    startWorldSession();
  }
}

void GameClient::submitAuth() {
  if (username_.empty() || password_.empty()) {
    statusText_ = "Username and password are required";
    return;
  }

  const AuthResult result = (authMode_ == AuthMode::Login)
                                ? authClient_.login(username_, password_)
                                : authClient_.reg(username_, password_);

  if (!result.ok) {
    statusText_ = result.message;
    return;
  }

  jwt_ = result.token;
  world_.localPlayer.id = username_;
  statusText_ = "JWT authenticated. Choose your character.";
  screen_ = ScreenState::CharacterSelect;
}

void GameClient::startWorldSession() {
  world_.localPlayer.character = characters_[selectedCharacterIndex_];
  world_.localPlayer.position = Vec3{0.0f, 0.5f, 0.0f};
  world_.remotePlayers.clear();
  joinSent_ = false;
  moveBroadcastAccumulator_ = 0.0f;

  if (!wsClient_.connect(wsUrl_, jwt_)) {
    statusText_ = wsClient_.lastStatus();
    return;
  }

  statusText_ = "Connecting to world socket...";
  screen_ = ScreenState::World;
}

void GameClient::leaveWorldSession() {
  wsClient_.disconnect();
  joinSent_ = false;
}

void GameClient::update(float dt) {
  if (screen_ != ScreenState::World) {
    return;
  }

  processNetworkMessages();
  updateMovement(dt);
  sendJoinIfNeeded();

  if (!wsClient_.isConnected()) {
    const auto s = wsClient_.lastStatus();
    if (!s.empty()) {
      statusText_ = s;
    }
  }
}

void GameClient::sendJoinIfNeeded() {
  if (joinSent_ || !wsClient_.isConnected()) {
    return;
  }

  json joinMsg{{"type", "join_world"}, {"character", world_.localPlayer.character}};
  wsClient_.sendText(joinMsg.dump());
  joinSent_ = true;
}

void GameClient::updateMovement(float dt) {
  Vec3 dir;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
    dir.z -= 1.0f;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
    dir.z += 1.0f;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
    dir.x -= 1.0f;
  }
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
    dir.x += 1.0f;
  }

  if (length(dir) <= 1e-6f) {
    return;
  }

  dir = normalize(dir);
  world_.localPlayer.position += dir * (4.0f * dt);

  moveBroadcastAccumulator_ += dt;
  if (moveBroadcastAccumulator_ < 0.05f) {
    return;
  }
  moveBroadcastAccumulator_ = 0.0f;

  if (!wsClient_.isConnected()) {
    return;
  }

  json moveMsg{{"type", "move"},
               {"character", world_.localPlayer.character},
               {"position",
                {{"x", world_.localPlayer.position.x},
                 {"y", world_.localPlayer.position.y},
                 {"z", world_.localPlayer.position.z}}}};
  wsClient_.sendText(moveMsg.dump());
}

void GameClient::processNetworkMessages() {
  for (const auto& raw : wsClient_.pollMessages()) {
    try {
      const json msg = json::parse(raw);

      if (msg.contains("selfId") && msg["selfId"].is_string()) {
        world_.localPlayer.id = msg["selfId"].get<std::string>();
      } else if (msg.contains("playerId") && msg["playerId"].is_string() &&
                 msg.value("type", "") == "welcome") {
        world_.localPlayer.id = msg["playerId"].get<std::string>();
      }

      if (msg.contains("players") && msg["players"].is_array()) {
        world_.remotePlayers.clear();
        for (const auto& node : msg["players"]) {
          const auto p = parsePlayer(node);
          if (!p.has_value()) {
            continue;
          }
          if (p->id == world_.localPlayer.id) {
            world_.localPlayer.position = p->position;
            continue;
          }
          world_.remotePlayers[p->id] = p.value();
        }
        continue;
      }

      if (msg.contains("type") && msg["type"].is_string()) {
        const std::string t = msg["type"].get<std::string>();

        if (t == "player_left") {
          const std::string id = msg.value("playerId", "");
          if (!id.empty()) {
            world_.remotePlayers.erase(id);
          }
          continue;
        }

        if (t == "player_update" || t == "player_joined") {
          const auto p = msg.contains("player") ? parsePlayer(msg["player"]) : parsePlayer(msg);
          if (p.has_value()) {
            if (p->id == world_.localPlayer.id) {
              world_.localPlayer.position = p->position;
            } else {
              world_.remotePlayers[p->id] = p.value();
            }
          }
          continue;
        }
      }

      const auto fallbackPlayer = parsePlayer(msg);
      if (fallbackPlayer.has_value()) {
        if (fallbackPlayer->id == world_.localPlayer.id) {
          world_.localPlayer.position = fallbackPlayer->position;
        } else {
          world_.remotePlayers[fallbackPlayer->id] = fallbackPlayer.value();
        }
      }
    } catch (...) {
      statusText_ = "Received invalid JSON from world socket";
    }
  }
}

void GameClient::render() {
  if (screen_ == ScreenState::World) {
    renderWorldScreen();
  } else {
    window_.clear(sf::Color(20, 22, 28));
    if (screen_ == ScreenState::Auth) {
      renderAuthScreen();
    } else {
      renderCharacterSelectScreen();
    }
  }

  window_.display();
}

void GameClient::drawLabel(const std::string& text, float x, float y, unsigned size,
                           const sf::Color& color) {
  if (!fontLoaded_) {
    return;
  }
  sf::Text t;
  t.setFont(font_);
  t.setCharacterSize(size);
  t.setFillColor(color);
  t.setString(text);
  t.setPosition(x, y);
  window_.draw(t);
}

void GameClient::renderAuthScreen() {
  drawLabel("MMORPG Client Login", 40, 32, 34, sf::Color(130, 210, 255));
  drawLabel("F1 toggles Login/Register, Tab switches field, Enter submits", 40, 82, 18,
            sf::Color(180, 185, 200));

  const bool isLogin = authMode_ == AuthMode::Login;
  drawLabel(std::string("Mode: ") + (isLogin ? "Login" : "Register"), 40, 130, 24,
            sf::Color(245, 205, 120));

  sf::RectangleShape userBox(sf::Vector2f(450, 42));
  userBox.setPosition(40, 180);
  userBox.setFillColor(sf::Color(38, 42, 52));
  userBox.setOutlineThickness(2.0f);
  userBox.setOutlineColor((authField_ == AuthField::Username) ? sf::Color::Cyan
                                                               : sf::Color(90, 100, 120));
  window_.draw(userBox);

  sf::RectangleShape passBox(sf::Vector2f(450, 42));
  passBox.setPosition(40, 250);
  passBox.setFillColor(sf::Color(38, 42, 52));
  passBox.setOutlineThickness(2.0f);
  passBox.setOutlineColor((authField_ == AuthField::Password) ? sf::Color::Cyan
                                                               : sf::Color(90, 100, 120));
  window_.draw(passBox);

  drawLabel("Username: " + username_, 52, 190, 20);
  drawLabel("Password: " + maskPassword(password_), 52, 260, 20);
  drawLabel(statusText_, 40, 330, 18, sf::Color(255, 170, 120));
}

void GameClient::renderCharacterSelectScreen() {
  drawLabel("Character Selection", 40, 32, 34, sf::Color(130, 210, 255));
  drawLabel("Left/Right to choose, Enter to join world", 40, 82, 18, sf::Color(180, 185, 200));

  for (std::size_t i = 0; i < characters_.size(); ++i) {
    const bool selected = i == selectedCharacterIndex_;
    drawLabel(characters_[i], 60.0f + static_cast<float>(i) * 220.0f, 180.0f, 30,
              selected ? sf::Color(245, 205, 120) : sf::Color(210, 210, 220));
  }

  drawLabel("Authenticated user: " + username_, 40, 280, 20);
  drawLabel(statusText_, 40, 320, 18, sf::Color(255, 170, 120));
}

void GameClient::renderWorldScreen() {
  renderer_.render(world_);

  window_.pushGLStates();
  drawLabel("World: " + world_.localPlayer.character + "  Pos(" +
                std::to_string(world_.localPlayer.position.x) + ", " +
                std::to_string(world_.localPlayer.position.z) + ")",
            20, 16, 18, sf::Color::White);
  drawLabel("Players visible: " + std::to_string(world_.remotePlayers.size()), 20, 40, 18,
            sf::Color::White);
  drawLabel("WS: " + wsClient_.lastStatus(), 20, 64, 16, sf::Color(220, 225, 230));
  drawLabel("WASD/Arrows move. Esc returns to auth.", 20, 88, 16, sf::Color(200, 200, 210));
  window_.popGLStates();
}
