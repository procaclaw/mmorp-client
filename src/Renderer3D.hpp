#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <unordered_map>

#include "SpriteManager.hpp"
#include "WorldState.hpp"

class Renderer3D {
 public:
  void initGL();
  void resize(int width, int height);
  void setCameraZoom(float zoom);
  float cameraZoom() const;
  void render(sf::RenderTarget& target, const WorldSnapshot& world, const sf::Font* font = nullptr);

 private:
  void drawTileLayer(sf::RenderTarget& target, const WorldSnapshot& world) const;
  void drawGrid(sf::RenderTarget& target, const WorldSnapshot& world) const;
  void drawEntities(sf::RenderTarget& target, const WorldSnapshot& world, const sf::Font* font) const;
  void drawMinimap(sf::RenderTarget& target, const WorldSnapshot& world) const;
  void drawHealthBar(sf::RenderTarget& target, sf::Vector2f center, float width, float fillRatio) const;
  static SpriteSheetDirection directionFromDelta(float dx, float dy);
  static bool isMoving(float dx, float dy);
  SpriteSheetDirection resolveDirection(const std::string& id, float renderX, float renderY, int gridX, int gridY,
                                        std::unordered_map<std::string, SpriteSheetDirection>& cache) const;
  int animationColumn(bool moving) const;
  static int rowForDirection(SpriteSheetDirection direction);

  int viewportWidth_ = 1280;
  int viewportHeight_ = 720;
  float cameraZoom_ = 0.75f;
  static constexpr int kAnimationFrameMs = 120;
  mutable SpriteManager spriteManager_;
  mutable bool spritesInitialized_ = false;
  mutable sf::Clock animationClock_;
  mutable std::unordered_map<std::string, SpriteSheetDirection> playerDirectionCache_;
  mutable std::unordered_map<std::string, SpriteSheetDirection> npcDirectionCache_;
  mutable std::unordered_map<std::string, SpriteSheetDirection> mobDirectionCache_;
};
