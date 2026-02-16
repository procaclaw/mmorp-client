#pragma once

#include <SFML/Graphics/Color.hpp>

#include "Math3D.hpp"
#include "WorldState.hpp"

class Renderer3D {
 public:
  void initGL();
  void resize(int width, int height);
  void render(const WorldState& world);

 private:
  void setCamera(const Vec3& target);
  void drawPlane(float size);
  void drawCube(float size);
  void drawSphere(float radius, int stacks, int slices);
  void setColor(const sf::Color& c);
};
