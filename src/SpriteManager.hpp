#pragma once

#include <SFML/Graphics.hpp>

#include <string>
#include <unordered_map>
#include <vector>

#include "WorldState.hpp"

enum class FacingDirection { North, East, South, West };

class SpriteManager {
 public:
  static constexpr unsigned kSpriteSize = 64;

  bool initialize(const std::string& spritesDirectory = "assets/sprites");

  const sf::Texture& tile(TileType tileType) const;
  const sf::Texture& player(FacingDirection direction) const;
  const sf::Texture& npc() const;
  const sf::Texture& mob(bool alive) const;

 private:
  struct Rgba {
    sf::Uint8 r = 0;
    sf::Uint8 g = 0;
    sf::Uint8 b = 0;
    sf::Uint8 a = 255;
  };

  bool loadTextureOrPlaceholder(const std::string& key, const std::string& fileName,
                                const std::vector<sf::Uint8>& fallbackPixels);
  static std::vector<sf::Uint8> makeTileGrassPixels();
  static std::vector<sf::Uint8> makeTileWaterPixels();
  static std::vector<sf::Uint8> makeTileWallPixels();
  static std::vector<sf::Uint8> makeTileForestPixels();
  static std::vector<sf::Uint8> makePlayerPixels(FacingDirection direction);
  static std::vector<sf::Uint8> makeNpcPixels();
  static std::vector<sf::Uint8> makeMobPixels(bool alive);

  static std::vector<sf::Uint8> makeBlankPixels(Rgba color);
  static void setPixel(std::vector<sf::Uint8>& pixels, unsigned x, unsigned y, Rgba color);
  static void fillRect(std::vector<sf::Uint8>& pixels, unsigned x, unsigned y, unsigned width,
                       unsigned height, Rgba color);

  std::string spritesDirectory_;
  std::unordered_map<std::string, sf::Texture> textures_;
};
