#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include "HttpAuthClient.hpp"
#include "Renderer3D.hpp"
#include "WebSocketClient.hpp"
#include "WorldState.hpp"

class GameClient {
 public:
  GameClient(std::string httpUrl, std::string wsUrl);
  void run();

 private:
  enum class ScreenState { Auth, CharacterSelect, CharacterCreate, World };
  enum class AuthMode { Login, Register };
  enum class AuthField { Username, Password };

  void processEvents();
  void update(float dt);
  void render();

  void renderAuthScreen();
  void renderCharacterSelectScreen();
  void renderCharacterCreateScreen();
  void renderWorldScreen();

  void handleAuthEvent(const sf::Event& event);
  void handleCharacterSelectEvent(const sf::Event& event);
  void handleCharacterCreateEvent(const sf::Event& event);
  void handleWorldEvent(const sf::Event& event);

  void submitAuth();
  void startWorldSession();
  void leaveWorldSession();

  void updateMovement(float dt);
  void updateInterpolations(float dt);
  void updateCombatEffects(float dt);
  void tryAttackNearest();
  void tryInteractNearest();
  void sendMoveCommand(int dx, int dy);
  void parseAndApplyMessage(const std::string& raw);
  void processNetworkMessages();
  void sendJoinIfNeeded();
  void maybeReconnect(float dt);

  void drawLabel(const std::string& text, float x, float y, unsigned size = 22,
                 const sf::Color& color = sf::Color::White);

  sf::RenderWindow window_;
  Renderer3D renderer_;
  HttpAuthClient authClient_;
  WebSocketClient wsClient_;
  std::string wsUrl_;

  ScreenState screen_ = ScreenState::Auth;
  AuthMode authMode_ = AuthMode::Login;
  AuthField authField_ = AuthField::Username;

  std::string username_;
  std::string password_;
  std::string jwt_;
  std::string statusText_ = "Enter username and password";

  std::vector<CharacterInfo> characters_;
  std::size_t selectedCharacterIndex_ = 0;
  std::string selectedCharacterId_;
  std::string createCharacterName_;
  std::size_t createClassIndex_ = 0;
  std::size_t localCharacterCounter_ = 1;

  WorldState world_;
  bool joinSent_ = false;
  float moveAccumulator_ = 0.0f;
  std::uint64_t lastMoveAtMs_ = 0;
  std::uint64_t lastAttackAtMs_ = 0;
  std::uint64_t lastInteractAtMs_ = 0;
  float reconnectAccumulator_ = 0.0f;
  bool reconnectEnabled_ = true;

  sf::Font font_;
  bool fontLoaded_ = false;
};
