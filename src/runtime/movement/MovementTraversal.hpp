#pragma once

#include <cstdint>
#include <string>

#include "content/assets/RoomAsset.hpp"
#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/movement/MovementKinematics.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {

enum class TraversalMechanic : std::uint8_t {
  Vault,
  Clamber,
  WireWalk,
};

enum class TraversalStatus : std::uint8_t {
  Applied,
  InvalidActor,
  ActorInactive,
  InvalidInput,
  MissingRoom,
  MissingCollisionSurfaces,
  UnsupportedMechanic,
  TargetNotFound,
  OutOfRange,
  NotFacingTarget,
  HeightRejected,
  WidthRejected,
  NoLandingGround,
  LandingBlocked,
  WorldMutationFailed,
};

enum class TraversalIntentTrigger : std::uint8_t {
  None,
  Jump,
  Interact,
};

enum class TraversalIntentStatus : std::uint8_t {
  NoIntent,
  Applied,
  NoTraversalCandidate,
  TraversalRejected,
  InvalidActor,
  ActorInactive,
  InvalidInput,
  MissingRoom,
  MissingCollisionSurfaces,
};

enum class TraversalCandidatePreviewStatus : std::uint8_t {
  Ready,
  NoCandidate,
  OutOfRange,
  BadAngle,
  HeightRejected,
  WidthRejected,
  NoLandingGround,
  LandingBlocked,
  InvalidActor,
  ActorInactive,
  InvalidInput,
  MissingRoom,
  MissingCollisionSurfaces,
  UnsupportedMechanic,
};

struct TraversalRequest {
  EntityId actor;
  TraversalMechanic mechanic = TraversalMechanic::Vault;
  Vec3 forward = {0.0F, 0.0F, -1.0F};
  const RoomAsset* room = nullptr;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  Vec3 roomWorldOffsetMeters;
  float maxStartRangeMeters = 1.25F;
  float landingClearanceMeters = 0.90F;
  float landingGroundSnapMeters = 1.00F;
  float minClamberLedgeHeightMeters = 0.45F;
  float maxClamberLedgeHeightMeters = 1.80F;
  float minClamberUsableWidthMeters = 0.45F;
};

struct TraversalResult {
  TraversalStatus status = TraversalStatus::InvalidInput;
  TraversalMechanic mechanic = TraversalMechanic::Vault;
  EntityId actor;
  Vec3 start;
  Vec3 finalPosition;
  MovementTravelFacts travel;
  std::string slotId;
  std::string slotKind;
  std::string slotHeightBand;
  std::string targetId;
  std::string landingSurfaceId;
  Vec3 railStartPosition;
  Vec3 railEndPosition;
  Vec3 railAxis = {1.0F, 0.0F, 0.0F};
  float slotLedgeHeightMeters = 0.0F;
  float slotUsableWidthMeters = 0.0F;
  float slotStartRangeMeters = 0.0F;
  float slotFacingDot = 0.0F;
  float railCoordinateMeters = 0.0F;
  float railLengthMeters = 0.0F;
  const char* reasonCode = "traversal_invalid_input";
};

struct TraversalIntentRequest {
  EntityId actor;
  bool jumpPressed = false;
  bool interactPressed = false;
  Vec3 forward = {0.0F, 0.0F, -1.0F};
  const RoomAsset* room = nullptr;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  Vec3 roomWorldOffsetMeters;
  float maxStartRangeMeters = 1.25F;
  float landingClearanceMeters = 0.90F;
  float landingGroundSnapMeters = 1.00F;
  float minClamberLedgeHeightMeters = 0.45F;
  float maxClamberLedgeHeightMeters = 1.80F;
  float minClamberUsableWidthMeters = 0.45F;
};

struct TraversalIntentResult {
  TraversalIntentStatus status = TraversalIntentStatus::NoIntent;
  TraversalIntentTrigger trigger = TraversalIntentTrigger::None;
  TraversalMechanic selectedMechanic = TraversalMechanic::Clamber;
  TraversalResult traversal;
  bool requested = false;
  bool traversalAttempted = false;
  bool consumedInput = false;
  bool accepted = false;
  bool fallbackJumpAllowed = false;
  const char* reasonCode = "traversal_intent_no_intent";
};

struct TraversalCandidatePreviewRequest {
  EntityId actor;
  Vec3 forward = {0.0F, 0.0F, -1.0F};
  const RoomAsset* room = nullptr;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  Vec3 roomWorldOffsetMeters;
  float maxStartRangeMeters = 1.25F;
  float landingClearanceMeters = 0.90F;
  float landingGroundSnapMeters = 1.00F;
  float minClamberLedgeHeightMeters = 0.45F;
  float maxClamberLedgeHeightMeters = 1.80F;
  float minClamberUsableWidthMeters = 0.45F;
  bool includeClamber = true;
  bool includeVault = true;
  bool includeWireWalk = true;
};

struct TraversalCandidatePreviewResult {
  TraversalCandidatePreviewStatus status = TraversalCandidatePreviewStatus::InvalidInput;
  TraversalMechanic selectedMechanic = TraversalMechanic::Clamber;
  EntityId actor;
  Vec3 start;
  Vec3 landingPosition;
  bool candidateAvailable = false;
  bool ready = false;
  std::string slotId = "none";
  std::string slotKind = "none";
  std::string slotHeightBand = "none";
  std::string targetId = "none";
  std::string landingSurfaceId = "none";
  float slotLedgeHeightMeters = 0.0F;
  float slotUsableWidthMeters = 0.0F;
  float slotStartRangeMeters = 0.0F;
  float slotFacingDot = 0.0F;
  const char* reasonCode = "traversal_preview_invalid_input";
  const char* hudCode = "INVALID";
};

// MA4 s2: SLOT-BASED cores -- preview/execute a traversal for ONE already-resolved slot, WITHOUT a
// RoomAsset (the slot carries all geometry). This is the seam that lets Move execution fire a
// traversal: the session arms ONE bridging slot onto the MovementRequest, and executeMovement uses
// these. Gate with the preview (the same 1.25 m / 0.35 facing gates) BEFORE the mutating execute.
// (`collisionSurfaces` is required for the landing/clearance checks -- the shipped slot carries
// geometry but the ground surface still lives in the surface set.)
TraversalCandidatePreviewResult previewTraversalCandidateForSlot(
    const WorldState& world, EntityId actor, const MovementTraversalSlot& slot,
    const SpatialSurfaceSet* collisionSurfaces, Vec3 forward);
TraversalResult executeTraversalMechanicForSlot(WorldState& world, EntityId actor,
                                                const MovementTraversalSlot& slot,
                                                const SpatialSurfaceSet* collisionSurfaces,
                                                Vec3 forward);

TraversalResult executeTraversalMechanic(WorldState& world, const TraversalRequest& request);
TraversalIntentResult executeTraversalIntent(WorldState& world,
                                             const TraversalIntentRequest& request);
TraversalCandidatePreviewResult previewTraversalCandidate(
    const WorldState& world,
    const TraversalCandidatePreviewRequest& request);
const char* traversalMechanicName(TraversalMechanic mechanic);
const char* traversalStatusName(TraversalStatus status);
const char* traversalIntentTriggerName(TraversalIntentTrigger trigger);
const char* traversalIntentStatusName(TraversalIntentStatus status);
const char* traversalCandidatePreviewStatusName(TraversalCandidatePreviewStatus status);
const char* traversalCandidatePreviewHudCode(TraversalCandidatePreviewStatus status);
bool traversalApplied(const TraversalResult& result);

}  // namespace iggy3d
