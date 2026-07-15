#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

struct cgltf_data;

namespace iggy3d {

enum class StaticMeshCollisionMode : std::uint8_t {
  Bounds,
  CompoundBounds,
  None,
  Convex,
  Mesh,
  Invalid,
};

enum class StaticMeshCollisionPartMetadataStatus : std::uint8_t {
  NotAuthored,
  Authored,
  Invalid,
};

struct StaticMeshCollisionPartMetadata {
  StaticMeshCollisionPartMetadataStatus status =
      StaticMeshCollisionPartMetadataStatus::NotAuthored;
  std::string_view reasonCode = "static_mesh_collision_part_not_authored";
  bool walkable = false;
};

enum class StaticMeshAttachmentSocketRole : std::uint8_t {
  Receiver,
  Plug,
  Invalid,
};

enum class StaticMeshAttachmentSocketMetadataStatus : std::uint8_t {
  NotAuthored,
  Authored,
  Invalid,
};

struct StaticMeshAttachmentSocketMetadata {
  StaticMeshAttachmentSocketMetadataStatus status =
      StaticMeshAttachmentSocketMetadataStatus::NotAuthored;
  StaticMeshAttachmentSocketRole role =
      StaticMeshAttachmentSocketRole::Invalid;
  std::string name;
  std::string compatibility;
  std::string_view reasonCode = "static_mesh_attachment_socket_not_authored";
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

// Node-local contract for compound AABB assets. Other extras keys are ignored,
// but malformed JSON and partial collision-part declarations fail closed.
[[nodiscard]] StaticMeshCollisionPartMetadata
parseStaticMeshCollisionPartMetadata(std::string_view extrasObject) noexcept;

// Node-local contract. Position and orientation come from the node transform;
// extras only identify the socket, its role, and its compatibility family.
[[nodiscard]] StaticMeshAttachmentSocketMetadata
parseStaticMeshAttachmentSocketMetadata(std::string_view extrasObject) noexcept;

[[nodiscard]] StaticMeshAuthoringMetadata importStaticMeshAuthoringMetadata(
    const ::cgltf_data& data);

}  // namespace detail
}  // namespace iggy3d
