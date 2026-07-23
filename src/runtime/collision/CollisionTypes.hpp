#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class CollisionSurfaceShape : std::uint8_t {
  Box,
  Plane,
  HeightPatch,
  Opening,
};

enum class CollisionSurfaceRole : std::uint8_t {
  Walkable,
  Blocker,
  ProjectileBlocker,
  Opening,
};

enum class CollisionQueryStatus : std::uint8_t {
  Hit,
  NoHit,
  InvalidInput,
  EmptySurfaceSet,
};

enum class CollisionQueryKind : std::uint8_t {
  All,
  Actor,
  Projectile,
  Walkable,
  Opening,
};

struct CollisionSurfaceView {
  std::string id;
  CollisionSurfaceShape shape = CollisionSurfaceShape::Plane;
  CollisionSurfaceRole role = CollisionSurfaceRole::Walkable;
  Aabb3 bounds;
  Vec3 normal;
  Vec3 planePoint;
  // HeightPatch uses center followed by four counter-clockwise corners.
  std::array<Vec3, 5U> heightPatchPoints{};
  bool blocksActor = false;
  bool blocksProjectile = false;
  bool blocksVision = true;
  bool hasActorMask = false;
  bool hasProjectileMask = false;
  bool opening = false;
  float collisionThicknessMeters = 0.0F;
  std::string runtimeOwnerStableName;
  std::vector<std::string> traversalTags;
};

struct CollisionQueryResult {
  CollisionQueryStatus status = CollisionQueryStatus::NoHit;
  std::string surfaceId;
  CollisionSurfaceRole role = CollisionSurfaceRole::Walkable;
  CollisionSurfaceShape shape = CollisionSurfaceShape::Plane;
  Vec3 pointMeters;
  Vec3 normal;
  float distanceMeters = 0.0F;
  float timeOfImpact = 0.0F;
  float heightMeters = 0.0F;
  std::size_t checkedSurfaceCount = 0;
  std::size_t blockingSurfaceCount = 0;
  std::string reasonCode = "collision_no_hit";
};

std::string_view collisionQueryStatusName(CollisionQueryStatus status);
std::string_view collisionSurfaceRoleName(CollisionSurfaceRole role);
std::string_view collisionSurfaceShapeName(CollisionSurfaceShape shape);

}  // namespace iggy3d
