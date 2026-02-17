#include "SpriteManager.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <string>

namespace {
const std::string kTileGrass = "tile_grass";
const std::string kTileWater = "tile_water";
const std::string kTileWall = "tile_wall";
const std::string kTileForest = "tile_forest";
const std::string kPlayerNorth = "player_north";
const std::string kPlayerEast = "player_east";
const std::string kPlayerSouth = "player_south";
const std::string kPlayerWest = "player_west";
const std::string kNpc = "npc";
const std::string kMobAlive = "mob_alive";
const std::string kMobDead = "mob_dead";
}  // namespace

bool SpriteManager::initialize(const std::string& spritesDirectory) {
  spritesDirectory_ = spritesDirectory;
  textures_.clear();

  bool allLoadedFromDisk = true;
  allLoadedFromDisk &= loadTextureOrPlaceholder(kTileGrass, "grass.png", makeTileGrassPixels());
  allLoadedFromDisk &= loadTextureOrPlaceholder(kTileWater, "water.png", makeTileWaterPixels());
  allLoadedFromDisk &= loadTextureOrPlaceholder(kTileWall, "wall.png", makeTileWallPixels());
  allLoadedFromDisk &= loadTextureOrPlaceholder(kTileForest, "forest.png", makeTileForestPixels());

  allLoadedFromDisk &=
      loadTextureOrPlaceholder(kPlayerNorth, "player_north.png", makePlayerPixels(FacingDirection::North));
  allLoadedFromDisk &=
      loadTextureOrPlaceholder(kPlayerEast, "player_east.png", makePlayerPixels(FacingDirection::East));
  allLoadedFromDisk &=
      loadTextureOrPlaceholder(kPlayerSouth, "player_south.png", makePlayerPixels(FacingDirection::South));
  allLoadedFromDisk &=
      loadTextureOrPlaceholder(kPlayerWest, "player_west.png", makePlayerPixels(FacingDirection::West));

  allLoadedFromDisk &= loadTextureOrPlaceholder(kNpc, "npc.png", makeNpcPixels());
  allLoadedFromDisk &= loadTextureOrPlaceholder(kMobAlive, "mob.png", makeMobPixels(true));
  allLoadedFromDisk &= loadTextureOrPlaceholder(kMobDead, "mob_dead.png", makeMobPixels(false));

  return allLoadedFromDisk;
}

const sf::Texture& SpriteManager::tile(TileType tileType) const {
  switch (tileType) {
    case TileType::Grass:
      return textures_.at(kTileGrass);
    case TileType::Water:
      return textures_.at(kTileWater);
    case TileType::Wall:
      return textures_.at(kTileWall);
    case TileType::Forest:
      return textures_.at(kTileForest);
  }
  return textures_.at(kTileGrass);
}

const sf::Texture& SpriteManager::player(FacingDirection direction) const {
  switch (direction) {
    case FacingDirection::North:
      return textures_.at(kPlayerNorth);
    case FacingDirection::East:
      return textures_.at(kPlayerEast);
    case FacingDirection::South:
      return textures_.at(kPlayerSouth);
    case FacingDirection::West:
      return textures_.at(kPlayerWest);
  }
  return textures_.at(kPlayerSouth);
}

const sf::Texture& SpriteManager::npc() const { return textures_.at(kNpc); }

const sf::Texture& SpriteManager::mob(bool alive) const {
  return textures_.at(alive ? kMobAlive : kMobDead);
}

bool SpriteManager::loadTextureOrPlaceholder(const std::string& key, const std::string& fileName,
                                             const std::vector<sf::Uint8>& fallbackPixels) {
  sf::Texture texture;
  const std::filesystem::path fullPath = std::filesystem::path(spritesDirectory_) / fileName;

  bool loadedFromDisk = false;
  if (std::filesystem::exists(fullPath)) {
    loadedFromDisk = texture.loadFromFile(fullPath.string());
  }

  if (!loadedFromDisk) {
    sf::Image image;
    image.create(kSpriteSize, kSpriteSize, fallbackPixels.data());
    if (!texture.loadFromImage(image)) {
      return false;
    }
  }

  texture.setSmooth(false);
  textures_[key] = std::move(texture);
  return loadedFromDisk;
}

std::vector<sf::Uint8> SpriteManager::makeBlankPixels(Rgba color) {
  std::vector<sf::Uint8> pixels(kSpriteSize * kSpriteSize * 4, 0);
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      setPixel(pixels, x, y, color);
    }
  }
  return pixels;
}

void SpriteManager::setPixel(std::vector<sf::Uint8>& pixels, unsigned x, unsigned y, Rgba color) {
  if (x >= kSpriteSize || y >= kSpriteSize) {
    return;
  }
  const std::size_t i = static_cast<std::size_t>((y * kSpriteSize + x) * 4);
  pixels[i + 0] = color.r;
  pixels[i + 1] = color.g;
  pixels[i + 2] = color.b;
  pixels[i + 3] = color.a;
}

void SpriteManager::fillRect(std::vector<sf::Uint8>& pixels, unsigned x, unsigned y, unsigned width,
                             unsigned height, Rgba color) {
  for (unsigned py = y; py < y + height; ++py) {
    for (unsigned px = x; px < x + width; ++px) {
      setPixel(pixels, px, py, color);
    }
  }
}

std::vector<sf::Uint8> SpriteManager::makeTileGrassPixels() {
  auto pixels = makeBlankPixels(Rgba{70, 140, 62, 255});
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      if (((x + y) % 14) == 0) {
        setPixel(pixels, x, y, Rgba{86, 164, 76, 255});
      } else if (((x * 3 + y * 5) % 26) == 0) {
        setPixel(pixels, x, y, Rgba{58, 122, 54, 255});
      }
    }
  }
  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeTileWaterPixels() {
  auto pixels = makeBlankPixels(Rgba{52, 104, 176, 255});
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      if (((x + y * 2) % 16) < 4) {
        setPixel(pixels, x, y, Rgba{78, 140, 214, 255});
      }
      if (((x * 2 + y) % 22) == 0) {
        setPixel(pixels, x, y, Rgba{34, 84, 150, 255});
      }
    }
  }
  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeTileWallPixels() {
  auto pixels = makeBlankPixels(Rgba{114, 118, 124, 255});
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      if (y % 16 == 0 || x % 32 == 0) {
        setPixel(pixels, x, y, Rgba{90, 94, 102, 255});
      }
      if (((x + y) % 38) == 0) {
        setPixel(pixels, x, y, Rgba{136, 140, 146, 255});
      }
    }
  }
  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeTileForestPixels() {
  auto pixels = makeBlankPixels(Rgba{38, 96, 42, 255});
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      if (((x * y) % 34) == 0) {
        setPixel(pixels, x, y, Rgba{28, 72, 31, 255});
      } else if (((x + y * 4) % 18) == 0) {
        setPixel(pixels, x, y, Rgba{52, 126, 58, 255});
      }
    }
  }

  for (unsigned cy = 10; cy <= 50; cy += 20) {
    for (unsigned cx = 10; cx <= 50; cx += 20) {
      for (int oy = -4; oy <= 4; ++oy) {
        for (int ox = -4; ox <= 4; ++ox) {
          const int px = static_cast<int>(cx) + ox;
          const int py = static_cast<int>(cy) + oy;
          if (px < 0 || py < 0) {
            continue;
          }
          if (ox * ox + oy * oy <= 16) {
            setPixel(pixels, static_cast<unsigned>(px), static_cast<unsigned>(py), Rgba{18, 56, 22, 255});
          }
        }
      }
    }
  }

  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makePlayerPixels(FacingDirection direction) {
  auto pixels = makeBlankPixels(Rgba{0, 0, 0, 0});

  fillRect(pixels, 20, 16, 24, 14, Rgba{248, 224, 176, 255});
  fillRect(pixels, 22, 30, 20, 20, Rgba{90, 150, 235, 255});
  fillRect(pixels, 16, 32, 6, 14, Rgba{60, 108, 176, 255});
  fillRect(pixels, 42, 32, 6, 14, Rgba{60, 108, 176, 255});
  fillRect(pixels, 24, 50, 6, 12, Rgba{72, 56, 108, 255});
  fillRect(pixels, 34, 50, 6, 12, Rgba{72, 56, 108, 255});

  const Rgba marker{255, 232, 112, 255};
  switch (direction) {
    case FacingDirection::North:
      fillRect(pixels, 28, 4, 8, 8, marker);
      fillRect(pixels, 30, 0, 4, 4, marker);
      break;
    case FacingDirection::East:
      fillRect(pixels, 50, 28, 8, 8, marker);
      fillRect(pixels, 58, 30, 4, 4, marker);
      break;
    case FacingDirection::South:
      fillRect(pixels, 28, 52, 8, 8, marker);
      fillRect(pixels, 30, 60, 4, 4, marker);
      break;
    case FacingDirection::West:
      fillRect(pixels, 6, 28, 8, 8, marker);
      fillRect(pixels, 2, 30, 4, 4, marker);
      break;
  }

  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      const std::size_t i = static_cast<std::size_t>((y * kSpriteSize + x) * 4);
      if (pixels[i + 3] == 0) {
        continue;
      }
      const bool border =
          x == 0 || y == 0 || x + 1 == kSpriteSize || y + 1 == kSpriteSize ||
          pixels[(static_cast<std::size_t>((y * kSpriteSize + std::max(1u, x) - 1) * 4)) + 3] == 0 ||
          pixels[(static_cast<std::size_t>((y * kSpriteSize + std::min(kSpriteSize - 1, x + 1)) * 4)) + 3] == 0 ||
          pixels[(static_cast<std::size_t>(((std::max(1u, y) - 1) * kSpriteSize + x) * 4)) + 3] == 0 ||
          pixels[(static_cast<std::size_t>(((std::min(kSpriteSize - 1, y + 1)) * kSpriteSize + x) * 4)) + 3] == 0;
      if (border) {
        setPixel(pixels, x, y, Rgba{26, 32, 54, 255});
      }
    }
  }

  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeNpcPixels() {
  auto pixels = makeBlankPixels(Rgba{0, 0, 0, 0});
  const int cx = 32;
  const int cy = 32;
  const int radius = 20;

  for (int y = 0; y < static_cast<int>(kSpriteSize); ++y) {
    for (int x = 0; x < static_cast<int>(kSpriteSize); ++x) {
      const int dx = x - cx;
      const int dy = y - cy;
      const int d2 = dx * dx + dy * dy;
      if (d2 <= radius * radius) {
        setPixel(pixels, static_cast<unsigned>(x), static_cast<unsigned>(y), Rgba{100, 240, 245, 255});
      }
      if (d2 >= (radius - 1) * (radius - 1) && d2 <= radius * radius) {
        setPixel(pixels, static_cast<unsigned>(x), static_cast<unsigned>(y), Rgba{60, 100, 150, 255});
      }
    }
  }

  fillRect(pixels, 24, 28, 4, 4, Rgba{20, 38, 60, 255});
  fillRect(pixels, 36, 28, 4, 4, Rgba{20, 38, 60, 255});
  fillRect(pixels, 26, 38, 12, 4, Rgba{44, 78, 98, 255});

  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeMobPixels(bool alive) {
  auto pixels = makeBlankPixels(Rgba{0, 0, 0, 0});
  const Rgba body = alive ? Rgba{220, 58, 58, 255} : Rgba{95, 52, 52, 255};
  const Rgba edge = alive ? Rgba{132, 26, 26, 255} : Rgba{62, 34, 34, 255};

  for (unsigned y = 12; y <= 50; ++y) {
    const int row = static_cast<int>(y) - 12;
    const int halfWidth = std::max(2, 20 - row / 2);
    const int cx = 32;
    for (int x = cx - halfWidth; x <= cx + halfWidth; ++x) {
      setPixel(pixels, static_cast<unsigned>(x), y, body);
    }
    setPixel(pixels, static_cast<unsigned>(cx - halfWidth), y, edge);
    setPixel(pixels, static_cast<unsigned>(cx + halfWidth), y, edge);
  }

  fillRect(pixels, 24, 22, 4, 4, Rgba{255, 206, 206, 255});
  fillRect(pixels, 36, 22, 4, 4, Rgba{255, 206, 206, 255});

  return pixels;
}
