#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "content/assets/RoomAsset.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class MovementTraversalSlotKind : std::uint8_t {
  Vault,
  Clamber,
  WireWalk,
};

enum class MovementTraversalSlotSelectionStatus : std::uint8_t {
  Found,
  TargetNotFound,
  OutOfRange,
  NotFacingSlot,
  HeightRejected,
  WidthRejected,
  InvalidInput,
};

inline constexpr std::size_t kInvalidTraversalSlotIndex =
    std::numeric_limits<std::size_t>::max();

struct MovementTraversalSlot {
  std::string slotId;
  MovementTraversalSlotKind kind = MovementTraversalSlotKind::Clamber;
  std::string sourceStaticMeshId;
  std::string frontSurfaceId;
  std::string topSurfaceId;
  std::string landingSurfaceId;
  std::string heightBand = "unknown";
  Aabb3 targetBounds;
  Aabb3 frontFaceBounds;
  Aabb3 landingBounds;
  Vec3 frontFaceNormal = {0.0F, 0.0F, 1.0F};
  Vec3 landingPosition;
  float topHeightMeters = 0.0F;
  float ledgeHeightMeters = 0.0F;
  float usableWidthMeters = 0.0F;
  float approachMinDistanceMeters = 0.0F;
  float approachMaxDistanceMeters = 1.25F;
  float facingDotMin = 0.35F;
  float requiredClearanceHeightMeters = 1.80F;
};

struct MovementTraversalSlotRegistry {
  std::vector<MovementTraversalSlot> slots;
};

struct MovementTraversalSlotSelectionRequest {
  MovementTraversalSlotKind kind = MovementTraversalSlotKind::Clamber;
  Vec3 actorPosition;
  Vec3 forward = {0.0F, 0.0F, -1.0F};
  float maxStartRangeMeters = 1.25F;
  float minLedgeHeightMeters = 0.0F;
  float maxLedgeHeightMeters = 2.0F;
  float minUsableWidthMeters = 0.0F;
  float facingDotMin = 0.35F;
};

struct MovementTraversalSlotSelection {
  MovementTraversalSlotSelectionStatus status =
      MovementTraversalSlotSelectionStatus::InvalidInput;
  std::size_t slotIndex = kInvalidTraversalSlotIndex;
  std::size_t candidateSlotIndex = kInvalidTraversalSlotIndex;
  float startRangeMeters = 0.0F;
  float facingDot = 0.0F;
  float ledgeHeightFromFeetMeters = 0.0F;
  float candidateStartRangeMeters = 0.0F;
  float candidateFacingDot = 0.0F;
  float candidateLedgeHeightFromFeetMeters = 0.0F;
  const char* reasonCode = "slot_invalid_input";
};

MovementTraversalSlotRegistry buildMovementTraversalSlotRegistry(const RoomAsset& room,
                                                                 Vec3 roomWorldOffsetMeters);
MovementTraversalSlotSelection selectMovementTraversalSlot(
    const MovementTraversalSlotRegistry& registry,
    const MovementTraversalSlotSelectionRequest& request);
const MovementTraversalSlot* selectedTraversalSlot(
    const MovementTraversalSlotRegistry& registry,
    const MovementTraversalSlotSelection& selection);
const MovementTraversalSlot* candidateTraversalSlot(
    const MovementTraversalSlotRegistry& registry,
    const MovementTraversalSlotSelection& selection);
const char* movementTraversalSlotKindName(MovementTraversalSlotKind kind);
const char* movementTraversalSlotSelectionStatusName(
    MovementTraversalSlotSelectionStatus status);

}  // namespace iggy3d
