#pragma once

#include <string>
#include <unordered_map>

#include "Math3D.hpp"

struct PlayerState {
  std::string id;
  std::string character;
  Vec3 position{0.0f, 0.5f, 0.0f};
};

struct WorldState {
  PlayerState localPlayer;
  std::unordered_map<std::string, PlayerState> remotePlayers;
};
