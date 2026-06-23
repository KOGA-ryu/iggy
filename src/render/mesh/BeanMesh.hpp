#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class BeanModelKind : std::uint8_t {
  Player,
  Npc,
  CodexProbe,
};

struct BeanMeshVertex {
  Vec3 position;
};

struct BeanMesh {
  BeanModelKind kind = BeanModelKind::Player;
  std::vector<BeanMeshVertex> vertices;
  std::vector<std::uint16_t> indices;
};

std::string_view beanModelId(BeanModelKind kind);
bool parseBeanModelId(std::string_view value, BeanModelKind& out);
Vec3 defaultBeanModelSize(BeanModelKind kind);
BeanMesh buildBeanMesh(BeanModelKind kind);

}  // namespace iggy3d
