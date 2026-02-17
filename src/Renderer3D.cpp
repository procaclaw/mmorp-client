#include "Renderer3D.hpp"

#include <algorithm>
#include <cmath>

namespace {
float clamp01(float v) {
  return std::max(0.0f, std::min(1.0f, v));
}

int frameLeftForColumn(int column) {
  return static_cast<int>(std::lround(static_cast<float>(column) * SpriteManager::kFrameWidth));
}

int frameTopForRow(int row) {
  return static_cast<int>(std::lround(static_cast<float>(row) * SpriteManager::kFrameHeight));
}

int frameWidthForColumn(int column) {
  return std::max(1, frameLeftForColumn(column + 1) - frameLeftForColumn(column));
}

int frameHeightForRow(int row) {
  return std::max(1, frameTopForRow(row + 1) - frameTopForRow(row));
}

sf::IntRect spriteFrameRect(int column, int row) {
  return sf::IntRect(frameLeftForColumn(column), frameTopForRow(row), frameWidthForColumn(column),
                     frameHeightForRow(row));
}

void drawName(sf::RenderTarget& target, const sf::Font* font, const std::string& name, sf::Vector2f center,
              unsigned size, const sf::Color& color) {
  if (font == nullptr || name.empty()) {
    return;
  }
  sf::Text text;
  text.setFont(*font);
  text.setCharacterSize(size);
  text.setFillColor(color);
  text.setString(name);
  const auto bounds = text.getLocalBounds();
  text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
  text.setPosition(center.x, center.y);
  target.draw(text);
}
}  // namespace

void Renderer3D::initGL() {}

void Renderer3D::resize(int width, int height) {
  viewportWidth_ = std::max(1, width);
  viewportHeight_ = std::max(1, height);
}

void Renderer3D::setCameraZoom(float zoom) {
  cameraZoom_ = std::clamp(zoom, 0.25f, 1.0f);
}

float Renderer3D::cameraZoom() const {
  return cameraZoom_;
}

void Renderer3D::render(sf::RenderTarget& target, const WorldSnapshot& world, const sf::Font* font) {
  if (!spritesInitialized_) {
    spriteManager_.initialize();
    animationClock_.restart();
    playerDirectionCache_.clear();
    npcDirectionCache_.clear();
    mobDirectionCache_.clear();
    spritesInitialized_ = true;
  }

  sf::View worldView;
  worldView.setSize(static_cast<float>(viewportWidth_) * cameraZoom_, static_cast<float>(viewportHeight_) * cameraZoom_);

  float localPx = static_cast<float>(world.width * world.tileSize) * 0.5f;
  float localPy = static_cast<float>(world.height * world.tileSize) * 0.5f;
  auto localIt = world.players.find(world.localPlayerId);
  if (localIt != world.players.end()) {
    localPx = (localIt->second.renderX + 0.5f) * static_cast<float>(world.tileSize);
    localPy = (localIt->second.renderY + 0.5f) * static_cast<float>(world.tileSize);
  }

  const float worldPixelWidth = static_cast<float>(world.width * world.tileSize);
  const float worldPixelHeight = static_cast<float>(world.height * world.tileSize);
  const float halfW = worldView.getSize().x * 0.5f;
  const float halfH = worldView.getSize().y * 0.5f;
  const float cx = std::clamp(localPx, halfW, std::max(halfW, worldPixelWidth - halfW));
  const float cy = std::clamp(localPy, halfH, std::max(halfH, worldPixelHeight - halfH));
  worldView.setCenter(cx, cy);

  target.setView(worldView);
  drawTileLayer(target, world);
  drawGrid(target, world);
  drawEntities(target, world, font);
  drawMinimap(target, world);
  target.setView(target.getDefaultView());
}

void Renderer3D::drawTileLayer(sf::RenderTarget& target, const WorldSnapshot& world) const {
  sf::Sprite tile;
  sf::RenderStates states;
  states.blendMode = sf::BlendAlpha;
  const float tileScale = static_cast<float>(world.tileSize) / static_cast<float>(SpriteManager::kSpriteSize);
  tile.setScale(tileScale, tileScale);
  for (int y = 0; y < world.height; ++y) {
    for (int x = 0; x < world.width; ++x) {
      const TileType type = world.tiles[static_cast<std::size_t>(y * world.width + x)];
      tile.setTexture(spriteManager_.tile(type));
      tile.setPosition(static_cast<float>(x * world.tileSize), static_cast<float>(y * world.tileSize));
      target.draw(tile, states);
    }
  }
}

void Renderer3D::drawGrid(sf::RenderTarget& target, const WorldSnapshot& world) const {
  const float w = static_cast<float>(world.width * world.tileSize);
  const float h = static_cast<float>(world.height * world.tileSize);
  sf::VertexArray lines(sf::Lines);
  for (int x = 0; x <= world.width; ++x) {
    const float px = static_cast<float>(x * world.tileSize);
    lines.append(sf::Vertex(sf::Vector2f(px, 0.0f), sf::Color(0, 0, 0, 32)));
    lines.append(sf::Vertex(sf::Vector2f(px, h), sf::Color(0, 0, 0, 32)));
  }
  for (int y = 0; y <= world.height; ++y) {
    const float py = static_cast<float>(y * world.tileSize);
    lines.append(sf::Vertex(sf::Vector2f(0.0f, py), sf::Color(0, 0, 0, 32)));
    lines.append(sf::Vertex(sf::Vector2f(w, py), sf::Color(0, 0, 0, 32)));
  }
  target.draw(lines);
}

void Renderer3D::drawEntities(sf::RenderTarget& target, const WorldSnapshot& world, const sf::Font* font) const {
  sf::Sprite sprite;
  sf::RenderStates states;
  states.blendMode = sf::BlendAlpha;

  for (const auto& [_, npc] : world.npcs) {
    const float dx = npc.renderX - static_cast<float>(npc.x);
    const float dy = npc.renderY - static_cast<float>(npc.y);
    const bool moving = isMoving(dx, dy);
    const auto direction = resolveDirection(npc.id, npc.renderX, npc.renderY, npc.x, npc.y, npcDirectionCache_);

    const sf::Vector2f center((npc.renderX + 0.5f) * static_cast<float>(world.tileSize),
                              (npc.renderY + 0.5f) * static_cast<float>(world.tileSize));
    sprite.setTexture(spriteManager_.npcSheet());
    const sf::IntRect frameRect = spriteFrameRect(animationColumn(moving), rowForDirection(direction));
    sprite.setTextureRect(frameRect);
    sprite.setOrigin(static_cast<float>(frameRect.width) * 0.5f, static_cast<float>(frameRect.height) * 0.5f);
    constexpr float npcSize = 22.0f;
    const float npcScale = npcSize / static_cast<float>(frameRect.width);
    sprite.setScale(npcScale, npcScale);
    sprite.setColor(sf::Color::White);
    sprite.setPosition(center);
    target.draw(sprite, states);
    drawName(target, font, npc.name, sf::Vector2f(center.x, center.y - 16.0f), 12, sf::Color::White);
  }

  for (const auto& [_, mob] : world.mobs) {
    const float dx = mob.renderX - static_cast<float>(mob.x);
    const float dy = mob.renderY - static_cast<float>(mob.y);
    const bool moving = isMoving(dx, dy);
    const auto direction = resolveDirection(mob.id, mob.renderX, mob.renderY, mob.x, mob.y, mobDirectionCache_);

    const sf::Vector2f center((mob.renderX + 0.5f) * static_cast<float>(world.tileSize),
                              (mob.renderY + 0.5f) * static_cast<float>(world.tileSize));
    sprite.setTexture(spriteManager_.mobSheet());
    const sf::IntRect frameRect = spriteFrameRect(animationColumn(moving && mob.alive), rowForDirection(direction));
    sprite.setTextureRect(frameRect);
    sprite.setOrigin(static_cast<float>(frameRect.width) * 0.5f, static_cast<float>(frameRect.height) * 0.5f);
    constexpr float mobSize = 20.0f;
    const float mobScale = mobSize / static_cast<float>(frameRect.width);
    sprite.setScale(mobScale, mobScale);
    sprite.setColor(mob.alive ? sf::Color::White : sf::Color(122, 122, 122));
    sprite.setPosition(center);
    target.draw(sprite, states);
    if (mob.hp < mob.maxHp) {
      drawHealthBar(target, sf::Vector2f(center.x, center.y - 16.0f), 22.0f,
                    static_cast<float>(mob.hp) / std::max(1.0f, static_cast<float>(mob.maxHp)));
    }
  }

  for (const auto& [id, player] : world.players) {
    const bool isSelf = id == world.localPlayerId;
    const float dx = player.renderX - static_cast<float>(player.x);
    const float dy = player.renderY - static_cast<float>(player.y);
    const bool moving = isMoving(dx, dy);
    const auto direction = resolveDirection(player.id, player.renderX, player.renderY, player.x, player.y,
                                            playerDirectionCache_);

    const sf::Vector2f center((player.renderX + 0.5f) * static_cast<float>(world.tileSize),
                              (player.renderY + 0.5f) * static_cast<float>(world.tileSize));
    sprite.setTexture(spriteManager_.playerSheet());
    const sf::IntRect frameRect = spriteFrameRect(animationColumn(moving), rowForDirection(direction));
    sprite.setTextureRect(frameRect);
    sprite.setOrigin(static_cast<float>(frameRect.width) * 0.5f, static_cast<float>(frameRect.height) * 0.5f);
    constexpr float playerSize = 24.0f;
    const float playerScale = playerSize / static_cast<float>(frameRect.width);
    sprite.setScale(playerScale, playerScale);
    sprite.setColor(isSelf ? sf::Color(255, 236, 122) : sf::Color(235, 235, 255));
    sprite.setPosition(center);
    target.draw(sprite, states);

    drawHealthBar(target, sf::Vector2f(center.x, center.y - 19.0f), 28.0f,
                  static_cast<float>(player.hp) / std::max(1.0f, static_cast<float>(player.maxHp)));
    drawName(target, font, player.name.empty() ? player.id : player.name,
             sf::Vector2f(center.x, center.y - 32.0f), 12, isSelf ? sf::Color(255, 235, 120) : sf::Color::White);
  }

  if (font == nullptr) {
    return;
  }
  for (const auto& fx : world.combatTexts) {
    sf::Text text;
    text.setFont(*font);
    text.setCharacterSize(14);
    text.setFillColor(sf::Color(fx.r, fx.g, fx.b, static_cast<sf::Uint8>(255.0f * clamp01(fx.ttl))));
    text.setString(fx.text);
    text.setPosition((fx.worldX + 0.5f) * static_cast<float>(world.tileSize) - 8.0f,
                     (fx.worldY + 0.5f) * static_cast<float>(world.tileSize) - (1.2f - fx.ttl) * 26.0f);
    target.draw(text);
  }
}

SpriteSheetDirection Renderer3D::directionFromDelta(float dx, float dy) {
  constexpr float kThreshold = 0.05f;
  if (std::abs(dx) > kThreshold && std::abs(dy) > kThreshold) {
    return SpriteSheetDirection::Diagonal;
  }
  if (dy < -kThreshold) {
    return SpriteSheetDirection::Back;
  }
  if (dy > kThreshold) {
    return SpriteSheetDirection::Front;
  }
  if (dx < -kThreshold) {
    return SpriteSheetDirection::Left;
  }
  if (dx > kThreshold) {
    return SpriteSheetDirection::Diagonal;
  }
  return SpriteSheetDirection::Front;
}

bool Renderer3D::isMoving(float dx, float dy) {
  constexpr float kThreshold = 0.05f;
  return std::abs(dx) > kThreshold || std::abs(dy) > kThreshold;
}

SpriteSheetDirection Renderer3D::resolveDirection(const std::string& id, float renderX, float renderY, int gridX,
                                                  int gridY,
                                                  std::unordered_map<std::string, SpriteSheetDirection>& cache) const {
  const float dx = renderX - static_cast<float>(gridX);
  const float dy = renderY - static_cast<float>(gridY);
  if (!isMoving(dx, dy)) {
    const auto it = cache.find(id);
    if (it != cache.end()) {
      return it->second;
    }
    return SpriteSheetDirection::Front;
  }

  const auto direction = directionFromDelta(dx, dy);
  cache[id] = direction;
  return direction;
}

int Renderer3D::animationColumn(bool moving) const {
  if (!moving) {
    return static_cast<int>(SpriteManager::kIdleStartColumn);
  }

  const auto elapsedMs = animationClock_.getElapsedTime().asMilliseconds();
  const auto frame = static_cast<int>((elapsedMs / kAnimationFrameMs) %
                                      static_cast<sf::Int64>(SpriteManager::kWalkFrameCount));
  return static_cast<int>(SpriteManager::kWalkStartColumn) + frame;
}

int Renderer3D::rowForDirection(SpriteSheetDirection direction) {
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

void Renderer3D::drawMinimap(sf::RenderTarget& target, const WorldSnapshot& world) const {
  const sf::View previousView = target.getView();
  target.setView(target.getDefaultView());
  const sf::View defaultView = target.getDefaultView();
  const auto size = defaultView.getSize();
  const auto center = defaultView.getCenter();
  const float topLeftX = center.x - size.x * 0.5f;
  const float topLeftY = center.y - size.y * 0.5f;

  const float miniSize = 170.0f;
  const sf::FloatRect mini(topLeftX + size.x - miniSize - 16.0f, topLeftY + 16.0f, miniSize, miniSize);
  sf::RectangleShape bg(sf::Vector2f(mini.width, mini.height));
  bg.setPosition(mini.left, mini.top);
  bg.setFillColor(sf::Color(16, 19, 28, 170));
  bg.setOutlineColor(sf::Color(215, 220, 235, 140));
  bg.setOutlineThickness(1.0f);
  target.draw(bg);

  const float sx = mini.width / static_cast<float>(std::max(1, world.width));
  const float sy = mini.height / static_cast<float>(std::max(1, world.height));

  sf::RectangleShape dot(sf::Vector2f(std::max(1.0f, sx), std::max(1.0f, sy)));
  for (const auto& [id, p] : world.players) {
    dot.setFillColor(id == world.localPlayerId ? sf::Color(255, 228, 107) : sf::Color(227, 231, 255));
    dot.setPosition(mini.left + p.renderX * sx, mini.top + p.renderY * sy);
    target.draw(dot);
  }
  for (const auto& [_, m] : world.mobs) {
    dot.setFillColor(sf::Color(235, 86, 86));
    dot.setPosition(mini.left + m.renderX * sx, mini.top + m.renderY * sy);
    target.draw(dot);
  }
  target.setView(previousView);
}

void Renderer3D::drawHealthBar(sf::RenderTarget& target, sf::Vector2f center, float width,
                               float fillRatio) const {
  const float h = 4.0f;
  sf::RectangleShape bg(sf::Vector2f(width, h));
  bg.setPosition(center.x - width * 0.5f, center.y);
  bg.setFillColor(sf::Color(45, 12, 12, 215));
  target.draw(bg);

  sf::RectangleShape fg(sf::Vector2f(width * clamp01(fillRatio), h));
  fg.setPosition(center.x - width * 0.5f, center.y);
  fg.setFillColor(sf::Color(78, 220, 86));
  target.draw(fg);
}
