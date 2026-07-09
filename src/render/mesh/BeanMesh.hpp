#pragma once

#include <cstdint>
#include <vector>

#include "core/math/Vec3.hpp"
#include "projection/scene/SceneModel.hpp"

namespace iggy3d {

struct BeanMeshVertex {
  Vec3 position;
};

struct BeanMesh {
  SceneModelKind kind = SceneModelKind::PlayerBean;
  std::vector<BeanMeshVertex> vertices;
  std::vector<std::uint16_t> indices;
};

Vec3 defaultBeanModelSize(SceneModelKind kind);
BeanMesh buildBeanMesh(SceneModelKind kind);

}  // namespace iggy3d
