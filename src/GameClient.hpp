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
  void renderSettingsMenu();
  void renderDialogOverlay(const WorldSnapshot& snapshot);

  void handleAuthEvent(const sf::Event& event);
  void handleCharacterSelectEvent(const sf::Event& event);
  void handleCharacterCreateEvent(const sf::Event& event);
  void handleWorldEvent(const sf::Event& event);
  void updateSettingsLayout();
  bool handleSettingsMousePressed(int x, int y);
  bool handleDialogMousePressed(int x, int y);
  void handleSettingsMouseMoved(int x);
  void handleSettingsMouseReleased();
  void applyViewportPreset(unsigned width, unsigned height);
  void setZoomFromSliderX(float x);
  void loadSettings();
  void saveSettings() const;

  void submitAuth();
  void startWorldSession();
  void leaveWorldSession();

  void updateMovement(float dt);
  void updateInterpolations(float dt);
  void updateCombatEffects(float dt);
  void tryAttackNearest();
  void tryInteractNearest();
  void sendDialogSelection(const std::string& npcId, const std::string& responseId);
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
  bool settingsMenuOpen_ = false;
  bool draggingZoomSlider_ = false;
  float settingsZoom_ = 0.75f;
  sf::FloatRect settingsPanelRect_;
  sf::FloatRect viewportPresetA_;
  sf::FloatRect viewportPresetB_;
  sf::FloatRect zoomSliderTrackRect_;
  sf::FloatRect zoomSliderKnobRect_;
  std::vector<sf::FloatRect> dialogOptionRects_;

  sf::Font font_;
  bool fontLoaded_ = false;
};
