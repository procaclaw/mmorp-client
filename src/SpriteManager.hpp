#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "WorldState.hpp"

enum class SpriteSheetDirection : std::uint8_t { Front = 0, Left = 1, Diagonal = 2, Back = 3 };

class SpriteManager {
 public:
  static constexpr unsigned kSpriteSize = 64;
  static constexpr unsigned kSheetColumns = 10;
  static constexpr unsigned kSheetRows = 4;
  static constexpr float kFrameWidth = 140.8f;
  static constexpr float kFrameHeight = 192.0f;
  static constexpr unsigned kIdleStartColumn = 0;
  static constexpr unsigned kIdleFrameCount = 4;
  static constexpr unsigned kWalkStartColumn = 4;
  static constexpr unsigned kWalkFrameCount = 6;
  static constexpr unsigned kAttackStartColumn = 10;
  static constexpr unsigned kAttackFrameCount = 6;
  static constexpr unsigned kHurtStartColumn = 16;
  static constexpr unsigned kHurtFrameCount = 4;
  static constexpr unsigned kDeathStartColumn = 20;
  static constexpr unsigned kDeathFrameCount = 2;
  static constexpr unsigned kSheetWidth = 1408;
  static constexpr unsigned kSheetHeight = 768;
  static constexpr unsigned kMonsterSheetHeight = 736;

  bool initialize(const std::string& spritesDirectory = "assets/sprites");

  const sf::Texture& tile(TileType tileType) const;
  const sf::Texture& playerSheet(const std::string& className) const;
  const sf::Texture& playerSheet() const;
  const sf::Texture& npcSheet() const;
  const sf::Texture& mobSheet() const;
  const sf::Texture& mobDead() const;

 private:
  struct Rgba {
    sf::Uint8 r = 0;
    sf::Uint8 g = 0;
    sf::Uint8 b = 0;
    sf::Uint8 a = 255;
  };

  bool loadTextureOrPlaceholder(const std::string& key, const std::string& fileName,
                                const std::vector<sf::Uint8>& fallbackPixels, unsigned width = kSpriteSize,
                                unsigned height = kSpriteSize, bool synthesizeBorderTransparency = true,
                                bool forceOpaqueAlpha = false);
  static std::vector<sf::Uint8> makeTileGrassPixels();
  static std::vector<sf::Uint8> makeTileWaterPixels();
  static std::vector<sf::Uint8> makeTileWallPixels();
  static std::vector<sf::Uint8> makeTileForestPixels();
  static std::vector<sf::Uint8> makePlayerSheetPixels();
  static std::vector<sf::Uint8> makeNpcSheetPixels();
  static std::vector<sf::Uint8> makeMobSheetPixels();
  static std::vector<sf::Uint8> makeMobPixels(bool alive);

  static std::vector<sf::Uint8> makeBlankPixels(unsigned width, unsigned height, Rgba color);
  static void setPixel(std::vector<sf::Uint8>& pixels, unsigned width, unsigned height, unsigned x, unsigned y,
                       Rgba color);
  static void fillRect(std::vector<sf::Uint8>& pixels, unsigned x, unsigned y, unsigned width,
                       unsigned height, unsigned canvasWidth, unsigned canvasHeight, Rgba color);
  static void blitFrame(std::vector<sf::Uint8>& sheet, unsigned frameX, unsigned frameY,
                        const std::vector<sf::Uint8>& framePixels);
  static std::vector<sf::Uint8> makeHumanoidFrame(Rgba armor, Rgba trim, unsigned frame);
  static std::vector<sf::Uint8> makeRoundFrame(Rgba body, Rgba edge, unsigned frame);
  static unsigned directionRowOffset(SpriteSheetDirection direction);

  std::string spritesDirectory_;
  std::unordered_map<std::string, sf::Texture> textures_;
};
