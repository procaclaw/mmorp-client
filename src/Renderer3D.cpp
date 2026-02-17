#include "Renderer3D.hpp"

#include <algorithm>

namespace {
sf::Color tileColor(TileType t) {
  switch (t) {
    case TileType::Grass:
      return sf::Color(78, 148, 73);
    case TileType::Water:
      return sf::Color(56, 116, 191);
    case TileType::Wall:
      return sf::Color(120, 122, 126);
    case TileType::Forest:
      return sf::Color(38, 98, 40);
  }
  return sf::Color(100, 120, 100);
}

float clamp01(float v) {
  return std::max(0.0f, std::min(1.0f, v));
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

void Renderer3D::render(sf::RenderTarget& target, const WorldSnapshot& world, const sf::Font* font) {
  sf::View worldView;
  worldView.setSize(static_cast<float>(viewportWidth_), static_cast<float>(viewportHeight_));

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
  sf::RectangleShape tile;
  tile.setSize(sf::Vector2f(static_cast<float>(world.tileSize), static_cast<float>(world.tileSize)));
  for (int y = 0; y < world.height; ++y) {
    for (int x = 0; x < world.width; ++x) {
      const TileType type = world.tiles[static_cast<std::size_t>(y * world.width + x)];
      tile.setFillColor(tileColor(type));
      tile.setPosition(static_cast<float>(x * world.tileSize), static_cast<float>(y * world.tileSize));
      target.draw(tile);
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
  for (const auto& [_, npc] : world.npcs) {
    sf::CircleShape body(9.0f);
    body.setFillColor(sf::Color(100, 240, 245));
    body.setOutlineColor(sf::Color(60, 100, 150));
    body.setOutlineThickness(2.0f);
    body.setOrigin(9.0f, 9.0f);
    const sf::Vector2f center((npc.renderX + 0.5f) * static_cast<float>(world.tileSize),
                              (npc.renderY + 0.5f) * static_cast<float>(world.tileSize));
    body.setPosition(center);
    target.draw(body);
    drawName(target, font, npc.name, sf::Vector2f(center.x, center.y - 16.0f), 12, sf::Color::White);
  }

  for (const auto& [_, mob] : world.mobs) {
    sf::CircleShape body(10.0f, 3);
    body.setFillColor(mob.alive ? sf::Color(220, 58, 58) : sf::Color(95, 52, 52));
    body.setOrigin(10.0f, 10.0f);
    const sf::Vector2f center((mob.renderX + 0.5f) * static_cast<float>(world.tileSize),
                              (mob.renderY + 0.5f) * static_cast<float>(world.tileSize));
    body.setPosition(center);
    target.draw(body);
    if (mob.hp < mob.maxHp) {
      drawHealthBar(target, sf::Vector2f(center.x, center.y - 16.0f), 22.0f,
                    static_cast<float>(mob.hp) / std::max(1.0f, static_cast<float>(mob.maxHp)));
    }
  }

  for (const auto& [id, player] : world.players) {
    const bool isSelf = id == world.localPlayerId;
    sf::CircleShape body(11.0f);
    body.setFillColor(isSelf ? sf::Color(240, 217, 88) : sf::Color(230, 230, 255));
    body.setOutlineColor(isSelf ? sf::Color(255, 166, 45) : sf::Color(120, 122, 160));
    body.setOutlineThickness(2.0f);
    body.setOrigin(11.0f, 11.0f);
    const sf::Vector2f center((player.renderX + 0.5f) * static_cast<float>(world.tileSize),
                              (player.renderY + 0.5f) * static_cast<float>(world.tileSize));
    body.setPosition(center);
    target.draw(body);

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
