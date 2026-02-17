#pragma once

#include <cmath>

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  Vec3() = default;
  Vec3(float xIn, float yIn, float zIn) : x(xIn), y(yIn), z(zIn) {}

  Vec3 operator+(const Vec3& rhs) const { return Vec3{x + rhs.x, y + rhs.y, z + rhs.z}; }
  Vec3 operator-(const Vec3& rhs) const { return Vec3{x - rhs.x, y - rhs.y, z - rhs.z}; }
  Vec3 operator*(float s) const { return Vec3{x * s, y * s, z * s}; }
  Vec3& operator+=(const Vec3& rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }
};

inline float dot(const Vec3& a, const Vec3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
  return Vec3{
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

inline float length(const Vec3& v) {
  return std::sqrt(dot(v, v));
}

inline Vec3 normalize(const Vec3& v) {
  const float len = length(v);
  if (len <= 1e-6f) {
    return Vec3{0.0f, 0.0f, 0.0f};
  }
  return v * (1.0f / len);
}
