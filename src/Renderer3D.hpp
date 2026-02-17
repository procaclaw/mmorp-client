#pragma once

#include <SFML/Graphics.hpp>

#include "SpriteManager.hpp"
#include "WorldState.hpp"

class Renderer3D {
 public:
  void initGL();
  void resize(int width, int height);
  void render(sf::RenderTarget& target, const WorldSnapshot& world, const sf::Font* font = nullptr);

 private:
  void drawTileLayer(sf::RenderTarget& target, const WorldSnapshot& world) const;
  void drawGrid(sf::RenderTarget& target, const WorldSnapshot& world) const;
  void drawEntities(sf::RenderTarget& target, const WorldSnapshot& world, const sf::Font* font) const;
  void drawMinimap(sf::RenderTarget& target, const WorldSnapshot& world) const;
  void drawHealthBar(sf::RenderTarget& target, sf::Vector2f center, float width, float fillRatio) const;
  static FacingDirection directionForPlayer(const PlayerState& player);

  int viewportWidth_ = 1280;
  int viewportHeight_ = 720;
  mutable SpriteManager spriteManager_;
  mutable bool spritesInitialized_ = false;
};
