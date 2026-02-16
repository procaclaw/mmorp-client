#include "Renderer3D.hpp"

#include <SFML/OpenGL.hpp>

#include <cmath>

namespace {
constexpr float kPi = 3.1415926535f;

void perspective(float fovYDeg, float aspect, float zNear, float zFar) {
  const float f = 1.0f / std::tan((fovYDeg * kPi / 180.0f) * 0.5f);
  const float m[16] = {
      f / aspect, 0, 0, 0,
      0, f, 0, 0,
      0, 0, (zFar + zNear) / (zNear - zFar), -1,
      0, 0, (2 * zFar * zNear) / (zNear - zFar), 0,
  };
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(m);
}

void lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
  const Vec3 f = normalize(center - eye);
  const Vec3 s = normalize(cross(f, up));
  const Vec3 u = cross(s, f);

  const float m[16] = {
      s.x, u.x, -f.x, 0,
      s.y, u.y, -f.y, 0,
      s.z, u.z, -f.z, 0,
      -dot(s, eye), -dot(u, eye), dot(f, eye), 1,
  };

  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(m);
}
}  // namespace

void Renderer3D::initGL() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_COLOR_MATERIAL);
  glShadeModel(GL_SMOOTH);
}

void Renderer3D::resize(int width, int height) {
  if (height == 0) {
    height = 1;
  }
  glViewport(0, 0, width, height);
  perspective(60.0f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 200.0f);
}

void Renderer3D::setColor(const sf::Color& c) {
  glColor3f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
}

void Renderer3D::setCamera(const Vec3& target) {
  const Vec3 cameraOffset{-5.0f, 4.0f, -5.0f};
  const Vec3 eye = target + cameraOffset;
  lookAt(eye, target, Vec3{0.0f, 1.0f, 0.0f});
}

void Renderer3D::drawPlane(float size) {
  const float hs = size * 0.5f;
  glBegin(GL_QUADS);
  glNormal3f(0.0f, 1.0f, 0.0f);
  glVertex3f(-hs, 0.0f, -hs);
  glVertex3f(hs, 0.0f, -hs);
  glVertex3f(hs, 0.0f, hs);
  glVertex3f(-hs, 0.0f, hs);
  glEnd();
}

void Renderer3D::drawCube(float size) {
  const float hs = size * 0.5f;
  glBegin(GL_QUADS);

  glNormal3f(0, 0, 1);
  glVertex3f(-hs, -hs, hs);
  glVertex3f(hs, -hs, hs);
  glVertex3f(hs, hs, hs);
  glVertex3f(-hs, hs, hs);

  glNormal3f(0, 0, -1);
  glVertex3f(hs, -hs, -hs);
  glVertex3f(-hs, -hs, -hs);
  glVertex3f(-hs, hs, -hs);
  glVertex3f(hs, hs, -hs);

  glNormal3f(1, 0, 0);
  glVertex3f(hs, -hs, hs);
  glVertex3f(hs, -hs, -hs);
  glVertex3f(hs, hs, -hs);
  glVertex3f(hs, hs, hs);

  glNormal3f(-1, 0, 0);
  glVertex3f(-hs, -hs, -hs);
  glVertex3f(-hs, -hs, hs);
  glVertex3f(-hs, hs, hs);
  glVertex3f(-hs, hs, -hs);

  glNormal3f(0, 1, 0);
  glVertex3f(-hs, hs, hs);
  glVertex3f(hs, hs, hs);
  glVertex3f(hs, hs, -hs);
  glVertex3f(-hs, hs, -hs);

  glNormal3f(0, -1, 0);
  glVertex3f(-hs, -hs, -hs);
  glVertex3f(hs, -hs, -hs);
  glVertex3f(hs, -hs, hs);
  glVertex3f(-hs, -hs, hs);

  glEnd();
}

void Renderer3D::drawSphere(float radius, int stacks, int slices) {
  for (int i = 0; i < stacks; ++i) {
    const float lat0 = kPi * (-0.5f + static_cast<float>(i) / static_cast<float>(stacks));
    const float z0 = std::sin(lat0) * radius;
    const float zr0 = std::cos(lat0) * radius;

    const float lat1 = kPi * (-0.5f + static_cast<float>(i + 1) / static_cast<float>(stacks));
    const float z1 = std::sin(lat1) * radius;
    const float zr1 = std::cos(lat1) * radius;

    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; ++j) {
      const float lng = 2.0f * kPi * static_cast<float>(j) / static_cast<float>(slices);
      const float x = std::cos(lng);
      const float y = std::sin(lng);

      glNormal3f(x * zr0 / radius, y * zr0 / radius, z0 / radius);
      glVertex3f(x * zr0, y * zr0, z0);

      glNormal3f(x * zr1 / radius, y * zr1 / radius, z1 / radius);
      glVertex3f(x * zr1, y * zr1, z1);
    }
    glEnd();
  }
}

void Renderer3D::render(const WorldState& world) {
  glClearColor(0.11f, 0.12f, 0.16f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  setCamera(world.localPlayer.position + Vec3{0.0f, 1.0f, 0.0f});

  setColor(sf::Color(64, 110, 66));
  drawPlane(60.0f);

  setColor(sf::Color(90, 90, 120));
  for (int i = -3; i <= 3; i += 3) {
    glPushMatrix();
    glTranslatef(static_cast<float>(i), 0.4f, -8.0f);
    drawCube(0.8f);
    glPopMatrix();
  }

  setColor(sf::Color(60, 170, 255));
  glPushMatrix();
  glTranslatef(world.localPlayer.position.x, world.localPlayer.position.y, world.localPlayer.position.z);
  drawCube(1.0f);
  glPopMatrix();

  setColor(sf::Color(255, 180, 80));
  for (const auto& [id, p] : world.remotePlayers) {
    (void)id;
    glPushMatrix();
    glTranslatef(p.position.x, p.position.y, p.position.z);
    drawSphere(0.55f, 12, 16);
    glPopMatrix();
  }
}
