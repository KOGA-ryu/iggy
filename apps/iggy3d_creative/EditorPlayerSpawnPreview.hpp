#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/play/PlayerSpawn.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorOverlayFrame;
struct CreativeEditorOverlayFrameRequest;

enum class CreativePlayerSpawnPreviewLineRole : std::uint8_t {
  PhysicalEnvelope,
  GroundClearance,
  FloorContact,
  CameraHeight,
  Facing,
  GroundingOffset,
  Count,
};

struct CreativePlayerSpawnPreviewLine {
  iggy3d::Vec3 start;
  iggy3d::Vec3 end;
  CreativePlayerSpawnPreviewLineRole role =
      CreativePlayerSpawnPreviewLineRole::PhysicalEnvelope;
};

inline constexpr std::size_t kCreativePlayerSpawnPreviewLineCapacity = 40U;

struct CreativePlayerSpawnPreviewGeometry {
  bool active = false;
  bool accepted = false;
  bool capacityExceeded = false;
  iggy3d::creative::CreativePlayerSpawnStatus status =
      iggy3d::creative::CreativePlayerSpawnStatus::NotRequested;
  std::string_view reasonCode = "creative_player_spawn_not_requested";
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::Vec3 authoredPositionMeters;
  iggy3d::Vec3 groundedPositionMeters;
  iggy3d::Vec3 cameraPositionMeters;
  iggy3d::Vec3 facingDirection;
  float clearanceRadiusMeters = 0.0F;
  float bodyHeightMeters = 0.0F;
  std::array<CreativePlayerSpawnPreviewLine,
             kCreativePlayerSpawnPreviewLineCapacity>
      lines{};
  std::size_t lineCount = 0U;
};

struct CreativePlayerSpawnPreviewCache {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  std::uint64_t documentRevision = 0U;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  const iggy3d::creative::CreativeRoomBakeResult* roomBakeIdentity = nullptr;
  std::uint64_t roomBakeRevision = 0U;
  std::uint64_t refreshCount = 0U;
  CreativePlayerSpawnPreviewGeometry geometry;
  bool valid = false;
};

struct CreativePlayerSpawnPlanSymbol {
  bool drawable = false;
  bool runtimeReady = false;
  iggy3d::creative::CreativeVec3 centerCells;
  iggy3d::creative::CreativeVec3 facingEndCells;
  double bodyRadiusCells = 0.0;
  double clearanceRadiusCells = 0.0;
};

// Produces bounded transient geometry from the same physical plan consumed by
// validation and play activation. Rejected spawns remain visible at their
// authored position so their failure can be repaired in place.
[[nodiscard]] CreativePlayerSpawnPreviewGeometry
planCreativePlayerSpawnPreview(
    const iggy3d::creative::CreativePlayerSpawnPlanRequest& request);
[[nodiscard]] bool refreshCreativePlayerSpawnPreviewCache(
    CreativePlayerSpawnPreviewCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeRoomBakeResult* roomBake,
    const iggy3d::creative::CreativeObject* selectedObject,
    std::uint64_t roomBakeRevision);
void invalidateCreativePlayerSpawnPreviewCache(
    CreativePlayerSpawnPreviewCache& cache) noexcept;
[[nodiscard]] CreativePlayerSpawnPlanSymbol planCreativePlayerSpawnPlanSymbol(
    iggy3d::creative::CreativeVec3 pointCells,
    double yawRadians,
    const iggy3d::creative::CreativePlayerSpawnSettings& settings,
    double cellSizeMeters) noexcept;
[[nodiscard]] std::string_view creativePlayerSpawnPreviewStatusLabel(
    iggy3d::creative::CreativePlayerSpawnStatus status) noexcept;

void appendCreativeEditorPlayerSpawnPreview(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);

}  // namespace iggy3d_creative_app
