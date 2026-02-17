#include "SpriteManager.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>

namespace {
const std::string kTileGrass = "tile_grass";
const std::string kTileWater = "tile_water";
const std::string kTileWall = "tile_wall";
const std::string kTileForest = "tile_forest";
const std::string kPlayerSheet = "player_sheet";
const std::string kNpcSheet = "npc_sheet";
const std::string kMobSheet = "mob_sheet";
const std::string kMobDead = "mob_dead";

unsigned frameForColumn(unsigned col) {
  if (col >= SpriteManager::kWalkStartColumn &&
      col < SpriteManager::kWalkStartColumn + SpriteManager::kWalkFrameCount) {
    return col - SpriteManager::kWalkStartColumn;
  }
  if (col >= SpriteManager::kIdleStartColumn &&
      col < SpriteManager::kIdleStartColumn + SpriteManager::kIdleFrameCount) {
    return col - SpriteManager::kIdleStartColumn;
  }
  if (col >= SpriteManager::kAttackStartColumn &&
      col < SpriteManager::kAttackStartColumn + SpriteManager::kAttackFrameCount) {
    return col - SpriteManager::kAttackStartColumn;
  }
  if (col >= SpriteManager::kHurtStartColumn &&
      col < SpriteManager::kHurtStartColumn + SpriteManager::kHurtFrameCount) {
    return col - SpriteManager::kHurtStartColumn;
  }
  if (col >= SpriteManager::kDeathStartColumn &&
      col < SpriteManager::kDeathStartColumn + SpriteManager::kDeathFrameCount) {
    return col - SpriteManager::kDeathStartColumn;
  }
  return 0;
}
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
      loadTextureOrPlaceholder(kPlayerSheet, "player_sheet.png", makePlayerSheetPixels(), kSheetWidth, kSheetHeight);
  allLoadedFromDisk &=
      loadTextureOrPlaceholder(kNpcSheet, "npc_sheet.png", makeNpcSheetPixels(), kSheetWidth, kSheetHeight);
  allLoadedFromDisk &=
      loadTextureOrPlaceholder(kMobSheet, "mob_sheet.png", makeMobSheetPixels(), kSheetWidth, kSheetHeight);

  // Optional legacy sprite used when a mob dies.
  loadTextureOrPlaceholder(kMobDead, "mob_dead.png", makeMobPixels(false));

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

const sf::Texture& SpriteManager::playerSheet() const { return textures_.at(kPlayerSheet); }

const sf::Texture& SpriteManager::npcSheet() const { return textures_.at(kNpcSheet); }

const sf::Texture& SpriteManager::mobSheet() const { return textures_.at(kMobSheet); }

const sf::Texture& SpriteManager::mobDead() const { return textures_.at(kMobDead); }

bool SpriteManager::loadTextureOrPlaceholder(const std::string& key, const std::string& fileName,
                                             const std::vector<sf::Uint8>& fallbackPixels, unsigned width,
                                             unsigned height) {
  sf::Texture texture;
  const std::filesystem::path fullPath = std::filesystem::path(spritesDirectory_) / fileName;

  bool loadedFromDisk = false;
  if (std::filesystem::exists(fullPath)) {
    loadedFromDisk = texture.loadFromFile(fullPath.string());
  }

  if (!loadedFromDisk) {
    sf::Image image;
    image.create(width, height, fallbackPixels.data());
    if (!texture.loadFromImage(image)) {
      return false;
    }
  }

  texture.setSmooth(false);
  textures_[key] = std::move(texture);
  return loadedFromDisk;
}

std::vector<sf::Uint8> SpriteManager::makeBlankPixels(unsigned width, unsigned height, Rgba color) {
  std::vector<sf::Uint8> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);
  for (unsigned y = 0; y < height; ++y) {
    for (unsigned x = 0; x < width; ++x) {
      setPixel(pixels, width, height, x, y, color);
    }
  }
  return pixels;
}

void SpriteManager::setPixel(std::vector<sf::Uint8>& pixels, unsigned width, unsigned height, unsigned x, unsigned y,
                             Rgba color) {
  if (x >= width || y >= height) {
    return;
  }
  const std::size_t i = static_cast<std::size_t>((y * width + x) * 4);
  pixels[i + 0] = color.r;
  pixels[i + 1] = color.g;
  pixels[i + 2] = color.b;
  pixels[i + 3] = color.a;
}

void SpriteManager::fillRect(std::vector<sf::Uint8>& pixels, unsigned x, unsigned y, unsigned width, unsigned height,
                             unsigned canvasWidth, unsigned canvasHeight, Rgba color) {
  for (unsigned py = y; py < y + height; ++py) {
    for (unsigned px = x; px < x + width; ++px) {
      setPixel(pixels, canvasWidth, canvasHeight, px, py, color);
    }
  }
}

void SpriteManager::blitFrame(std::vector<sf::Uint8>& sheet, unsigned frameX, unsigned frameY,
                              const std::vector<sf::Uint8>& framePixels) {
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      const std::size_t src = static_cast<std::size_t>((y * kSpriteSize + x) * 4);
      const unsigned destX = frameX * kSpriteSize + x;
      const unsigned destY = frameY * kSpriteSize + y;
      const std::size_t dst = static_cast<std::size_t>((destY * kSheetWidth + destX) * 4);
      sheet[dst + 0] = framePixels[src + 0];
      sheet[dst + 1] = framePixels[src + 1];
      sheet[dst + 2] = framePixels[src + 2];
      sheet[dst + 3] = framePixels[src + 3];
    }
  }
}

unsigned SpriteManager::directionRowOffset(SpriteSheetDirection direction) {
  switch (direction) {
    case SpriteSheetDirection::Front:
      return 0;
    case SpriteSheetDirection::Left:
      return 1;
    case SpriteSheetDirection::Diagonal:
      return 2;
    case SpriteSheetDirection::Back:
      return 3;
  }
  return 0;
}

std::vector<sf::Uint8> SpriteManager::makeHumanoidFrame(Rgba armor, Rgba trim, unsigned frame) {
  auto pixels = makeBlankPixels(kSpriteSize, kSpriteSize, Rgba{0, 0, 0, 0});

  const unsigned cycle = frame % kWalkFrameCount;
  const int bob = (cycle == 1 || cycle == 2) ? 1 : ((cycle == 4 || cycle == 5) ? -1 : 0);
  const int swing = (cycle <= 2) ? static_cast<int>(cycle) : (5 - static_cast<int>(cycle));

  fillRect(pixels, 22, static_cast<unsigned>(16 + bob), 20, 12, kSpriteSize, kSpriteSize, Rgba{248, 224, 176, 255});
  fillRect(pixels, 22, static_cast<unsigned>(28 + bob), 20, 20, kSpriteSize, kSpriteSize, armor);
  fillRect(pixels, static_cast<unsigned>(16 - swing), static_cast<unsigned>(31 + bob), 6, 14, kSpriteSize, kSpriteSize,
           trim);
  fillRect(pixels, static_cast<unsigned>(42 + swing), static_cast<unsigned>(31 + bob), 6, 14, kSpriteSize, kSpriteSize,
           trim);

  const int leftLegShift = (cycle == 0 || cycle == 3) ? 0 : ((cycle < 3) ? -1 : 1);
  const int rightLegShift = -leftLegShift;
  fillRect(pixels, static_cast<unsigned>(24 + leftLegShift), static_cast<unsigned>(49 + bob), 6, 13, kSpriteSize,
           kSpriteSize, Rgba{72, 56, 108, 255});
  fillRect(pixels, static_cast<unsigned>(34 + rightLegShift), static_cast<unsigned>(49 + bob), 6, 13, kSpriteSize,
           kSpriteSize, Rgba{72, 56, 108, 255});

  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeRoundFrame(Rgba body, Rgba edge, unsigned frame) {
  auto pixels = makeBlankPixels(kSpriteSize, kSpriteSize, Rgba{0, 0, 0, 0});
  const unsigned cycle = frame % kWalkFrameCount;
  const int bob = (cycle == 1 || cycle == 2) ? 1 : ((cycle == 4 || cycle == 5) ? -1 : 0);

  const int cx = 32;
  const int cy = 32 + bob;
  const int radius = 20;

  for (int y = 0; y < static_cast<int>(kSpriteSize); ++y) {
    for (int x = 0; x < static_cast<int>(kSpriteSize); ++x) {
      const int dx = x - cx;
      const int dy = y - cy;
      const int d2 = dx * dx + dy * dy;
      if (d2 <= radius * radius) {
        setPixel(pixels, kSpriteSize, kSpriteSize, static_cast<unsigned>(x), static_cast<unsigned>(y), body);
      }
      if (d2 >= (radius - 1) * (radius - 1) && d2 <= radius * radius) {
        setPixel(pixels, kSpriteSize, kSpriteSize, static_cast<unsigned>(x), static_cast<unsigned>(y), edge);
      }
    }
  }

  const int eyeShift = (cycle <= 2) ? static_cast<int>(cycle) - 1 : 2 - static_cast<int>(cycle - 3);
  fillRect(pixels, static_cast<unsigned>(24 + eyeShift), static_cast<unsigned>(28 + bob), 4, 4, kSpriteSize,
           kSpriteSize, Rgba{20, 38, 60, 255});
  fillRect(pixels, static_cast<unsigned>(36 + eyeShift), static_cast<unsigned>(28 + bob), 4, 4, kSpriteSize,
           kSpriteSize, Rgba{20, 38, 60, 255});
  fillRect(pixels, 26, static_cast<unsigned>(38 + bob), 12, 4, kSpriteSize, kSpriteSize, Rgba{44, 78, 98, 255});

  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makePlayerSheetPixels() {
  auto sheet = makeBlankPixels(kSheetWidth, kSheetHeight, Rgba{0, 0, 0, 0});
  const std::array<SpriteSheetDirection, 4> directions = {SpriteSheetDirection::Front, SpriteSheetDirection::Left,
                                                           SpriteSheetDirection::Diagonal, SpriteSheetDirection::Back};

  for (const auto direction : directions) {
    const unsigned row = directionRowOffset(direction);
    const Rgba armor = direction == SpriteSheetDirection::Back
                           ? Rgba{78, 134, 208, 255}
                           : (direction == SpriteSheetDirection::Diagonal ? Rgba{96, 158, 235, 255}
                                                                          : Rgba{90, 150, 235, 255});
    for (unsigned col = 0; col < kSheetColumns; ++col) {
      auto frame = makeHumanoidFrame(armor, Rgba{54, 96, 160, 255}, frameForColumn(col));
      if (direction == SpriteSheetDirection::Left) {
        fillRect(frame, 8, 28, 7, 7, kSpriteSize, kSpriteSize, Rgba{255, 232, 112, 255});
      } else if (direction == SpriteSheetDirection::Diagonal) {
        fillRect(frame, 46, 24, 7, 7, kSpriteSize, kSpriteSize, Rgba{255, 232, 112, 255});
      } else if (direction == SpriteSheetDirection::Back) {
        fillRect(frame, 28, 6, 8, 8, kSpriteSize, kSpriteSize, Rgba{255, 232, 112, 255});
      } else {
        fillRect(frame, 28, 52, 8, 8, kSpriteSize, kSpriteSize, Rgba{255, 232, 112, 255});
      }
      blitFrame(sheet, col, row, frame);
    }
  }

  return sheet;
}

std::vector<sf::Uint8> SpriteManager::makeNpcSheetPixels() {
  auto sheet = makeBlankPixels(kSheetWidth, kSheetHeight, Rgba{0, 0, 0, 0});

  for (unsigned row = 0; row < kSheetRows; ++row) {
    for (unsigned col = 0; col < kSheetColumns; ++col) {
      auto frame = makeRoundFrame(Rgba{100, static_cast<sf::Uint8>(220 + row * 6), 245, 255}, Rgba{60, 100, 150, 255},
                                  frameForColumn(col));
      blitFrame(sheet, col, row, frame);
    }
  }

  return sheet;
}

std::vector<sf::Uint8> SpriteManager::makeMobSheetPixels() {
  auto sheet = makeBlankPixels(kSheetWidth, kSheetHeight, Rgba{0, 0, 0, 0});

  for (unsigned row = 0; row < kSheetRows; ++row) {
    for (unsigned col = 0; col < kSheetColumns; ++col) {
      auto frame = makeRoundFrame(Rgba{220, static_cast<sf::Uint8>(58 + row * 8), 58, 255}, Rgba{132, 26, 26, 255},
                                  frameForColumn(col));
      blitFrame(sheet, col, row, frame);
    }
  }

  return sheet;
}

std::vector<sf::Uint8> SpriteManager::makeTileGrassPixels() {
  auto pixels = makeBlankPixels(kSpriteSize, kSpriteSize, Rgba{70, 140, 62, 255});
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      if (((x + y) % 14) == 0) {
        setPixel(pixels, kSpriteSize, kSpriteSize, x, y, Rgba{86, 164, 76, 255});
      } else if (((x * 3 + y * 5) % 26) == 0) {
        setPixel(pixels, kSpriteSize, kSpriteSize, x, y, Rgba{58, 122, 54, 255});
      }
    }
  }
  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeTileWaterPixels() {
  auto pixels = makeBlankPixels(kSpriteSize, kSpriteSize, Rgba{52, 104, 176, 255});
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      if (((x + y * 2) % 16) < 4) {
        setPixel(pixels, kSpriteSize, kSpriteSize, x, y, Rgba{78, 140, 214, 255});
      }
      if (((x * 2 + y) % 22) == 0) {
        setPixel(pixels, kSpriteSize, kSpriteSize, x, y, Rgba{34, 84, 150, 255});
      }
    }
  }
  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeTileWallPixels() {
  auto pixels = makeBlankPixels(kSpriteSize, kSpriteSize, Rgba{114, 118, 124, 255});
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      if (y % 16 == 0 || x % 32 == 0) {
        setPixel(pixels, kSpriteSize, kSpriteSize, x, y, Rgba{90, 94, 102, 255});
      }
      if (((x + y) % 38) == 0) {
        setPixel(pixels, kSpriteSize, kSpriteSize, x, y, Rgba{136, 140, 146, 255});
      }
    }
  }
  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeTileForestPixels() {
  auto pixels = makeBlankPixels(kSpriteSize, kSpriteSize, Rgba{38, 96, 42, 255});
  for (unsigned y = 0; y < kSpriteSize; ++y) {
    for (unsigned x = 0; x < kSpriteSize; ++x) {
      if (((x * y) % 34) == 0) {
        setPixel(pixels, kSpriteSize, kSpriteSize, x, y, Rgba{28, 72, 31, 255});
      } else if (((x + y * 4) % 18) == 0) {
        setPixel(pixels, kSpriteSize, kSpriteSize, x, y, Rgba{52, 126, 58, 255});
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
            setPixel(pixels, kSpriteSize, kSpriteSize, static_cast<unsigned>(px), static_cast<unsigned>(py),
                     Rgba{18, 56, 22, 255});
          }
        }
      }
    }
  }

  return pixels;
}

std::vector<sf::Uint8> SpriteManager::makeMobPixels(bool alive) {
  auto pixels = makeBlankPixels(kSpriteSize, kSpriteSize, Rgba{0, 0, 0, 0});
  const Rgba body = alive ? Rgba{220, 58, 58, 255} : Rgba{95, 52, 52, 255};
  const Rgba edge = alive ? Rgba{132, 26, 26, 255} : Rgba{62, 34, 34, 255};

  for (unsigned y = 12; y <= 50; ++y) {
    const int row = static_cast<int>(y) - 12;
    const int halfWidth = std::max(2, 20 - row / 2);
    const int cx = 32;
    for (int x = cx - halfWidth; x <= cx + halfWidth; ++x) {
      setPixel(pixels, kSpriteSize, kSpriteSize, static_cast<unsigned>(x), y, body);
    }
    setPixel(pixels, kSpriteSize, kSpriteSize, static_cast<unsigned>(cx - halfWidth), y, edge);
    setPixel(pixels, kSpriteSize, kSpriteSize, static_cast<unsigned>(cx + halfWidth), y, edge);
  }

  fillRect(pixels, 24, 22, 4, 4, kSpriteSize, kSpriteSize, Rgba{255, 206, 206, 255});
  fillRect(pixels, 36, 22, 4, 4, kSpriteSize, kSpriteSize, Rgba{255, 206, 206, 255});

  return pixels;
}
