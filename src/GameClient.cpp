#include "GameClient.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/syslimits.h>
#else
#include <unistd.h>
#endif

#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace {
constexpr float kMinZoom = 0.25f;
constexpr float kMaxZoom = 1.0f;

struct ClassArchetype {
  const char* name;
  sf::Color color;
  sf::Color accent;
  const char* description;
  const char* stats;
};

const std::array<ClassArchetype, 3> kClassArchetypes{{
    {"Warrior", sf::Color(210, 72, 72), sf::Color(255, 180, 170), "Frontline fighter with high durability.",
     "HP: High   Attack: Medium   Magic: Low"},
    {"Mage", sf::Color(80, 125, 230), sf::Color(170, 205, 255), "Ranged spellcaster with burst damage.",
     "HP: Low   Attack: High   Magic: Very High"},
    {"Rogue", sf::Color(75, 180, 90), sf::Color(185, 240, 190), "Agile assassin focused on speed and crits.",
     "HP: Medium   Attack: High   Magic: Low"},
}};

std::string toLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

const ClassArchetype& archetypeFromClassName(const std::string& className) {
  const std::string lowered = toLowerCopy(className);
  for (const auto& archetype : kClassArchetypes) {
    if (toLowerCopy(archetype.name) == lowered) {
      return archetype;
    }
  }
  return kClassArchetypes[0];
}

std::string trimName(const std::string& value) {
  const auto begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return "";
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(begin, end - begin + 1);
}

std::string maskPassword(const std::string& p) {
  return std::string(p.size(), '*');
}

bool containsPoint(const sf::FloatRect& rect, sf::Vector2f point) {
  return rect.contains(point);
}

std::filesystem::path settingsFilePath() {
#if defined(_WIN32)
  char buffer[MAX_PATH];
  const DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
  if (len > 0 && len < MAX_PATH) {
    return std::filesystem::path(buffer).parent_path() / "settings.json";
  }
#elif defined(__APPLE__)
  char buffer[PATH_MAX];
  uint32_t size = sizeof(buffer);
  if (_NSGetExecutablePath(buffer, &size) == 0) {
    return std::filesystem::path(buffer).parent_path() / "settings.json";
  }
#else
  char buffer[4096];
  const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
  if (len > 0) {
    buffer[len] = '\0';
    return std::filesystem::path(buffer).parent_path() / "settings.json";
  }
#endif
  return std::filesystem::current_path() / "settings.json";
}

std::optional<std::string> getStringField(const json& j, std::initializer_list<const char*> keys) {
  for (const char* key : keys) {
    if (j.contains(key) && j[key].is_string()) {
      return j[key].get<std::string>();
    }
  }
  return std::nullopt;
}

std::optional<int> getIntField(const json& j, std::initializer_list<const char*> keys) {
  for (const char* key : keys) {
    if (!j.contains(key)) {
      continue;
    }
    const auto& v = j[key];
    if (v.is_number_integer()) {
      return v.get<int>();
    }
    if (v.is_number_float()) {
      return static_cast<int>(std::round(v.get<float>()));
    }
    if (v.is_string()) {
      try {
        return std::stoi(v.get<std::string>());
      } catch (...) {
      }
    }
  }
  return std::nullopt;
}

std::pair<int, int> parsePosition2D(const json& j) {
  if (j.contains("position") && j["position"].is_object()) {
    const auto& p = j["position"];
    const int x = getIntField(p, {"x", "tileX", "col"}).value_or(0);
    const int y = getIntField(p, {"y", "tileY", "row"}).value_or(0);
    return {x, y};
  }
  const int x = getIntField(j, {"x", "tileX", "col"}).value_or(0);
  const int y = getIntField(j, {"y", "tileY", "row"}).value_or(0);
  return {x, y};
}

TileType parseTileType(const json& node) {
  if (node.is_string()) {
    std::string s = node.get<std::string>();
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (s == "grass" || s == "g") {
      return TileType::Grass;
    }
    if (s == "water" || s == "w") {
      return TileType::Water;
    }
    if (s == "wall" || s == "#") {
      return TileType::Wall;
    }
    if (s == "forest" || s == "f") {
      return TileType::Forest;
    }
  } else if (node.is_number_integer()) {
    switch (node.get<int>()) {
      case 1:
        return TileType::Water;
      case 2:
        return TileType::Wall;
      case 3:
        return TileType::Forest;
      default:
        return TileType::Grass;
    }
  }
  return TileType::Grass;
}

PlayerState parsePlayer(const json& j) {
  PlayerState p;
  p.id = getStringField(j, {"id", "playerId", "userId", "username", "name"}).value_or("");
  p.name = getStringField(j, {"name", "username", "displayName"}).value_or(p.id);
  p.className = getStringField(j, {"class", "character", "job"}).value_or("Unknown");
  p.level = getIntField(j, {"level"}).value_or(1);
  p.experience = getIntField(j, {"experience", "xp"}).value_or(0);
  p.hp = getIntField(j, {"hp", "health"}).value_or(100);
  p.maxHp = std::max(1, getIntField(j, {"maxHP", "maxHp", "hpMax", "maxHealth"}).value_or(100));
  p.alive = p.hp > 0;
  const auto [x, y] = parsePosition2D(j);
  p.x = x;
  p.y = y;
  p.renderX = static_cast<float>(x);
  p.renderY = static_cast<float>(y);
  return p;
}

NpcState parseNpc(const json& j) {
  NpcState n;
  n.id = getStringField(j, {"id", "npcId", "name"}).value_or("");
  n.name = getStringField(j, {"name"}).value_or(n.id);
  n.role = getStringField(j, {"role", "npc_role"}).value_or("");
  n.portrait = getStringField(j, {"portrait", "npc_portrait"}).value_or("");
  const auto [x, y] = parsePosition2D(j);
  n.x = x;
  n.y = y;
  n.renderX = static_cast<float>(x);
  n.renderY = static_cast<float>(y);
  return n;
}

std::vector<DialogResponseState> parseDialogResponses(const json& node) {
  std::vector<DialogResponseState> responses;
  if (!node.contains("responses") || !node["responses"].is_array()) {
    return responses;
  }
  responses.reserve(node["responses"].size());
  for (const auto& entry : node["responses"]) {
    if (!entry.is_object()) {
      continue;
    }
    DialogResponseState resp;
    resp.id = getStringField(entry, {"id"}).value_or("");
    resp.text = getStringField(entry, {"text", "label", "name"}).value_or("");
    resp.nextNodeId = getStringField(entry, {"next_node_id", "nextNodeId"}).value_or("");
    resp.questTrigger = getStringField(entry, {"quest_trigger", "questTrigger"}).value_or("");
    if (!resp.id.empty() && !resp.text.empty()) {
      responses.push_back(resp);
    }
  }
  return responses;
}

MobState parseMob(const json& j) {
  MobState m;
  m.id = getStringField(j, {"id", "mobId", "name"}).value_or("");
  m.name = getStringField(j, {"name", "type"}).value_or(m.id);
  m.hp = getIntField(j, {"hp", "health"}).value_or(100);
  m.maxHp = std::max(1, getIntField(j, {"maxHP", "maxHp", "hpMax", "maxHealth"}).value_or(100));
  m.alive = m.hp > 0;
  m.aggressive = j.value("aggressive", j.value("isAggro", false));
  const auto [x, y] = parsePosition2D(j);
  m.x = x;
  m.y = y;
  m.renderX = static_cast<float>(x);
  m.renderY = static_cast<float>(y);
  return m;
}

template <typename T>
void upsertEntity(std::unordered_map<std::string, T>& map, const T& incoming) {
  auto it = map.find(incoming.id);
  if (it == map.end()) {
    map[incoming.id] = incoming;
    return;
  }
  T next = incoming;
  next.renderX = it->second.renderX;
  next.renderY = it->second.renderY;
  it->second = next;
}

void parseTiles(WorldSnapshot& data, const json& mapNode) {
  const int width = getIntField(mapNode, {"width", "w"}).value_or(data.width);
  const int height = getIntField(mapNode, {"height", "h"}).value_or(data.height);
  if (width <= 0 || height <= 0) {
    return;
  }
  data.width = width;
  data.height = height;
  data.tiles.assign(static_cast<std::size_t>(width * height), TileType::Grass);

  if (!mapNode.contains("tiles") || !mapNode["tiles"].is_array()) {
    return;
  }
  const auto& rows = mapNode["tiles"];
  for (int y = 0; y < std::min(height, static_cast<int>(rows.size())); ++y) {
    const auto& row = rows[static_cast<std::size_t>(y)];
    if (row.is_string()) {
      const std::string s = row.get<std::string>();
      for (int x = 0; x < std::min(width, static_cast<int>(s.size())); ++x) {
        data.tiles[static_cast<std::size_t>(y * width + x)] = parseTileType(std::string(1, s[static_cast<std::size_t>(x)]));
      }
      continue;
    }
    if (!row.is_array()) {
      continue;
    }
    for (int x = 0; x < std::min(width, static_cast<int>(row.size())); ++x) {
      data.tiles[static_cast<std::size_t>(y * width + x)] = parseTileType(row[static_cast<std::size_t>(x)]);
    }
  }
}
}  // namespace

GameClient::GameClient(std::string httpUrl, std::string wsUrl)
    : window_(sf::VideoMode(1920, 1080), "MMORPG SFML Client"), authClient_(std::move(httpUrl)),
      wsUrl_(std::move(wsUrl)) {
  window_.setVerticalSyncEnabled(false);
  window_.setFramerateLimit(60);

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
  loadSettings();
  renderer_.resize(static_cast<int>(window_.getSize().x), static_cast<int>(window_.getSize().y));
  settingsZoom_ = renderer_.cameraZoom();
  updateSettingsLayout();
}

void GameClient::run() {
  sf::Clock clock;
  while (window_.isOpen()) {
    const float dt = std::min(0.1f, clock.restart().asSeconds());
    processEvents();
    update(dt);
    render();
  }
  leaveWorldSession();
}

void GameClient::processEvents() {
  sf::Event event{};
  while (window_.pollEvent(event)) {
    if (event.type == sf::Event::Closed) {
      window_.close();
      continue;
    }
    if (event.type == sf::Event::Resized) {
      renderer_.resize(static_cast<int>(event.size.width), static_cast<int>(event.size.height));
      updateSettingsLayout();
    }

    if (screen_ == ScreenState::Auth) {
      handleAuthEvent(event);
    } else if (screen_ == ScreenState::CharacterSelect) {
      handleCharacterSelectEvent(event);
    } else if (screen_ == ScreenState::CharacterCreate) {
      handleCharacterCreateEvent(event);
    } else if (screen_ == ScreenState::World) {
      handleWorldEvent(event);
    }
  }
}

void GameClient::updateSettingsLayout() {
  const sf::Vector2u size = window_.getSize();
  const float panelWidth = 460.0f;
  const float panelHeight = 250.0f;
  const float panelX = (static_cast<float>(size.x) - panelWidth) * 0.5f;
  const float panelY = (static_cast<float>(size.y) - panelHeight) * 0.5f;
  settingsPanelRect_ = sf::FloatRect(panelX, panelY, panelWidth, panelHeight);

  viewportPresetA_ = sf::FloatRect(panelX + 22.0f, panelY + 72.0f, 194.0f, 38.0f);
  viewportPresetB_ = sf::FloatRect(panelX + 244.0f, panelY + 72.0f, 194.0f, 38.0f);
  zoomSliderTrackRect_ = sf::FloatRect(panelX + 28.0f, panelY + 174.0f, panelWidth - 56.0f, 8.0f);

  const float t = (settingsZoom_ - kMinZoom) / (kMaxZoom - kMinZoom);
  const float knobCenterX = zoomSliderTrackRect_.left + std::clamp(t, 0.0f, 1.0f) * zoomSliderTrackRect_.width;
  zoomSliderKnobRect_ = sf::FloatRect(knobCenterX - 8.0f, zoomSliderTrackRect_.top - 6.0f, 16.0f, 20.0f);
}

void GameClient::setZoomFromSliderX(float x) {
  const float normalized =
      (x - zoomSliderTrackRect_.left) / std::max(1.0f, zoomSliderTrackRect_.width);
  const float clamped = std::clamp(normalized, 0.0f, 1.0f);
  const float newZoom = kMinZoom + clamped * (kMaxZoom - kMinZoom);
  if (std::abs(newZoom - settingsZoom_) < 0.0001f) {
    return;
  }
  settingsZoom_ = newZoom;
  renderer_.setCameraZoom(settingsZoom_);
  saveSettings();
  updateSettingsLayout();
}

void GameClient::applyViewportPreset(unsigned width, unsigned height) {
  window_.setSize(sf::Vector2u(width, height));
  renderer_.resize(static_cast<int>(width), static_cast<int>(height));
  updateSettingsLayout();
}

bool GameClient::handleSettingsMousePressed(int x, int y) {
  const sf::Vector2f mouse(static_cast<float>(x), static_cast<float>(y));
  updateSettingsLayout();

  if (containsPoint(viewportPresetA_, mouse)) {
    applyViewportPreset(1920, 1080);
    saveSettings();
    return true;
  }
  if (containsPoint(viewportPresetB_, mouse)) {
    applyViewportPreset(1280, 768);
    saveSettings();
    return true;
  }
  if (containsPoint(zoomSliderTrackRect_, mouse) || containsPoint(zoomSliderKnobRect_, mouse)) {
    draggingZoomSlider_ = true;
    setZoomFromSliderX(mouse.x);
    return true;
  }
  return containsPoint(settingsPanelRect_, mouse);
}

bool GameClient::handleDialogMousePressed(int x, int y) {
  DialogState dialogCopy;
  {
    std::lock_guard<std::mutex> lock(world_.mutex);
    dialogCopy = world_.data.dialog;
  }
  if (!dialogCopy.active || dialogCopy.responses.empty()) {
    return false;
  }
  const sf::Vector2f mouse(static_cast<float>(x), static_cast<float>(y));
  const std::size_t count = std::min(dialogCopy.responses.size(), dialogOptionRects_.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (dialogOptionRects_[i].contains(mouse)) {
      sendDialogSelection(dialogCopy.npcId, dialogCopy.responses[i].id);
      return true;
    }
  }
  return false;
}

void GameClient::handleSettingsMouseMoved(int x) {
  if (!draggingZoomSlider_) {
    return;
  }
  setZoomFromSliderX(static_cast<float>(x));
}

void GameClient::handleSettingsMouseReleased() {
  draggingZoomSlider_ = false;
}

void GameClient::loadSettings() {
  settingsZoom_ = renderer_.cameraZoom();

  const std::filesystem::path path = settingsFilePath();
  std::ifstream in(path);
  if (!in.is_open()) {
    return;
  }

  json config;
  try {
    in >> config;
  } catch (...) {
    return;
  }

  unsigned viewportWidth = window_.getSize().x;
  unsigned viewportHeight = window_.getSize().y;
  if (config.contains("viewport") && config["viewport"].is_object()) {
    const auto& viewport = config["viewport"];
    if (auto width = getIntField(viewport, {"width"}); width && *width > 0) {
      viewportWidth = static_cast<unsigned>(*width);
    }
    if (auto height = getIntField(viewport, {"height"}); height && *height > 0) {
      viewportHeight = static_cast<unsigned>(*height);
    }
  }

  window_.setSize(sf::Vector2u(viewportWidth, viewportHeight));
  renderer_.resize(static_cast<int>(viewportWidth), static_cast<int>(viewportHeight));

  if (config.contains("camera_zoom")) {
    const auto& zoom = config["camera_zoom"];
    if (zoom.is_number()) {
      settingsZoom_ = std::clamp(zoom.get<float>(), kMinZoom, kMaxZoom);
      renderer_.setCameraZoom(settingsZoom_);
    }
  }
}

void GameClient::saveSettings() const {
  const sf::Vector2u viewport = window_.getSize();
  const json config{
      {"viewport", {{"width", viewport.x}, {"height", viewport.y}}},
      {"camera_zoom", renderer_.cameraZoom()},
  };

  std::ofstream out(settingsFilePath(), std::ios::trunc);
  if (!out.is_open()) {
    return;
  }
  out << config.dump(2) << '\n';
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
    return;
  }
  if (event.type == sf::Event::TextEntered) {
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
  const std::size_t optionCount = characters_.size() + 1;
  if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
    selectedCharacterIndex_ = (selectedCharacterIndex_ == 0) ? optionCount - 1 : selectedCharacterIndex_ - 1;
  } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
    selectedCharacterIndex_ = (selectedCharacterIndex_ + 1) % optionCount;
  } else if (event.key.code == sf::Keyboard::Enter) {
    if (selectedCharacterIndex_ == characters_.size()) {
      createCharacterName_.clear();
      createClassIndex_ = 0;
      statusText_ = "Create a new character";
      screen_ = ScreenState::CharacterCreate;
      return;
    }
    selectedCharacterId_ = characters_[selectedCharacterIndex_].id;
    startWorldSession();
  } else if (event.key.code == sf::Keyboard::Escape) {
    screen_ = ScreenState::Auth;
    statusText_ = "Back to login";
  }
}

void GameClient::handleCharacterCreateEvent(const sf::Event& event) {
  if (event.type == sf::Event::KeyPressed) {
    if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
      createClassIndex_ = (createClassIndex_ == 0) ? kClassArchetypes.size() - 1 : createClassIndex_ - 1;
    } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
      createClassIndex_ = (createClassIndex_ + 1) % kClassArchetypes.size();
    } else if (event.key.code == sf::Keyboard::BackSpace) {
      if (!createCharacterName_.empty()) {
        createCharacterName_.pop_back();
      }
    } else if (event.key.code == sf::Keyboard::Enter) {
      const std::string finalName = trimName(createCharacterName_);
      if (finalName.empty()) {
        statusText_ = "Character name is required";
        return;
      }
      const std::string className = kClassArchetypes[createClassIndex_].name;
      auto createdOpt = authClient_.createCharacter(jwt_, finalName, className);
      if (createdOpt) {
        characters_.push_back(*createdOpt);
        selectedCharacterIndex_ = characters_.size() - 1;
        selectedCharacterId_ = createdOpt->id;
        statusText_ = "Character created on server. Press Enter to join.";
      } else {
        statusText_ = "Failed to create character on server. Try again.";
      }
      screen_ = ScreenState::CharacterSelect;
    } else if (event.key.code == sf::Keyboard::Escape) {
      statusText_ = "Character creation canceled";
      screen_ = ScreenState::CharacterSelect;
    }
    return;
  }

  if (event.type == sf::Event::TextEntered) {
    const auto code = event.text.unicode;
    if (code >= 32 && code < 127 && createCharacterName_.size() < 16) {
      createCharacterName_.push_back(static_cast<char>(code));
    }
  }
}

void GameClient::handleWorldEvent(const sf::Event& event) {
  const bool dialogActive = [&]() {
    std::lock_guard<std::mutex> lock(world_.mutex);
    return world_.data.dialog.active;
  }();

  if (event.type == sf::Event::KeyPressed) {
    if (event.key.code == sf::Keyboard::F10) {
      settingsMenuOpen_ = !settingsMenuOpen_;
      draggingZoomSlider_ = false;
      updateSettingsLayout();
      return;
    }
    if (settingsMenuOpen_ && event.key.code == sf::Keyboard::Escape) {
      settingsMenuOpen_ = false;
      draggingZoomSlider_ = false;
      return;
    }
    if (settingsMenuOpen_) {
      return;
    }
    if (event.key.code == sf::Keyboard::Escape) {
      if (dialogActive) {
        world_.pushChat("Choose a response to end the conversation.");
        return;
      }
      leaveWorldSession();
      screen_ = ScreenState::Auth;
      statusText_ = "Disconnected from world";
      return;
    }
    if (dialogActive) {
      return;
    }
    if (event.key.code == sf::Keyboard::Space) {
      tryAttackNearest();
      return;
    }
    if (event.key.code == sf::Keyboard::E) {
      tryInteractNearest();
      return;
    }
  }
  if (settingsMenuOpen_ && event.type == sf::Event::MouseMoved) {
    handleSettingsMouseMoved(event.mouseMove.x);
    return;
  }
  if (settingsMenuOpen_ && event.type == sf::Event::MouseButtonReleased &&
      event.mouseButton.button == sf::Mouse::Left) {
    handleSettingsMouseReleased();
    return;
  }
  if (settingsMenuOpen_ && event.type == sf::Event::MouseButtonPressed &&
      event.mouseButton.button == sf::Mouse::Left) {
    handleSettingsMousePressed(event.mouseButton.x, event.mouseButton.y);
    return;
  }
  if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
    if (dialogActive && handleDialogMousePressed(event.mouseButton.x, event.mouseButton.y)) {
      return;
    }
    if (!dialogActive) {
      // Left click prefers NPC interaction when in range, then falls back to attack.
      const std::uint64_t beforeInteract = lastInteractAtMs_;
      tryInteractNearest();
      if (lastInteractAtMs_ == beforeInteract) {
        tryAttackNearest();
      }
    }
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
  
  // Fetch characters from server
  characters_ = authClient_.fetchCharacters(jwt_);
  selectedCharacterIndex_ = 0;
  selectedCharacterId_ = characters_.empty() ? "" : characters_[0].id;
  statusText_ = characters_.empty() ? "No characters found. Create your first character."
                                    : "Select character or create a new one";
  screen_ = ScreenState::CharacterSelect;
}

void GameClient::startWorldSession() {
  if (characters_.empty() || selectedCharacterIndex_ >= characters_.size()) {
    statusText_ = "Select a character first";
    screen_ = ScreenState::CharacterSelect;
    return;
  }
  leaveWorldSession();

  const CharacterInfo& selected = characters_[selectedCharacterIndex_];
  
  {
    std::lock_guard<std::mutex> lock(world_.mutex);
    world_.data = WorldSnapshot{};
    world_.data.localPlayerId = selected.id;
    PlayerState self;
    self.id = selected.id;
    self.name = selected.name;
    self.className = selected.className;
    world_.data.players[self.id] = self;
  }

  joinSent_ = false;
  moveAccumulator_ = 0.0f;
  reconnectAccumulator_ = 0.0f;
  lastMoveAtMs_ = 0;
  lastAttackAtMs_ = 0;
  lastInteractAtMs_ = 0;
  if (!wsClient_.connect(wsUrl_, jwt_)) {
    statusText_ = wsClient_.lastStatus();
    world_.setConnectionStatus(statusText_, false);
    return;
  }
  std::printf("[client] connecting to %s\n", wsUrl_.c_str());
  statusText_ = "Connecting to world...";
  world_.setConnectionStatus(statusText_, false);
  screen_ = ScreenState::World;
}

void GameClient::leaveWorldSession() {
  wsClient_.disconnect();
  joinSent_ = false;
  reconnectAccumulator_ = 0.0f;
  world_.setConnectionStatus("Disconnected", false);
}

void GameClient::update(float dt) {
  if (screen_ != ScreenState::World) {
    return;
  }

  processNetworkMessages();
  sendJoinIfNeeded();
  updateMovement(dt);
  updateInterpolations(dt);
  updateCombatEffects(dt);
  maybeReconnect(dt);

  if (!wsClient_.isConnected()) {
    world_.setConnectionStatus(wsClient_.lastStatus(), false);
  } else {
    world_.setConnectionStatus("Connected", true);
  }
}

void GameClient::sendJoinIfNeeded() {
  if (joinSent_ || !wsClient_.isConnected()) {
    return;
  }
  const CharacterInfo& selected = characters_[selectedCharacterIndex_];
  json joinMsg{
      {"type", "join"},
      {"character_id", selected.id},
      {"name", selected.name},
      {"class", selected.className},
  };
  wsClient_.sendText(joinMsg.dump());
  joinSent_ = true;
  std::printf("[client] join sent for %s (id: %s)\n", selected.name.c_str(), selected.id.c_str());
}

void GameClient::sendMoveCommand(int dx, int dy) {
  if ((dx == 0 && dy == 0) || !wsClient_.isConnected()) {
    return;
  }
  const std::uint64_t now = WorldState::nowMs();
  if (now - lastMoveAtMs_ < 85) {
    return;
  }
  lastMoveAtMs_ = now;

  {
    std::lock_guard<std::mutex> lock(world_.mutex);
    auto selfIt = world_.data.players.find(world_.data.localPlayerId);
    if (selfIt != world_.data.players.end()) {
      const int nx = selfIt->second.x + dx;
      const int ny = selfIt->second.y + dy;
      const bool valid = nx >= 0 && ny >= 0 && nx < world_.data.width && ny < world_.data.height;
      if (valid) {
        const TileType tile = world_.data.tiles[static_cast<std::size_t>(ny * world_.data.width + nx)];
        if (tile != TileType::Wall && tile != TileType::Water) {
          selfIt->second.x = nx;
          selfIt->second.y = ny;
        }
      }
    }
  }

  json moveMsg{{"type", "move"}, {"dx", dx}, {"dy", dy}};
  wsClient_.sendText(moveMsg.dump());
}

void GameClient::updateMovement(float dt) {
  if (settingsMenuOpen_) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(world_.mutex);
    if (world_.data.dialog.active) {
      return;
    }
  }
  moveAccumulator_ += dt;
  if (moveAccumulator_ < 0.09f) {
    return;
  }
  moveAccumulator_ = 0.0f;

  int dx = 0;
  int dy = 0;
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
    dy = -1;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
    dy = 1;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
    dx = -1;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
    dx = 1;
  }
  sendMoveCommand(dx, dy);
}

void GameClient::updateInterpolations(float dt) {
  const float alpha = std::min(1.0f, dt * 12.0f);
  std::lock_guard<std::mutex> lock(world_.mutex);
  for (auto& [_, p] : world_.data.players) {
    p.renderX += (static_cast<float>(p.x) - p.renderX) * alpha;
    p.renderY += (static_cast<float>(p.y) - p.renderY) * alpha;
  }
  for (auto& [_, n] : world_.data.npcs) {
    n.renderX += (static_cast<float>(n.x) - n.renderX) * alpha;
    n.renderY += (static_cast<float>(n.y) - n.renderY) * alpha;
  }
  for (auto& [_, m] : world_.data.mobs) {
    m.renderX += (static_cast<float>(m.x) - m.renderX) * alpha;
    m.renderY += (static_cast<float>(m.y) - m.renderY) * alpha;
  }
}

void GameClient::updateCombatEffects(float dt) {
  std::lock_guard<std::mutex> lock(world_.mutex);
  for (auto& fx : world_.data.combatTexts) {
    fx.ttl -= dt;
  }
  while (!world_.data.combatTexts.empty() && world_.data.combatTexts.front().ttl <= 0.0f) {
    world_.data.combatTexts.pop_front();
  }
}

void GameClient::tryAttackNearest() {
  if (!wsClient_.isConnected()) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(world_.mutex);
    if (world_.data.dialog.active) {
      return;
    }
  }
  const std::uint64_t now = WorldState::nowMs();
  if (now - lastAttackAtMs_ < 220) {
    return;
  }
  lastAttackAtMs_ = now;

  std::string targetMobId;
  int selfX = 0;
  int selfY = 0;
  int bestDist = std::numeric_limits<int>::max();
  {
    std::lock_guard<std::mutex> lock(world_.mutex);
    const auto selfIt = world_.data.players.find(world_.data.localPlayerId);
    if (selfIt == world_.data.players.end()) {
      return;
    }
    selfX = selfIt->second.x;
    selfY = selfIt->second.y;
    for (const auto& [id, mob] : world_.data.mobs) {
      if (!mob.alive) {
        continue;
      }
      const int dist = std::abs(mob.x - selfX) + std::abs(mob.y - selfY);
      if (dist < bestDist) {
        bestDist = dist;
        targetMobId = id;
      }
    }
  }

  if (targetMobId.empty() || bestDist > 2) {
    return;
  }

  json attackMsg{{"type", "attack"}, {"targetId", targetMobId}, {"mobId", targetMobId}};
  wsClient_.sendText(attackMsg.dump());
}

void GameClient::tryInteractNearest() {
  if (!wsClient_.isConnected()) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(world_.mutex);
    if (world_.data.dialog.active) {
      return;
    }
  }
  const std::uint64_t now = WorldState::nowMs();
  if (now - lastInteractAtMs_ < 1000) {
    return;
  }

  std::string targetNpcId;
  int selfX = 0;
  int selfY = 0;
  int bestDist = std::numeric_limits<int>::max();
  {
    std::lock_guard<std::mutex> lock(world_.mutex);
    const auto selfIt = world_.data.players.find(world_.data.localPlayerId);
    if (selfIt == world_.data.players.end()) {
      return;
    }
    selfX = selfIt->second.x;
    selfY = selfIt->second.y;
    for (const auto& [id, npc] : world_.data.npcs) {
      const int dist = std::abs(npc.x - selfX) + std::abs(npc.y - selfY);
      if (dist < bestDist) {
        bestDist = dist;
        targetNpcId = id;
      }
    }
  }

  if (targetNpcId.empty() || bestDist > 2) {
    return;
  }

  lastInteractAtMs_ = now;
  json interactMsg{{"type", "interact"}, {"npcId", targetNpcId}, {"action", "talk"}};
  wsClient_.sendText(interactMsg.dump());
}

void GameClient::sendDialogSelection(const std::string& npcId, const std::string& responseId) {
  if (!wsClient_.isConnected() || npcId.empty() || responseId.empty()) {
    return;
  }
  json selectMsg{{"type", "dialog_select"}, {"npcId", npcId}, {"response_id", responseId}};
  wsClient_.sendText(selectMsg.dump());
}

void GameClient::parseAndApplyMessage(const std::string& raw) {
  try {
    const json msg = json::parse(raw);
    const std::string type = msg.value("type", "");

    const auto text = getStringField(msg, {"message", "text", "error"});
    if (type == "error") {
      world_.pushError(text.value_or("Server error"));
      return;
    }

    if (type == "welcome") {
      {
        std::lock_guard<std::mutex> lock(world_.mutex);
        auto& data = world_.data;
        data.worldReady = true;
        data.lastServerUpdateMs = WorldState::nowMs();

        if (const auto selfId = getStringField(msg, {"selfId", "playerId", "id"}); selfId.has_value()) {
          data.localPlayerId = selfId.value();
        }

        if (msg.contains("world") && msg["world"].is_object()) {
          const auto& w = msg["world"];
          if (w.contains("map") && w["map"].is_object()) {
            parseTiles(data, w["map"]);
          } else if (w.contains("tiles")) {
            parseTiles(data, w);
          }
          if (w.contains("players") && w["players"].is_array()) {
            data.players.clear();
            for (const auto& node : w["players"]) {
              PlayerState p = parsePlayer(node);
              if (!p.id.empty()) {
                upsertEntity(data.players, p);
              }
            }
          }
          if (w.contains("npcs") && w["npcs"].is_array()) {
            data.npcs.clear();
            for (const auto& node : w["npcs"]) {
              NpcState n = parseNpc(node);
              if (!n.id.empty()) {
                upsertEntity(data.npcs, n);
              }
            }
          }
          if (w.contains("mobs") && w["mobs"].is_array()) {
            data.mobs.clear();
            for (const auto& node : w["mobs"]) {
              MobState m = parseMob(node);
              if (!m.id.empty()) {
                upsertEntity(data.mobs, m);
              }
            }
          }
        }

        if (msg.contains("map") && msg["map"].is_object()) {
          parseTiles(data, msg["map"]);
        }
        if (msg.contains("players") && msg["players"].is_array()) {
          data.players.clear();
          for (const auto& node : msg["players"]) {
            PlayerState p = parsePlayer(node);
            if (!p.id.empty()) {
              upsertEntity(data.players, p);
            }
          }
        }
        if (msg.contains("npcs") && msg["npcs"].is_array()) {
          data.npcs.clear();
          for (const auto& node : msg["npcs"]) {
            NpcState n = parseNpc(node);
            if (!n.id.empty()) {
              upsertEntity(data.npcs, n);
            }
          }
        }
        if (msg.contains("mobs") && msg["mobs"].is_array()) {
          data.mobs.clear();
          for (const auto& node : msg["mobs"]) {
            MobState m = parseMob(node);
            if (!m.id.empty()) {
              upsertEntity(data.mobs, m);
            }
          }
        }

        if (data.players.find(data.localPlayerId) == data.players.end()) {
          PlayerState self;
          self.id = data.localPlayerId.empty() ? (characters_.empty() ? username_ : characters_[selectedCharacterIndex_].id) : data.localPlayerId;
          self.name = characters_.empty() ? username_ : characters_[selectedCharacterIndex_].name;
          self.className = characters_.empty() ? "unknown" : characters_[selectedCharacterIndex_].className;
          upsertEntity(data.players, self);
        }

        // Override self position from server-provided character data in welcome message
        if (msg.contains("character") && msg["character"].is_object()) {
          const auto& charObj = msg["character"];
          if (auto selfIt = data.players.find(data.localPlayerId); selfIt != data.players.end()) {
            if (auto posX = getIntField(charObj, {"pos_x", "x"}); posX.has_value()) {
              selfIt->second.x = *posX;
              selfIt->second.renderX = static_cast<float>(*posX);
            }
            if (auto posY = getIntField(charObj, {"pos_y", "y"}); posY.has_value()) {
              selfIt->second.y = *posY;
              selfIt->second.renderY = static_cast<float>(*posY);
            }
          }
        }
      }
      world_.pushChat("Joined world");
      return;
    }

    if (type == "player_joined" || type == "player_moved" || type == "player_update") {
      const json& playerNode = (msg.contains("player") && msg["player"].is_object()) ? msg["player"] : msg;
      PlayerState p = parsePlayer(playerNode);
      if (p.id.empty()) {
        return;
      }
      std::lock_guard<std::mutex> lock(world_.mutex);
      upsertEntity(world_.data.players, p);
      if (type == "player_joined") {
        world_.data.chatLines.push_back(ChatLine{p.name + " joined the world", WorldState::nowMs()});
        while (world_.data.chatLines.size() > 12) {
          world_.data.chatLines.pop_front();
        }
      }
      return;
    }

    if (type == "player_left") {
      const std::string id = getStringField(msg, {"playerId", "id"}).value_or("");
      if (id.empty()) {
        return;
      }
      std::lock_guard<std::mutex> lock(world_.mutex);
      world_.data.players.erase(id);
      world_.data.chatLines.push_back(ChatLine{id + " left the world", WorldState::nowMs()});
      while (world_.data.chatLines.size() > 12) {
        world_.data.chatLines.pop_front();
      }
      return;
    }

    if (type == "mob_update") {
      std::lock_guard<std::mutex> lock(world_.mutex);
      if (msg.contains("mobs") && msg["mobs"].is_array()) {
        for (const auto& node : msg["mobs"]) {
          MobState mob = parseMob(node);
          if (!mob.id.empty()) {
            upsertEntity(world_.data.mobs, mob);
          }
        }
      } else {
        const json& mobNode = (msg.contains("mob") && msg["mob"].is_object()) ? msg["mob"] : msg;
        MobState mob = parseMob(mobNode);
        if (!mob.id.empty()) {
          upsertEntity(world_.data.mobs, mob);
        }
      }
      return;
    }

    if (type == "combat") {
      std::string targetId = getStringField(msg, {"targetId", "mobId", "victimId"}).value_or("");
      const int damage = getIntField(msg, {"damage", "amount"}).value_or(0);
      std::lock_guard<std::mutex> lock(world_.mutex);
      float fxX = 0.0f;
      float fxY = 0.0f;
      if (auto it = world_.data.mobs.find(targetId); it != world_.data.mobs.end()) {
        it->second.hp = std::max(0, it->second.hp - damage);
        it->second.alive = it->second.hp > 0;
        fxX = static_cast<float>(it->second.x);
        fxY = static_cast<float>(it->second.y);
      } else if (auto pit = world_.data.players.find(targetId); pit != world_.data.players.end()) {
        pit->second.hp = std::max(0, pit->second.hp - damage);
        pit->second.alive = pit->second.hp > 0;
        fxX = static_cast<float>(pit->second.x);
        fxY = static_cast<float>(pit->second.y);
      }
      FloatingCombatText fx;
      fx.text = "-" + std::to_string(damage);
      fx.worldX = fxX;
      fx.worldY = fxY;
      fx.r = 255;
      fx.g = 80;
      fx.b = 80;
      world_.data.combatTexts.push_back(fx);
      while (world_.data.combatTexts.size() > 32) {
        world_.data.combatTexts.pop_front();
      }
      if (damage > 0) {
        world_.data.chatLines.push_back(ChatLine{"Combat: " + std::to_string(damage) + " damage", WorldState::nowMs()});
        while (world_.data.chatLines.size() > 12) {
          world_.data.chatLines.pop_front();
        }
      }
      return;
    }

    if (type == "player_died") {
      const std::string id = getStringField(msg, {"playerId", "id"}).value_or("");
      if (id.empty()) {
        return;
      }
      std::lock_guard<std::mutex> lock(world_.mutex);
      auto it = world_.data.players.find(id);
      if (it != world_.data.players.end()) {
        it->second.hp = 0;
        it->second.alive = false;
      }
      world_.data.chatLines.push_back(ChatLine{id + " died", WorldState::nowMs()});
      while (world_.data.chatLines.size() > 12) {
        world_.data.chatLines.pop_front();
      }
      return;
    }

    if (type == "dialog_start" || type == "dialog_update") {
      const json& node = (msg.contains("node") && msg["node"].is_object()) ? msg["node"] : msg;
      DialogState dialog;
      dialog.active = true;
      dialog.npcId = getStringField(msg, {"npc_id", "npcId"}).value_or("");
      dialog.npcName = getStringField(msg, {"npc_name", "npcName"}).value_or(dialog.npcId);
      dialog.npcRole = getStringField(msg, {"npc_role", "npcRole"}).value_or("");
      dialog.npcPortrait = getStringField(msg, {"npc_portrait", "npcPortrait"}).value_or("");
      dialog.nodeId = getStringField(node, {"id", "node_id"}).value_or("");
      dialog.text = getStringField(node, {"text", "message"}).value_or("");
      dialog.responses = parseDialogResponses(node);

      {
        std::lock_guard<std::mutex> lock(world_.mutex);
        // Fill missing metadata from world snapshot for stable UI.
        if (auto npcIt = world_.data.npcs.find(dialog.npcId); npcIt != world_.data.npcs.end()) {
          if (dialog.npcName.empty()) {
            dialog.npcName = npcIt->second.name;
          }
          if (dialog.npcRole.empty()) {
            dialog.npcRole = npcIt->second.role;
          }
          if (dialog.npcPortrait.empty()) {
            dialog.npcPortrait = npcIt->second.portrait;
          }
        }
        world_.data.dialog = dialog;
      }
      if (const auto questTrigger = getStringField(msg, {"quest_trigger", "questTrigger"}); questTrigger && !questTrigger->empty()) {
        world_.pushChat("Quest triggered: " + *questTrigger);
      }
      return;
    }

    if (type == "dialog_end") {
      std::string questTrigger = getStringField(msg, {"quest_trigger", "questTrigger"}).value_or("");
      std::string npcName = getStringField(msg, {"npc_name", "npcName"}).value_or("");
      if (npcName.empty()) {
        const std::string npcId = getStringField(msg, {"npc_id", "npcId"}).value_or("");
        std::lock_guard<std::mutex> lock(world_.mutex);
        if (auto it = world_.data.npcs.find(npcId); it != world_.data.npcs.end()) {
          npcName = it->second.name;
        }
      }
      {
        std::lock_guard<std::mutex> lock(world_.mutex);
        world_.data.dialog = DialogState{};
      }
      if (!npcName.empty()) {
        world_.pushChat("Conversation ended with " + npcName);
      }
      if (!questTrigger.empty()) {
        world_.pushChat("Quest triggered: " + questTrigger);
      }
      return;
    }

    if (type == "npc_response") {
      std::string npcId = getStringField(msg, {"npcId", "id"}).value_or("");
      std::string npcText;
      std::vector<std::string> optionLabels;

      if (msg.contains("result") && msg["result"].is_object()) {
        const auto& result = msg["result"];
        if (result.contains("text") && result["text"].is_string()) {
          npcText = result["text"].get<std::string>();
        }
        if (result.contains("options") && result["options"].is_array()) {
          for (const auto& option : result["options"]) {
            if (option.is_string()) {
              optionLabels.push_back(option.get<std::string>());
              continue;
            }
            if (option.is_object()) {
              const auto label = getStringField(option, {"label", "text", "name", "id"});
              if (label.has_value() && !label->empty()) {
                optionLabels.push_back(label.value());
              }
            }
          }
        }
      }

      if (npcText.empty()) {
        npcText = getStringField(msg, {"text", "message"}).value_or("");
      }
      if (optionLabels.empty() && msg.contains("options") && msg["options"].is_array()) {
        for (const auto& option : msg["options"]) {
          if (option.is_string()) {
            optionLabels.push_back(option.get<std::string>());
          } else if (option.is_object()) {
            const auto label = getStringField(option, {"label", "text", "name", "id"});
            if (label.has_value() && !label->empty()) {
              optionLabels.push_back(label.value());
            }
          }
        }
      }

      std::string npcName = npcId;
      {
        std::lock_guard<std::mutex> lock(world_.mutex);
        auto npcIt = world_.data.npcs.find(npcId);
        if (npcIt != world_.data.npcs.end() && !npcIt->second.name.empty()) {
          npcName = npcIt->second.name;
        }
      }

      if (!npcText.empty()) {
        const std::string prefix = npcName.empty() ? "[NPC] " : ("[" + npcName + "] ");
        world_.pushChat(prefix + npcText);
      }
      if (!optionLabels.empty()) {
        std::string optionsStr;
        for (std::size_t i = 0; i < optionLabels.size(); ++i) {
          if (i > 0) {
            optionsStr += ", ";
          }
          optionsStr += optionLabels[i];
        }
        world_.pushChat("NPC Options: " + optionsStr);
      }
      return;
    }

    if (text.has_value() && !text->empty()) {
      world_.pushChat(text.value());
    }
  } catch (const std::exception& ex) {
    world_.pushError(std::string("Invalid JSON: ") + ex.what());
  }
}

void GameClient::processNetworkMessages() {
  for (const std::string& raw : wsClient_.pollMessages()) {
    parseAndApplyMessage(raw);
  }
}

void GameClient::maybeReconnect(float dt) {
  if (!reconnectEnabled_ || screen_ != ScreenState::World || wsClient_.isConnected()) {
    reconnectAccumulator_ = 0.0f;
    return;
  }
  reconnectAccumulator_ += dt;
  if (reconnectAccumulator_ < 3.0f) {
    return;
  }
  reconnectAccumulator_ = 0.0f;
  if (wsClient_.connect(wsUrl_, jwt_)) {
    joinSent_ = false;
    world_.pushChat("Reconnected to world socket");
  }
}

void GameClient::render() {
  if (screen_ == ScreenState::World) {
    renderWorldScreen();
  } else {
    window_.clear(sf::Color(20, 22, 28));
    if (screen_ == ScreenState::Auth) {
      renderAuthScreen();
    } else if (screen_ == ScreenState::CharacterSelect) {
      renderCharacterSelectScreen();
    } else {
      renderCharacterCreateScreen();
    }
  }
  window_.display();
}

void GameClient::drawLabel(const std::string& text, float x, float y, unsigned size, const sf::Color& color) {
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
  drawLabel("F1 toggles Login/Register, Tab switches field, Enter submits", 40, 82, 18, sf::Color(180, 185, 200));

  const bool isLogin = authMode_ == AuthMode::Login;
  drawLabel(std::string("Mode: ") + (isLogin ? "Login" : "Register"), 40, 130, 24, sf::Color(245, 205, 120));

  sf::RectangleShape userBox(sf::Vector2f(450, 42));
  userBox.setPosition(40, 180);
  userBox.setFillColor(sf::Color(38, 42, 52));
  userBox.setOutlineThickness(2.0f);
  userBox.setOutlineColor((authField_ == AuthField::Username) ? sf::Color::Cyan : sf::Color(90, 100, 120));
  window_.draw(userBox);

  sf::RectangleShape passBox(sf::Vector2f(450, 42));
  passBox.setPosition(40, 250);
  passBox.setFillColor(sf::Color(38, 42, 52));
  passBox.setOutlineThickness(2.0f);
  passBox.setOutlineColor((authField_ == AuthField::Password) ? sf::Color::Cyan : sf::Color(90, 100, 120));
  window_.draw(passBox);

  drawLabel("Username: " + username_, 52, 190, 20);
  drawLabel("Password: " + maskPassword(password_), 52, 260, 20);
  drawLabel(statusText_, 40, 330, 18, sf::Color(255, 170, 120));
}

void GameClient::renderCharacterSelectScreen() {
  const sf::Vector2u size = window_.getSize();
  sf::RectangleShape gradientTop(sf::Vector2f(static_cast<float>(size.x), 180.0f));
  gradientTop.setPosition(0.0f, 0.0f);
  gradientTop.setFillColor(sf::Color(30, 36, 52));
  window_.draw(gradientTop);

  drawLabel("Character Selection", 44, 30, 36, sf::Color(130, 210, 255));
  drawLabel("Left/Right to choose, Enter to confirm, Esc back", 44, 84, 18, sf::Color(186, 194, 214));

  const std::size_t optionCount = characters_.size() + 1;
  const float cardWidth = 240.0f;
  const float cardHeight = 320.0f;
  const float gap = 22.0f;
  const float totalWidth = static_cast<float>(optionCount) * cardWidth + static_cast<float>(optionCount - 1) * gap;
  const float startX = std::max(30.0f, (static_cast<float>(size.x) - totalWidth) * 0.5f);
  const float cardY = 170.0f;

  if (selectedCharacterIndex_ >= optionCount) {
    selectedCharacterIndex_ = 0;
  }

  for (std::size_t i = 0; i < optionCount; ++i) {
    const bool selected = i == selectedCharacterIndex_;
    const bool createCard = i == characters_.size();
    const float x = startX + static_cast<float>(i) * (cardWidth + gap);

    sf::RectangleShape card(sf::Vector2f(cardWidth, cardHeight));
    card.setPosition(x, cardY);
    card.setFillColor(selected ? sf::Color(46, 55, 80) : sf::Color(30, 35, 52));
    card.setOutlineThickness(selected ? 3.0f : 1.5f);
    card.setOutlineColor(selected ? sf::Color(245, 205, 120) : sf::Color(80, 92, 126));
    window_.draw(card);

    if (createCard) {
      sf::CircleShape marker(12.0f);
      marker.setPosition(x + cardWidth - 30.0f, cardY + 18.0f);
      marker.setFillColor(sf::Color(180, 190, 220));
      window_.draw(marker);

      sf::RectangleShape plusA(sf::Vector2f(80.0f, 10.0f));
      plusA.setPosition(x + 80.0f, cardY + 100.0f);
      plusA.setFillColor(sf::Color(180, 190, 220));
      window_.draw(plusA);
      sf::RectangleShape plusB(sf::Vector2f(10.0f, 80.0f));
      plusB.setPosition(x + 115.0f, cardY + 65.0f);
      plusB.setFillColor(sf::Color(180, 190, 220));
      window_.draw(plusB);

      drawLabel("Create New", x + 52.0f, cardY + 182.0f, 28, sf::Color(230, 235, 245));
      drawLabel("Character", x + 62.0f, cardY + 214.0f, 28, sf::Color(230, 235, 245));
      drawLabel("Start a fresh archetype", x + 28.0f, cardY + 270.0f, 16, sf::Color(166, 176, 200));
      continue;
    }

    const CharacterInfo& character = characters_[i];
    const ClassArchetype& archetype = archetypeFromClassName(character.className);

    sf::RectangleShape previewFrame(sf::Vector2f(180.0f, 118.0f));
    previewFrame.setPosition(x + 30.0f, cardY + 46.0f);
    previewFrame.setFillColor(sf::Color(20, 24, 37));
    previewFrame.setOutlineThickness(2.0f);
    previewFrame.setOutlineColor(archetype.color);
    window_.draw(previewFrame);

    sf::CircleShape classMarker(10.0f);
    classMarker.setPosition(x + cardWidth - 28.0f, cardY + 16.0f);
    classMarker.setFillColor(archetype.color);
    window_.draw(classMarker);

    if (archetype.name == std::string("Warrior")) {
      sf::RectangleShape blade(sf::Vector2f(22.0f, 70.0f));
      blade.setPosition(x + 108.0f, cardY + 69.0f);
      blade.setFillColor(archetype.accent);
      window_.draw(blade);
      sf::RectangleShape guard(sf::Vector2f(54.0f, 10.0f));
      guard.setPosition(x + 92.0f, cardY + 122.0f);
      guard.setFillColor(archetype.color);
      window_.draw(guard);
    } else if (archetype.name == std::string("Mage")) {
      sf::CircleShape orb(32.0f);
      orb.setPosition(x + 88.0f, cardY + 80.0f);
      orb.setFillColor(archetype.accent);
      orb.setOutlineThickness(5.0f);
      orb.setOutlineColor(archetype.color);
      window_.draw(orb);
    } else {
      sf::ConvexShape diamond(4);
      diamond.setPoint(0, sf::Vector2f(x + 120.0f, cardY + 66.0f));
      diamond.setPoint(1, sf::Vector2f(x + 158.0f, cardY + 106.0f));
      diamond.setPoint(2, sf::Vector2f(x + 120.0f, cardY + 146.0f));
      diamond.setPoint(3, sf::Vector2f(x + 82.0f, cardY + 106.0f));
      diamond.setFillColor(archetype.accent);
      diamond.setOutlineThickness(4.0f);
      diamond.setOutlineColor(archetype.color);
      window_.draw(diamond);
    }

    drawLabel(character.name, x + 22.0f, cardY + 186.0f, 26, sf::Color(235, 238, 245));
    drawLabel(character.className, x + 22.0f, cardY + 224.0f, 20, archetype.color);
    drawLabel("Level 1", x + 22.0f, cardY + 254.0f, 20, sf::Color(190, 198, 220));
    drawLabel(archetype.description, x + 22.0f, cardY + 286.0f, 14, sf::Color(170, 178, 200));
  }

  drawLabel("Authenticated user: " + username_, 44, static_cast<float>(size.y) - 74.0f, 20, sf::Color(210, 220, 240));
  drawLabel(statusText_, 44, static_cast<float>(size.y) - 44.0f, 18, sf::Color(255, 176, 132));
}

void GameClient::renderCharacterCreateScreen() {
  const sf::Vector2u size = window_.getSize();
  const ClassArchetype& archetype = kClassArchetypes[createClassIndex_];

  sf::RectangleShape bg(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
  bg.setPosition(0.0f, 0.0f);
  bg.setFillColor(sf::Color(22, 27, 42));
  window_.draw(bg);

  sf::RectangleShape panel(sf::Vector2f(900.0f, 520.0f));
  panel.setPosition(190.0f, 110.0f);
  panel.setFillColor(sf::Color(28, 34, 54));
  panel.setOutlineThickness(2.0f);
  panel.setOutlineColor(sf::Color(90, 102, 136));
  window_.draw(panel);

  drawLabel("Create Character", 230, 138, 36, sf::Color(130, 210, 255));
  drawLabel("Type name, Left/Right selects class, Enter creates, Esc cancels", 230, 188, 18,
            sf::Color(178, 188, 210));

  sf::RectangleShape nameBox(sf::Vector2f(520.0f, 52.0f));
  nameBox.setPosition(230.0f, 240.0f);
  nameBox.setFillColor(sf::Color(18, 22, 35));
  nameBox.setOutlineThickness(2.0f);
  nameBox.setOutlineColor(sf::Color(95, 109, 146));
  window_.draw(nameBox);

  drawLabel("Name: " + createCharacterName_, 246, 253, 24, sf::Color::White);

  sf::RectangleShape classPanel(sf::Vector2f(520.0f, 270.0f));
  classPanel.setPosition(230.0f, 320.0f);
  classPanel.setFillColor(sf::Color(20, 25, 39));
  classPanel.setOutlineThickness(2.0f);
  classPanel.setOutlineColor(archetype.color);
  window_.draw(classPanel);

  drawLabel(archetype.name, 250, 340, 34, archetype.color);
  drawLabel(archetype.description, 250, 388, 18, sf::Color(220, 225, 235));
  drawLabel(archetype.stats, 250, 422, 18, archetype.accent);

  sf::CircleShape marker(16.0f);
  marker.setPosition(698.0f, 338.0f);
  marker.setFillColor(archetype.color);
  window_.draw(marker);

  sf::RectangleShape previewFrame(sf::Vector2f(220.0f, 220.0f));
  previewFrame.setPosition(798.0f, 320.0f);
  previewFrame.setFillColor(sf::Color(18, 24, 36));
  previewFrame.setOutlineThickness(2.0f);
  previewFrame.setOutlineColor(archetype.color);
  window_.draw(previewFrame);

  if (archetype.name == std::string("Warrior")) {
    sf::RectangleShape blade(sf::Vector2f(28.0f, 110.0f));
    blade.setPosition(892.0f, 364.0f);
    blade.setFillColor(archetype.accent);
    window_.draw(blade);
    sf::RectangleShape guard(sf::Vector2f(70.0f, 12.0f));
    guard.setPosition(871.0f, 444.0f);
    guard.setFillColor(archetype.color);
    window_.draw(guard);
  } else if (archetype.name == std::string("Mage")) {
    sf::CircleShape orb(56.0f);
    orb.setPosition(850.0f, 372.0f);
    orb.setFillColor(archetype.accent);
    orb.setOutlineThickness(7.0f);
    orb.setOutlineColor(archetype.color);
    window_.draw(orb);
  } else {
    sf::ConvexShape diamond(4);
    diamond.setPoint(0, sf::Vector2f(908.0f, 360.0f));
    diamond.setPoint(1, sf::Vector2f(970.0f, 430.0f));
    diamond.setPoint(2, sf::Vector2f(908.0f, 500.0f));
    diamond.setPoint(3, sf::Vector2f(846.0f, 430.0f));
    diamond.setFillColor(archetype.accent);
    diamond.setOutlineThickness(6.0f);
    diamond.setOutlineColor(archetype.color);
    window_.draw(diamond);
  }

  drawLabel("Class Preview", 830, 548, 18, sf::Color(184, 196, 220));
  drawLabel(statusText_, 230, 604, 18, sf::Color(255, 176, 132));
}

void GameClient::renderWorldScreen() {
  const WorldSnapshot snapshot = world_.snapshot();
  window_.clear(sf::Color(12, 14, 20));
  renderer_.render(window_, snapshot, fontLoaded_ ? &font_ : nullptr);

  sf::RectangleShape panel(sf::Vector2f(320.0f, 136.0f));
  panel.setPosition(14.0f, 10.0f);
  panel.setFillColor(sf::Color(10, 12, 18, 190));
  panel.setOutlineThickness(1.0f);
  panel.setOutlineColor(sf::Color(200, 205, 220, 120));
  window_.draw(panel);

  auto localIt = snapshot.players.find(snapshot.localPlayerId);
  const PlayerState* self = (localIt == snapshot.players.end()) ? nullptr : &localIt->second;
  drawLabel("Connection: " + snapshot.connectionStatus, 24, 20, 16, snapshot.connected ? sf::Color(130, 245, 150)
                                                                                          : sf::Color(255, 150, 120));
  drawLabel("Controls: WASD/Arrows move, Space attack, E/Click talk, Esc exit", 24, 42, 14,
            sf::Color(220, 225, 235));
  if (self != nullptr) {
    drawLabel("Player: " + self->name + "  Class: " + self->className, 24, 66, 16, sf::Color::White);
    drawLabel("HP: " + std::to_string(self->hp) + "/" + std::to_string(self->maxHp) + "  Level: " +
                  std::to_string(self->level),
              24, 88, 16, sf::Color::White);
    drawLabel("XP: " + std::to_string(self->experience) + "  Pos: (" + std::to_string(self->x) + ", " +
                  std::to_string(self->y) + ")",
              24, 110, 16, sf::Color::White);
  }

  const float chatYBase = static_cast<float>(window_.getSize().y) - 24.0f;
  int line = 0;
  for (auto it = snapshot.chatLines.rbegin(); it != snapshot.chatLines.rend() && line < 7; ++it, ++line) {
    drawLabel(it->text, 18, chatYBase - static_cast<float>(line) * 18.0f, 15, sf::Color(225, 230, 240));
  }

  line = 0;
  for (auto it = snapshot.errors.rbegin(); it != snapshot.errors.rend() && line < 3; ++it, ++line) {
    drawLabel(*it, 360, 20 + static_cast<float>(line) * 20.0f, 16, sf::Color(255, 125, 110));
  }

  if (settingsMenuOpen_) {
    renderSettingsMenu();
    return;
  }
  if (snapshot.dialog.active) {
    renderDialogOverlay(snapshot);
  }
}

void GameClient::renderDialogOverlay(const WorldSnapshot& snapshot) {
  dialogOptionRects_.clear();
  const DialogState& dialog = snapshot.dialog;
  if (!dialog.active) {
    return;
  }

  const sf::Vector2u windowSize = window_.getSize();
  const float panelWidth = std::min(860.0f, static_cast<float>(windowSize.x) - 120.0f);
  const float panelHeight = 300.0f;
  const float panelX = (static_cast<float>(windowSize.x) - panelWidth) * 0.5f;
  const float panelY = static_cast<float>(windowSize.y) - panelHeight - 38.0f;

  sf::RectangleShape dimmer(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
  dimmer.setFillColor(sf::Color(8, 10, 16, 120));
  window_.draw(dimmer);

  sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
  panel.setPosition(panelX, panelY);
  panel.setFillColor(sf::Color(20, 25, 35, 240));
  panel.setOutlineThickness(2.0f);
  panel.setOutlineColor(sf::Color(160, 178, 208, 220));
  window_.draw(panel);

  sf::RectangleShape portraitFrame(sf::Vector2f(112.0f, 112.0f));
  portraitFrame.setPosition(panelX + 20.0f, panelY + 20.0f);
  const std::string role = toLowerCopy(dialog.npcRole);
  const bool merchant = role == "merchant";
  portraitFrame.setFillColor(merchant ? sf::Color(70, 92, 128) : sf::Color(86, 72, 120));
  portraitFrame.setOutlineThickness(2.0f);
  portraitFrame.setOutlineColor(sf::Color(220, 228, 245, 220));
  window_.draw(portraitFrame);

  sf::CircleShape portrait(36.0f);
  portrait.setPosition(panelX + 38.0f, panelY + 34.0f);
  portrait.setFillColor(merchant ? sf::Color(220, 176, 120) : sf::Color(166, 210, 145));
  window_.draw(portrait);

  const char portraitGlyph = merchant ? '$' : '!';
  drawLabel(std::string(1, portraitGlyph), panelX + 67.0f, panelY + 48.0f, 34, sf::Color(28, 32, 40));

  const std::string npcName = dialog.npcName.empty() ? "NPC" : dialog.npcName;
  drawLabel(npcName, panelX + 150.0f, panelY + 24.0f, 28, sf::Color(240, 245, 255));
  drawLabel(dialog.text, panelX + 150.0f, panelY + 68.0f, 19, sf::Color(220, 225, 238));
  drawLabel("Choose a response:", panelX + 24.0f, panelY + 150.0f, 17, sf::Color(184, 198, 220));

  const float optionWidth = panelWidth - 48.0f;
  const float optionHeight = 36.0f;
  for (std::size_t i = 0; i < dialog.responses.size(); ++i) {
    const float y = panelY + 180.0f + static_cast<float>(i) * 42.0f;
    sf::FloatRect rect(panelX + 24.0f, y, optionWidth, optionHeight);
    dialogOptionRects_.push_back(rect);

    sf::RectangleShape optionBg(sf::Vector2f(rect.width, rect.height));
    optionBg.setPosition(rect.left, rect.top);
    optionBg.setFillColor(sf::Color(38, 46, 66, 245));
    optionBg.setOutlineThickness(1.0f);
    optionBg.setOutlineColor(sf::Color(120, 138, 170, 220));
    window_.draw(optionBg);

    drawLabel(std::to_string(i + 1) + ". " + dialog.responses[i].text, rect.left + 10.0f, rect.top + 7.0f, 18,
              sf::Color(234, 240, 252));
  }

  drawLabel("Select a response to continue", panelX + panelWidth - 260.0f, panelY + panelHeight - 26.0f, 14,
            sf::Color(170, 182, 204));
}

void GameClient::renderSettingsMenu() {
  updateSettingsLayout();

  const sf::Vector2u size = window_.getSize();
  sf::RectangleShape dimmer(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
  dimmer.setPosition(0.0f, 0.0f);
  dimmer.setFillColor(sf::Color(8, 10, 16, 155));
  window_.draw(dimmer);

  sf::RectangleShape panel(sf::Vector2f(settingsPanelRect_.width, settingsPanelRect_.height));
  panel.setPosition(settingsPanelRect_.left, settingsPanelRect_.top);
  panel.setFillColor(sf::Color(24, 28, 40, 240));
  panel.setOutlineThickness(2.0f);
  panel.setOutlineColor(sf::Color(186, 200, 230, 210));
  window_.draw(panel);

  drawLabel("Settings (F10)", settingsPanelRect_.left + 20.0f, settingsPanelRect_.top + 16.0f, 24,
            sf::Color(232, 238, 255));
  drawLabel("Viewport presets", settingsPanelRect_.left + 22.0f, settingsPanelRect_.top + 48.0f, 16,
            sf::Color(188, 199, 224));

  const auto drawPresetButton = [this](const sf::FloatRect& rect, const std::string& label) {
    sf::RectangleShape button(sf::Vector2f(rect.width, rect.height));
    button.setPosition(rect.left, rect.top);
    button.setFillColor(sf::Color(37, 44, 63, 250));
    button.setOutlineThickness(1.0f);
    button.setOutlineColor(sf::Color(112, 126, 158, 230));
    window_.draw(button);
    drawLabel(label, rect.left + 12.0f, rect.top + 8.0f, 18, sf::Color(228, 235, 248));
  };

  drawPresetButton(viewportPresetA_, "1920x1080");
  drawPresetButton(viewportPresetB_, "1280x768");

  drawLabel("Camera Zoom", settingsPanelRect_.left + 22.0f, settingsPanelRect_.top + 134.0f, 16,
            sf::Color(188, 199, 224));
  char zoomText[16];
  std::snprintf(zoomText, sizeof(zoomText), "%.2f", settingsZoom_);
  drawLabel(zoomText, settingsPanelRect_.left + settingsPanelRect_.width - 70.0f, settingsPanelRect_.top + 134.0f,
            16, sf::Color(228, 235, 248));

  sf::RectangleShape track(sf::Vector2f(zoomSliderTrackRect_.width, zoomSliderTrackRect_.height));
  track.setPosition(zoomSliderTrackRect_.left, zoomSliderTrackRect_.top);
  track.setFillColor(sf::Color(62, 74, 106, 220));
  window_.draw(track);

  const float filledWidth = std::max(0.0f, (settingsZoom_ - kMinZoom) / (kMaxZoom - kMinZoom) * zoomSliderTrackRect_.width);
  sf::RectangleShape fill(sf::Vector2f(filledWidth, zoomSliderTrackRect_.height));
  fill.setPosition(zoomSliderTrackRect_.left, zoomSliderTrackRect_.top);
  fill.setFillColor(sf::Color(122, 208, 255, 240));
  window_.draw(fill);

  sf::RectangleShape knob(sf::Vector2f(zoomSliderKnobRect_.width, zoomSliderKnobRect_.height));
  knob.setPosition(zoomSliderKnobRect_.left, zoomSliderKnobRect_.top);
  knob.setFillColor(sf::Color(232, 241, 255, 245));
  knob.setOutlineThickness(1.0f);
  knob.setOutlineColor(sf::Color(86, 94, 128, 240));
  window_.draw(knob);

  drawLabel("Drag slider to adjust zoom in real-time", settingsPanelRect_.left + 22.0f, settingsPanelRect_.top + 206.0f,
            14, sf::Color(162, 174, 202));
}
