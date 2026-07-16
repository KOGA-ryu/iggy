#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"

namespace iggy3d {

enum class PhysicsSpatialSurfaceColliderBakeStatus : std::uint8_t {
  Baked,
  MissingSurfaceSet,
  InvalidConfig,
  InvalidSurfaceBounds,
  InvalidSurfaceShape,
  InvalidGeneratedCollider,
};

struct PhysicsSpatialSurfaceColliderBakeConfig {
  // Fallback extrusion for plane surfaces that carry no authored collision
  // thickness. Bounds-backed Creative floors provide their resolved thickness.
  float planeThicknessMeters = 0.10F;
  float minHalfExtentMeters = 0.001F;
  PhysicsBodyId firstGeneratedBodyId{1U};
  bool includeWalkable = true;
  bool includeActorBlockers = true;
  bool includeProjectileBlockers = false;
  bool includeOpenings = false;
  bool includeSensors = false;
};

struct PhysicsSpatialSurfaceColliderBakeRequest {
  const SpatialSurfaceSet* surfaces = nullptr;
  PhysicsSpatialSurfaceColliderBakeConfig config;
};

struct PhysicsSpatialSurfaceColliderBakeResult {
  bool ok = false;
  PhysicsSpatialSurfaceColliderBakeStatus status =
      PhysicsSpatialSurfaceColliderBakeStatus::MissingSurfaceSet;
  std::string_view reasonCode =
      "physics_spatial_surface_bake_missing_surface_set";
  std::size_t surfaceCount = 0U;
  std::size_t includedSurfaceCount = 0U;
  std::size_t skippedSurfaceCount = 0U;
  std::size_t colliderCount = 0U;
  std::size_t invalidSurfaceIndex = 0U;
  std::vector<PhysicsAabbCollider> colliders;
  std::vector<std::size_t> sourceSurfaceIndices;
  std::vector<std::string> sourceSurfaceIds;
  std::vector<CollisionSurfaceRole> sourceRoles;
  std::vector<CollisionSurfaceShape> sourceShapes;
  std::vector<std::string> runtimeOwnerStableNames;
};

std::string_view physicsSpatialSurfaceColliderBakeStatusName(
    PhysicsSpatialSurfaceColliderBakeStatus status);
bool isValidPhysicsSpatialSurfaceColliderBakeConfig(
    const PhysicsSpatialSurfaceColliderBakeConfig& config);
PhysicsSpatialSurfaceColliderBakeResult
bakePhysicsAabbCollidersFromSpatialSurfaces(
    const PhysicsSpatialSurfaceColliderBakeRequest& request);

}  // namespace iggy3d
