#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

struct cgltf_data;

namespace iggy3d {

enum class StaticMeshCollisionMode : std::uint8_t {
  Bounds,
  None,
  Convex,
  Mesh,
  Invalid,
};

enum class StaticMeshAuthoringMetadataStatus : std::uint8_t {
  DefaultsApplied,
  Authored,
  UnsupportedCollision,
  Invalid,
};

struct StaticMeshAuthoringMetadata {
  StaticMeshCollisionMode collisionMode = StaticMeshCollisionMode::Bounds;
  StaticMeshAuthoringMetadataStatus status =
      StaticMeshAuthoringMetadataStatus::DefaultsApplied;
  std::string categoryId;
  std::string_view reasonCode = "static_mesh_authoring_defaults_applied";
  bool walkable = false;
  bool collisionSpecified = false;
  bool walkableSpecified = false;
};

[[nodiscard]] std::string_view toString(
    StaticMeshCollisionMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    StaticMeshAuthoringMetadataStatus status) noexcept;

namespace detail {

// Pure bounded kernel used by the importer and focused tests. Each item is one
// glTF extras object from the asset, document, node, or mesh level.
[[nodiscard]] StaticMeshAuthoringMetadata parseStaticMeshAuthoringMetadata(
    std::span<const std::string_view> extrasObjects) noexcept;

[[nodiscard]] StaticMeshAuthoringMetadata importStaticMeshAuthoringMetadata(
    const ::cgltf_data& data);

}  // namespace detail
}  // namespace iggy3d
