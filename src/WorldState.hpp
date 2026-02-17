#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

enum class TileType : std::uint8_t { Grass, Water, Wall, Forest };

struct PlayerState {
  std::string id;
  std::string name;
  std::string className = "Unknown";
  int x = 0;
  int y = 0;
  float renderX = 0.0f;
  float renderY = 0.0f;
  int hp = 100;
  int maxHp = 100;
  int level = 1;
  int experience = 0;
  bool alive = true;
};

struct NpcState {
  std::string id;
  std::string name;
  int x = 0;
  int y = 0;
  float renderX = 0.0f;
  float renderY = 0.0f;
};

struct MobState {
  std::string id;
  std::string name;
  int x = 0;
  int y = 0;
  float renderX = 0.0f;
  float renderY = 0.0f;
  int hp = 100;
  int maxHp = 100;
  bool alive = true;
  bool aggressive = false;
};

struct FloatingCombatText {
  std::string text;
  float worldX = 0.0f;
  float worldY = 0.0f;
  std::uint8_t r = 255;
  std::uint8_t g = 255;
  std::uint8_t b = 255;
  float ttl = 1.1f;
};

struct ChatLine {
  std::string text;
  std::uint64_t createdAtMs = 0;
};

struct WorldSnapshot {
  int width = 50;
  int height = 50;
  int tileSize = 32;
  std::vector<TileType> tiles = std::vector<TileType>(width * height, TileType::Grass);

  std::string localPlayerId;
  std::unordered_map<std::string, PlayerState> players;
  std::unordered_map<std::string, NpcState> npcs;
  std::unordered_map<std::string, MobState> mobs;

  std::deque<FloatingCombatText> combatTexts;
  std::deque<ChatLine> chatLines;
  std::deque<std::string> errors;

  std::string connectionStatus = "Disconnected";
  bool connected = false;
  bool worldReady = false;
  std::uint64_t lastServerUpdateMs = 0;
};

struct WorldState {
  mutable std::mutex mutex;
  WorldSnapshot data;

  static std::uint64_t nowMs() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
  }

  WorldSnapshot snapshot() const {
    std::lock_guard<std::mutex> lock(mutex);
    return data;
  }

  void setConnectionStatus(const std::string& status, bool isConnected) {
    std::lock_guard<std::mutex> lock(mutex);
    data.connectionStatus = status;
    data.connected = isConnected;
  }

  void pushChat(std::string text) {
    std::lock_guard<std::mutex> lock(mutex);
    if (text.empty()) {
      return;
    }
    data.chatLines.push_back(ChatLine{std::move(text), nowMs()});
    while (data.chatLines.size() > 12) {
      data.chatLines.pop_front();
    }
  }

  void pushError(std::string text) {
    std::lock_guard<std::mutex> lock(mutex);
    if (text.empty()) {
      return;
    }
    data.errors.push_back(std::move(text));
    while (data.errors.size() > 6) {
      data.errors.pop_front();
    }
  }

  bool isBlocked(int x, int y) const {
    std::lock_guard<std::mutex> lock(mutex);
    if (x < 0 || y < 0 || x >= data.width || y >= data.height) {
      return true;
    }
    const TileType t = data.tiles[static_cast<std::size_t>(y * data.width + x)];
    return t == TileType::Wall || t == TileType::Water;
  }
};
