#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/movement/MovementParams.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

namespace iggy3d {

class SpatialSurfaceSet;

// Clamber motor (flow feat v1). Engagement is GEOMETRIC over the baked AABB
// colliders -- never hand-annotation. Traversal tags are honored only as an
// allow/deny override where the bake already carries a back-reference to the
// tagged source surface: `no_player` denies; `clamber`/`clamber_candidate`
// mark explicit intent (geometrically in-band surfaces are allowed either
// way in v1). Band and clearance refusals are exact and are NEVER overridden
// by tags.

enum class ClamberRefusalReason : std::uint8_t {
  None,
  NotGrounded,
  InvalidDirection,
  NoBlockingSurface,
  TopBelowBand,
  TopAboveBand,
  ReachExceeded,
  NoLandingClearance,
  SurfaceDenied,
};

std::string_view clamberRefusalReasonCode(ClamberRefusalReason reason);

struct ClamberEngageRequest {
  const PhysicsSpatialSurfaceColliderBakeResult* bake = nullptr;
  // Optional: source surfaces for the tag override. May be null; the bake's
  // sourceSurfaceIndices back-reference is the only tag path used.
  const SpatialSurfaceSet* surfaces = nullptr;
  Vec3 startFootMeters;
  // Horizontal movement intent; need not be normalized.
  Vec3 desiredDirection;
  bool grounded = false;
  // Surface id the motor reported as blocking (firstHit/stepObstacle).
  std::string_view blockingSurfaceId;
  MovementParams params;
};

struct ClamberEngageResult {
  bool engaged = false;
  ClamberRefusalReason refusal = ClamberRefusalReason::None;
  Vec3 targetFootMeters;
  std::string surfaceId;
  float ledgeHeightMeters = 0.0F;
};

ClamberEngageResult evaluateClamberEngage(const ClamberEngageRequest& request);

// Phase state. TRANSIENT-IN-PERSISTENCE by law (route-state precedent):
// lives on SessionState outside `transient`, never hashed, never serialized.
struct ClamberPhaseState {
  bool active = false;
  EntityId actor{};
  std::uint32_t ticksElapsed = 0U;
  std::uint32_t ticksTotal = 0U;
  Vec3 startFootMeters;
  Vec3 targetFootMeters;
};

// Deterministic piecewise-linear path: vertical rise first, then the
// horizontal mantle onto the ledge. Pure function of the stored endpoints
// and the tick counter -- no wall clock, no randomness.
Vec3 clamberPositionAtTick(const ClamberPhaseState& phase,
                           std::uint32_t ticksElapsed);

}  // namespace iggy3d
