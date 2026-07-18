#include "runtime/movement/ClamberMotor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

#include "core/math/Aabb3.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"

namespace iggy3d {
namespace {

constexpr float kClamberEpsilon = 0.0001F;
// The motor parks the capsule a skin-width from the face; allow that much
// residual penetration when measuring the face gap.
constexpr float kFaceGapSlackMeters = 0.05F;
// Landing inset past the ledge face, capped at half the ledge depth so
// narrow pieces (kit vault_rail, 0.15 m deep) land on their centerline.
constexpr float kLandingInsetExtraMeters = 0.05F;
// Clearance volume floats this far above the ledge top so a capsule resting
// exactly on the ledge does not self-collide with it.
constexpr float kClearanceLiftMeters = 0.03F;

ClamberEngageResult refuse(ClamberRefusalReason reason) {
  ClamberEngageResult result;
  result.engaged = false;
  result.refusal = reason;
  return result;
}

bool surfaceTagDenied(const ClamberEngageRequest& request,
                      std::size_t colliderIndex) {
  if (request.surfaces == nullptr || request.bake == nullptr ||
      colliderIndex >= request.bake->sourceSurfaceIndices.size()) {
    return false;
  }
  const std::size_t surfaceIndex =
      request.bake->sourceSurfaceIndices[colliderIndex];
  if (surfaceIndex >= request.surfaces->size()) {
    return false;
  }
  const CollisionSurfaceView& view = request.surfaces->surfaces()[surfaceIndex];
  for (const std::string& tag : view.traversalTags) {
    // Deny override. `clamber`/`clamber_candidate` are the allow side and
    // v1 geometry already allows in-band surfaces, so only the deny tag has
    // an effect here; the allow tags stay honored implicitly.
    if (tag == "no_player" || tag == "debug_only") {
      return true;
    }
  }
  return false;
}

}  // namespace

std::string_view clamberRefusalReasonCode(ClamberRefusalReason reason) {
  switch (reason) {
    case ClamberRefusalReason::None: return "clamber_engaged";
    case ClamberRefusalReason::NotGrounded: return "clamber_not_grounded";
    case ClamberRefusalReason::InvalidDirection:
      return "clamber_invalid_direction";
    case ClamberRefusalReason::NoBlockingSurface:
      return "clamber_no_blocking_surface";
    case ClamberRefusalReason::TopBelowBand:
      return "clamber_top_below_band";
    case ClamberRefusalReason::TopAboveBand:
      return "clamber_top_above_band";
    case ClamberRefusalReason::ReachExceeded:
      return "clamber_reach_exceeded";
    case ClamberRefusalReason::NoLandingClearance:
      return "clamber_no_landing_clearance";
    case ClamberRefusalReason::SurfaceDenied:
      return "clamber_surface_denied";
  }
  return "clamber_invalid_reason";
}

ClamberEngageResult evaluateClamberEngage(const ClamberEngageRequest& request) {
  if (!request.grounded) {
    return refuse(ClamberRefusalReason::NotGrounded);
  }
  if (request.bake == nullptr || !request.bake->ok ||
      request.blockingSurfaceId.empty()) {
    return refuse(ClamberRefusalReason::NoBlockingSurface);
  }
  Vec3 direction = request.desiredDirection;
  direction.y = 0.0F;
  if (lengthSquared(direction) <= kClamberEpsilon * kClamberEpsilon) {
    return refuse(ClamberRefusalReason::InvalidDirection);
  }
  direction = normalized(direction);

  const MovementParams& params = request.params;
  const Vec3 foot = request.startFootMeters;
  const float bodyHalfHeight = params.heightMeters * 0.5F;
  const Vec3 bodyCenter = foot + Vec3{0.0F, bodyHalfHeight, 0.0F};
  const float bodyMinY = foot.y;
  const float bodyMaxY = foot.y + params.heightMeters;

  // Find the blocking collider: same source surface as the motor's hit,
  // vertically overlapping the capsule, nearest along the push direction.
  std::size_t chosenIndex = request.bake->colliders.size();
  float chosenFaceGap = std::numeric_limits<float>::max();
  for (std::size_t i = 0; i < request.bake->colliders.size(); ++i) {
    if (i >= request.bake->sourceSurfaceIds.size() ||
        request.bake->sourceSurfaceIds[i] != request.blockingSurfaceId) {
      continue;
    }
    const PhysicsAabbCollider& collider = request.bake->colliders[i];
    if (collider.sensor) {
      continue;
    }
    if (collider.bounds.min.y >= bodyMaxY - kClamberEpsilon ||
        collider.bounds.max.y <= bodyMinY + kClamberEpsilon) {
      continue;
    }
    // Horizontal slab gap from the capsule's leading faces to the collider
    // along the push direction (per-axis entry distance; the larger axis
    // constrains).
    float faceGap = -std::numeric_limits<float>::max();
    bool approachable = true;
    const float centerAxis[2] = {bodyCenter.x, bodyCenter.z};
    const float dirAxis[2] = {direction.x, direction.z};
    const float minAxis[2] = {collider.bounds.min.x, collider.bounds.min.z};
    const float maxAxis[2] = {collider.bounds.max.x, collider.bounds.max.z};
    for (int axis = 0; axis < 2; ++axis) {
      const float bodyMin = centerAxis[axis] - params.radiusMeters;
      const float bodyMax = centerAxis[axis] + params.radiusMeters;
      if (std::fabs(dirAxis[axis]) <= kClamberEpsilon) {
        if (bodyMax <= minAxis[axis] || bodyMin >= maxAxis[axis]) {
          approachable = false;  // separated on an axis we are not closing
          break;
        }
        continue;
      }
      const float gap = dirAxis[axis] > 0.0F
                            ? (minAxis[axis] - bodyMax) / dirAxis[axis]
                            : (maxAxis[axis] - bodyMin) / dirAxis[axis];
      faceGap = std::max(faceGap, gap);
    }
    if (!approachable || faceGap < -kFaceGapSlackMeters) {
      continue;
    }
    faceGap = std::max(faceGap, 0.0F);
    if (faceGap < chosenFaceGap) {
      chosenFaceGap = faceGap;
      chosenIndex = i;
    }
  }
  if (chosenIndex >= request.bake->colliders.size()) {
    return refuse(ClamberRefusalReason::NoBlockingSurface);
  }
  const PhysicsAabbCollider& ledge = request.bake->colliders[chosenIndex];

  // Band gate: exact and tag-immune. Bottom exclusive (autoStep territory),
  // top inclusive.
  const float ledgeHeight = ledge.bounds.max.y - foot.y;
  if (ledgeHeight <= params.clamberBandBottomMeters + kClamberEpsilon) {
    return refuse(ClamberRefusalReason::TopBelowBand);
  }
  if (ledgeHeight > params.clamberBandTopMeters + kClamberEpsilon) {
    return refuse(ClamberRefusalReason::TopAboveBand);
  }
  if (chosenFaceGap > params.clamberMaxReachMeters) {
    return refuse(ClamberRefusalReason::ReachExceeded);
  }
  if (surfaceTagDenied(request, chosenIndex)) {
    return refuse(ClamberRefusalReason::SurfaceDenied);
  }

  // Landing target: past the ledge face by radius plus an inset, capped at
  // half the ledge depth along the dominant approach axis so narrow ledges
  // land on their centerline.
  const bool xDominant = std::fabs(direction.x) >= std::fabs(direction.z);
  const float ledgeDepth = xDominant
                               ? (ledge.bounds.max.x - ledge.bounds.min.x)
                               : (ledge.bounds.max.z - ledge.bounds.min.z);
  const float inset = std::min(params.radiusMeters + kLandingInsetExtraMeters,
                               ledgeDepth * 0.5F);
  const float travel = chosenFaceGap + params.radiusMeters + inset;
  Vec3 targetFoot = foot + direction * travel;
  targetFoot.y = ledge.bounds.max.y;

  // Landing clearance: a capsule-sized box floated just above the ledge top
  // must overlap nothing. Shrunk by skin so resting contact does not count.
  const Vec3 clearanceHalf{params.radiusMeters - params.skinMeters,
                           bodyHalfHeight,
                           params.radiusMeters - params.skinMeters};
  const Vec3 clearanceCenter{
      targetFoot.x, targetFoot.y + kClearanceLiftMeters + bodyHalfHeight,
      targetFoot.z};
  const Aabb3 clearanceBounds =
      aabbFromCenterExtents(clearanceCenter, clearanceHalf);
  for (const PhysicsAabbCollider& collider : request.bake->colliders) {
    if (collider.sensor) {
      continue;
    }
    if (intersects(clearanceBounds, collider.bounds)) {
      return refuse(ClamberRefusalReason::NoLandingClearance);
    }
  }

  ClamberEngageResult result;
  result.engaged = true;
  result.refusal = ClamberRefusalReason::None;
  result.targetFootMeters = targetFoot;
  result.surfaceId = std::string(request.blockingSurfaceId);
  result.ledgeHeightMeters = ledgeHeight;
  return result;
}

Vec3 clamberPositionAtTick(const ClamberPhaseState& phase,
                           std::uint32_t ticksElapsed) {
  if (phase.ticksTotal == 0U || ticksElapsed >= phase.ticksTotal) {
    return phase.targetFootMeters;
  }
  const float fraction = static_cast<float>(ticksElapsed) /
                         static_cast<float>(phase.ticksTotal);
  const float rise = phase.targetFootMeters.y - phase.startFootMeters.y;
  Vec3 horizontal = phase.targetFootMeters - phase.startFootMeters;
  horizontal.y = 0.0F;
  const float mantle = length(horizontal);
  const float pathLength = std::max(rise, 0.0F) + mantle;
  if (pathLength <= kClamberEpsilon) {
    return phase.targetFootMeters;
  }
  const float traveled = fraction * pathLength;
  if (traveled <= rise) {
    return phase.startFootMeters + Vec3{0.0F, traveled, 0.0F};
  }
  const float mantleFraction =
      mantle <= kClamberEpsilon ? 1.0F : (traveled - rise) / mantle;
  Vec3 position = phase.startFootMeters + horizontal * mantleFraction;
  position.y = phase.targetFootMeters.y;
  return position;
}

}  // namespace iggy3d
