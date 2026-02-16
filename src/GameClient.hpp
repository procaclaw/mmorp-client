#pragma once

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

#include "HttpAuthClient.hpp"
#include "Renderer3D.hpp"
#include "WebSocketClient.hpp"
#include "WorldState.hpp"

class GameClient {
 public:
  GameClient();
  void run();

 private:
  enum class ScreenState { Auth, CharacterSelect, World };
  enum class AuthMode { Login, Register };
  enum class AuthField { Username, Password };

  void processEvents();
  void update(float dt);
  void render();

  void renderAuthScreen();
  void renderCharacterSelectScreen();
  void renderWorldScreen();

  void handleAuthEvent(const sf::Event& event);
  void handleCharacterSelectEvent(const sf::Event& event);

  void submitAuth();
  void startWorldSession();
  void leaveWorldSession();

  void updateMovement(float dt);
  void processNetworkMessages();
  void sendJoinIfNeeded();

  void drawLabel(const std::string& text, float x, float y, unsigned size = 22,
                 const sf::Color& color = sf::Color::White);

  sf::RenderWindow window_;
  Renderer3D renderer_;
  HttpAuthClient authClient_;
  WebSocketClient wsClient_;

  ScreenState screen_ = ScreenState::Auth;
  AuthMode authMode_ = AuthMode::Login;
  AuthField authField_ = AuthField::Username;

  std::string username_;
  std::string password_;
  std::string jwt_;
  std::string statusText_ = "Enter username and password";

  std::vector<std::string> characters_{"Warrior", "Mage", "Rogue"};
  std::size_t selectedCharacterIndex_ = 0;

  WorldState world_;
  bool joinSent_ = false;
  float moveBroadcastAccumulator_ = 0.0f;

  sf::Font font_;
  bool fontLoaded_ = false;
};
