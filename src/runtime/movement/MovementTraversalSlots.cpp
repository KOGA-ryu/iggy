#include "runtime/movement/MovementTraversalSlots.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "content/assets/TraversalTag.hpp"

namespace iggy3d {
namespace {

inline constexpr float kEpsilon = 0.0001F;

bool hasText(std::string_view value, std::string_view expected) {
  return value.find(expected) != std::string_view::npos;
}

bool containsString(const std::vector<std::string>& values, std::string_view expected) {
  for (const std::string& value : values) {
    if (value == expected) {
      return true;
    }
  }
  return false;
}

bool kindForTraversalTag(std::string_view tag, MovementTraversalSlotKind& out) {
  const std::optional<TraversalTag> parsed = parseTraversalTag(tag);
  if (!parsed.has_value()) {
    return false;
  }
  switch (*parsed) {
    case TraversalTag::Vault:
      out = MovementTraversalSlotKind::Vault;
      return true;
    case TraversalTag::Clamber:
      out = MovementTraversalSlotKind::Clamber;
      return true;
    case TraversalTag::WireWalk:
      out = MovementTraversalSlotKind::WireWalk;
      return true;
    case TraversalTag::Walkable:
    case TraversalTag::Blocker:
    case TraversalTag::ProjectileBlocker:
    case TraversalTag::Opening:
    case TraversalTag::ClamberCandidate:
    case TraversalTag::NoPlayer:
    case TraversalTag::DebugOnly:
      return false;
  }
  return false;
}

bool affordanceExists(const MovementTraversalAffordanceRegistry& registry,
                      std::string_view meshId,
                      MovementTraversalSlotKind kind) {
  for (const MovementTraversalAffordance& affordance : registry.affordances) {
    if (affordance.sourceStaticMeshId == meshId && affordance.kind == kind) {
      return true;
    }
  }
  return false;
}

bool meshHasAuthoredTraversalAffordance(
    const MovementTraversalAffordanceRegistry& registry,
    std::string_view meshId) {
  for (const MovementTraversalAffordance& affordance : registry.affordances) {
    if (affordance.sourceStaticMeshId == meshId && affordance.authoredTag) {
      return true;
    }
  }
  return false;
}

void appendAffordance(MovementTraversalAffordanceRegistry& registry,
                      MovementTraversalSlotKind kind,
                      std::string_view meshId,
                      std::string_view sourceSurfaceId,
                      bool authoredTag) {
  if (meshId.empty() || affordanceExists(registry, meshId, kind)) {
    return;
  }
  MovementTraversalAffordance affordance;
  affordance.kind = kind;
  affordance.sourceStaticMeshId = std::string(meshId);
  affordance.sourceSurfaceId =
      sourceSurfaceId.empty() ? "legacy_name_fallback" : std::string(sourceSurfaceId);
  affordance.authoredTag = authoredTag;
  registry.affordances.push_back(std::move(affordance));
}

const RoomStaticMeshAsset* findMeshById(const RoomAsset& room, std::string_view meshId) {
  for (const RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (mesh.id == meshId) {
      return &mesh;
    }
  }
  return nullptr;
}

bool meshSupportsTraversalAffordance(const RoomStaticMeshAsset& mesh,
                                     MovementTraversalSlotKind kind) {
  switch (kind) {
    case MovementTraversalSlotKind::Vault:
    case MovementTraversalSlotKind::WireWalk:
      return mesh.role == "rail";
    case MovementTraversalSlotKind::Clamber:
      return mesh.role == "ledge" || mesh.role == "wall";
  }
  return false;
}

bool normalized(Vec3 value, Vec3& out) {
  if (!isFinite(value)) {
    return false;
  }
  const float lengthSquaredValue = lengthSquared(value);
  if (!std::isfinite(lengthSquaredValue) || lengthSquaredValue <= kEpsilon * kEpsilon) {
    return false;
  }
  out = value / std::sqrt(lengthSquaredValue);
  return isFinite(out);
}

bool normalizeHorizontal(Vec3 value, Vec3& out) {
  value.y = 0.0F;
  return normalized(value, out);
}

bool buildBounds(std::span<const Vec3> points, Vec3 offset, Aabb3& out) {
  if (points.empty()) {
    return false;
  }
  Vec3 minPoint{std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  Vec3 maxPoint{-std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max()};
  for (const Vec3 rawPoint : points) {
    const Vec3 point = rawPoint + offset;
    if (!isFinite(point)) {
      return false;
    }
    minPoint.x = std::min(minPoint.x, point.x);
    minPoint.y = std::min(minPoint.y, point.y);
    minPoint.z = std::min(minPoint.z, point.z);
    maxPoint.x = std::max(maxPoint.x, point.x);
    maxPoint.y = std::max(maxPoint.y, point.y);
    maxPoint.z = std::max(maxPoint.z, point.z);
  }
  out = makeAabb3(minPoint, maxPoint);
  return isValid(out);
}

Aabb3 meshBounds(const RoomStaticMeshAsset& mesh, Vec3 offset) {
  return aabbFromCenterExtents(mesh.positionMeters + offset, mesh.sizeMeters * 0.5F);
}

float horizontalDistanceToBounds(Vec3 point, const Aabb3& bounds) {
  const Vec3 closest = closestPoint(bounds, {point.x, center(bounds).y, point.z});
  const float x = closest.x - point.x;
  const float z = closest.z - point.z;
  return std::sqrt(x * x + z * z);
}

float horizontalUsableWidth(const Aabb3& bounds) {
  const Vec3 size = bounds.max - bounds.min;
  return std::max(size.x, size.z);
}

std::string heightBandForLedge(float ledgeHeightMeters) {
  if (!std::isfinite(ledgeHeightMeters)) {
    return "unknown";
  }
  if (ledgeHeightMeters < 0.10F) {
    return "floor";
  }
  if (ledgeHeightMeters <= 0.45F) {
    return "step_up";
  }
  if (ledgeHeightMeters <= 0.85F) {
    return "clamber_low";
  }
  if (ledgeHeightMeters <= 1.30F) {
    return "clamber_mid";
  }
  if (ledgeHeightMeters <= 1.80F) {
    return "clamber_high";
  }
  return "blocked_high";
}

const RoomSpatialSurface* findSurfaceForMesh(const RoomAsset& room,
                                             std::string_view meshId,
                                             RoomSpatialSurfaceRole role,
                                             std::string_view preferredTag = {}) {
  const RoomSpatialSurface* fallback = nullptr;
  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    if (surface.sourceStaticMeshId == meshId && surface.role == role) {
      if (!preferredTag.empty() && containsString(surface.traversalTags, preferredTag)) {
        return &surface;
      }
      if (fallback == nullptr) {
        fallback = &surface;
      }
    }
  }
  return fallback;
}

const RoomSpatialSurface* findActorBlockerForMesh(const RoomAsset& room,
                                                  std::string_view meshId,
                                                  std::string_view preferredTag = {}) {
  const RoomSpatialSurface* fallback = nullptr;
  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    if (surface.sourceStaticMeshId == meshId && surface.blocksActor) {
      if (!preferredTag.empty() && containsString(surface.traversalTags, preferredTag)) {
        return &surface;
      }
      if (fallback == nullptr) {
        fallback = &surface;
      }
    }
  }
  return fallback;
}

void appendVaultSlot(MovementTraversalSlotRegistry& registry,
                     const RoomStaticMeshAsset& mesh,
                     const MovementTraversalAffordance& affordance,
                     Vec3 offset) {
  const Aabb3 bounds = meshBounds(mesh, offset);
  if (!isValid(bounds)) {
    return;
  }
  MovementTraversalSlot slot;
  slot.slotId = mesh.id;
  slot.kind = MovementTraversalSlotKind::Vault;
  slot.sourceStaticMeshId = mesh.id;
  slot.affordanceSourceId = affordance.sourceSurfaceId;
  slot.targetBounds = bounds;
  slot.frontFaceBounds = bounds;
  slot.landingBounds = bounds;
  slot.landingPosition = center(bounds);
  slot.topHeightMeters = bounds.max.y;
  slot.ledgeHeightMeters = bounds.max.y - bounds.min.y;
  slot.usableWidthMeters = horizontalUsableWidth(bounds);
  slot.heightBand = "vault_low";
  slot.approachMaxDistanceMeters = 1.25F;
  slot.requiredClearanceHeightMeters = 1.20F;
  slot.authoredAffordance = affordance.authoredTag;
  registry.slots.push_back(std::move(slot));
}

void appendWireWalkSlot(MovementTraversalSlotRegistry& registry,
                        const RoomStaticMeshAsset& mesh,
                        const MovementTraversalAffordance& affordance,
                        Vec3 offset) {
  const Aabb3 bounds = meshBounds(mesh, offset);
  if (!isValid(bounds)) {
    return;
  }

  MovementTraversalSlot slot;
  slot.slotId = mesh.id;
  slot.kind = MovementTraversalSlotKind::WireWalk;
  slot.sourceStaticMeshId = mesh.id;
  slot.affordanceSourceId = affordance.sourceSurfaceId;
  slot.topSurfaceId = mesh.id;
  slot.landingSurfaceId = mesh.id;
  slot.targetBounds = bounds;
  slot.frontFaceBounds = bounds;
  slot.landingBounds = bounds;
  slot.landingPosition = center(bounds);
  slot.landingPosition.y = bounds.max.y;
  slot.topHeightMeters = bounds.max.y;
  slot.ledgeHeightMeters = bounds.max.y - bounds.min.y;
  slot.usableWidthMeters = horizontalUsableWidth(bounds);
  slot.heightBand = "wire_balance";
  slot.approachMaxDistanceMeters = 1.25F;
  slot.requiredClearanceHeightMeters = 1.80F;
  slot.authoredAffordance = affordance.authoredTag;
  registry.slots.push_back(std::move(slot));
}

void appendClamberSlot(MovementTraversalSlotRegistry& registry,
                       const RoomAsset& room,
                       const RoomStaticMeshAsset& mesh,
                       const MovementTraversalAffordance& affordance,
                       Vec3 offset) {
  const std::string_view clamberTag = traversalTagId(TraversalTag::Clamber);
  // The durable clamber tag authorizes the mesh affordance. Top and blocker
  // surfaces are measured helper geometry: prefer tagged surfaces, but same-mesh
  // untagged helpers are valid fallbacks for existing authored rooms.
  const RoomSpatialSurface* top =
      findSurfaceForMesh(room, mesh.id, RoomSpatialSurfaceRole::Walkable, clamberTag);
  if (top == nullptr) {
    return;
  }

  Aabb3 topBounds;
  if (!buildBounds(top->pointsMeters, offset, topBounds)) {
    return;
  }
  const Aabb3 targetBounds = meshBounds(mesh, offset);
  if (!isValid(targetBounds)) {
    return;
  }

  const RoomSpatialSurface* blocker = findActorBlockerForMesh(room, mesh.id, clamberTag);
  if (blocker == nullptr) {
    return;
  }
  Aabb3 frontBounds;
  if (!buildBounds(blocker->pointsMeters, offset, frontBounds)) {
    return;
  }
  Vec3 frontNormal = {0.0F, 0.0F, 1.0F};
  Vec3 normalizedNormal;
  if (normalized(blocker->normal, normalizedNormal)) {
    frontNormal = normalizedNormal;
  }

  MovementTraversalSlot slot;
  slot.slotId = mesh.id + ":" + top->id;
  slot.kind = MovementTraversalSlotKind::Clamber;
  slot.sourceStaticMeshId = mesh.id;
  slot.affordanceSourceId = affordance.sourceSurfaceId;
  slot.frontSurfaceId = blocker->id;
  slot.topSurfaceId = top->id;
  slot.landingSurfaceId = top->id;
  slot.targetBounds = targetBounds;
  slot.frontFaceBounds = frontBounds;
  slot.landingBounds = topBounds;
  slot.frontFaceNormal = frontNormal;
  slot.landingPosition = center(topBounds);
  slot.topHeightMeters = topBounds.max.y;
  slot.ledgeHeightMeters = topBounds.max.y - targetBounds.min.y;
  slot.usableWidthMeters = horizontalUsableWidth(topBounds);
  slot.heightBand = heightBandForLedge(slot.ledgeHeightMeters);
  slot.approachMaxDistanceMeters = 1.25F;
  slot.requiredClearanceHeightMeters = 1.80F;
  slot.authoredAffordance = affordance.authoredTag;
  registry.slots.push_back(std::move(slot));
}

bool kindMatches(MovementTraversalSlotKind candidate, MovementTraversalSlotKind expected) {
  return candidate == expected;
}

}  // namespace

MovementTraversalAffordanceRegistry buildMovementTraversalAffordanceRegistry(
    const RoomAsset& room) {
  MovementTraversalAffordanceRegistry registry;
  registry.affordances.reserve(room.spatialSurfaces.size());

  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    for (const std::string& tag : surface.traversalTags) {
      MovementTraversalSlotKind kind = MovementTraversalSlotKind::Clamber;
      const RoomStaticMeshAsset* sourceMesh = findMeshById(room, surface.sourceStaticMeshId);
      if (kindForTraversalTag(tag, kind) && sourceMesh != nullptr &&
          meshSupportsTraversalAffordance(*sourceMesh, kind)) {
        appendAffordance(registry, kind, surface.sourceStaticMeshId, surface.id, true);
      }
    }
  }

  for (const RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (meshHasAuthoredTraversalAffordance(registry, mesh.id)) {
      continue;
    }
    if (mesh.role == "rail" && hasText(mesh.id, "vault")) {
      appendAffordance(registry,
                       MovementTraversalSlotKind::Vault,
                       mesh.id,
                       "legacy_name_fallback",
                       false);
    }
    if (mesh.role == "rail" && hasText(mesh.id, "wire")) {
      appendAffordance(registry,
                       MovementTraversalSlotKind::WireWalk,
                       mesh.id,
                       "legacy_name_fallback",
                       false);
    }
  }

  return registry;
}

MovementTraversalSlotRegistry buildMovementTraversalSlotRegistry(const RoomAsset& room,
                                                                 Vec3 roomWorldOffsetMeters) {
  const MovementTraversalAffordanceRegistry affordanceRegistry =
      buildMovementTraversalAffordanceRegistry(room);

  MovementTraversalSlotRegistry registry;
  registry.affordances = affordanceRegistry.affordances;
  registry.slots.reserve(registry.affordances.size());
  for (const MovementTraversalAffordance& affordance : registry.affordances) {
    const RoomStaticMeshAsset* mesh = findMeshById(room, affordance.sourceStaticMeshId);
    if (mesh == nullptr || !meshSupportsTraversalAffordance(*mesh, affordance.kind)) {
      continue;
    }
    switch (affordance.kind) {
      case MovementTraversalSlotKind::Vault:
        appendVaultSlot(registry, *mesh, affordance, roomWorldOffsetMeters);
        break;
      case MovementTraversalSlotKind::Clamber:
        appendClamberSlot(registry, room, *mesh, affordance, roomWorldOffsetMeters);
        break;
      case MovementTraversalSlotKind::WireWalk:
        appendWireWalkSlot(registry, *mesh, affordance, roomWorldOffsetMeters);
        break;
    }
  }
  return registry;
}

MovementTraversalSlotSelection selectMovementTraversalSlot(
    const MovementTraversalSlotRegistry& registry,
    const MovementTraversalSlotSelectionRequest& request) {
  MovementTraversalSlotSelection selection;
  if (!isFinite(request.actorPosition) || !std::isfinite(request.maxStartRangeMeters) ||
      request.maxStartRangeMeters <= 0.0F || !std::isfinite(request.minLedgeHeightMeters) ||
      !std::isfinite(request.maxLedgeHeightMeters) ||
      request.maxLedgeHeightMeters < request.minLedgeHeightMeters ||
      !std::isfinite(request.minUsableWidthMeters) || request.minUsableWidthMeters < 0.0F) {
    selection.status = MovementTraversalSlotSelectionStatus::InvalidInput;
    selection.reasonCode = movementTraversalSlotSelectionStatusName(selection.status);
    return selection;
  }

  Vec3 forward;
  if (!normalizeHorizontal(request.forward, forward)) {
    selection.status = MovementTraversalSlotSelectionStatus::InvalidInput;
    selection.reasonCode = movementTraversalSlotSelectionStatusName(selection.status);
    return selection;
  }

  bool sawKind = false;
  bool sawInRange = false;
  bool sawHeightEligible = false;
  bool sawWidthEligible = false;
  bool sawFacing = false;
  float bestRange = 0.0F;
  float bestCandidateRange = 0.0F;

  for (std::size_t index = 0; index < registry.slots.size(); ++index) {
    const MovementTraversalSlot& slot = registry.slots[index];
    if (!kindMatches(slot.kind, request.kind)) {
      continue;
    }
    sawKind = true;
    const float range = horizontalDistanceToBounds(request.actorPosition, slot.frontFaceBounds);
    Vec3 toSlot;
    const float facingDot =
        normalizeHorizontal(center(slot.frontFaceBounds) - request.actorPosition, toSlot)
            ? dot(forward, toSlot)
            : 0.0F;
    const float ledgeHeightFromFeet = slot.topHeightMeters - request.actorPosition.y;
    if (std::isfinite(range) &&
        (selection.candidateSlotIndex == kInvalidTraversalSlotIndex ||
         range < bestCandidateRange ||
         (std::fabs(range - bestCandidateRange) <= kEpsilon &&
          slot.slotId < registry.slots[selection.candidateSlotIndex].slotId))) {
      selection.candidateSlotIndex = index;
      selection.candidateStartRangeMeters = range;
      selection.candidateFacingDot = facingDot;
      selection.candidateLedgeHeightFromFeetMeters = ledgeHeightFromFeet;
      bestCandidateRange = range;
    }
    const float maxRange = std::min(request.maxStartRangeMeters, slot.approachMaxDistanceMeters);
    if (!std::isfinite(range) || range < slot.approachMinDistanceMeters ||
        range > maxRange) {
      continue;
    }
    sawInRange = true;

    if (ledgeHeightFromFeet < request.minLedgeHeightMeters ||
        ledgeHeightFromFeet > request.maxLedgeHeightMeters) {
      continue;
    }
    sawHeightEligible = true;

    if (slot.usableWidthMeters < request.minUsableWidthMeters) {
      continue;
    }
    sawWidthEligible = true;

    if (facingDot < std::max(request.facingDotMin, slot.facingDotMin)) {
      continue;
    }
    sawFacing = true;

    if (selection.slotIndex == kInvalidTraversalSlotIndex || range < bestRange ||
        (std::fabs(range - bestRange) <= kEpsilon &&
         slot.slotId < registry.slots[selection.slotIndex].slotId)) {
      selection.slotIndex = index;
      selection.startRangeMeters = range;
      selection.facingDot = facingDot;
      selection.ledgeHeightFromFeetMeters = ledgeHeightFromFeet;
      bestRange = range;
    }
  }

  if (selection.slotIndex != kInvalidTraversalSlotIndex) {
    selection.status = MovementTraversalSlotSelectionStatus::Found;
  } else if (!sawKind) {
    selection.status = MovementTraversalSlotSelectionStatus::TargetNotFound;
  } else if (!sawInRange) {
    selection.status = MovementTraversalSlotSelectionStatus::OutOfRange;
  } else if (!sawHeightEligible) {
    selection.status = MovementTraversalSlotSelectionStatus::HeightRejected;
  } else if (!sawWidthEligible) {
    selection.status = MovementTraversalSlotSelectionStatus::WidthRejected;
  } else if (!sawFacing) {
    selection.status = MovementTraversalSlotSelectionStatus::NotFacingSlot;
  } else {
    selection.status = MovementTraversalSlotSelectionStatus::TargetNotFound;
  }
  selection.reasonCode = movementTraversalSlotSelectionStatusName(selection.status);
  return selection;
}

const MovementTraversalSlot* selectedTraversalSlot(
    const MovementTraversalSlotRegistry& registry,
    const MovementTraversalSlotSelection& selection) {
  if (selection.slotIndex >= registry.slots.size()) {
    return nullptr;
  }
  return &registry.slots[selection.slotIndex];
}

const MovementTraversalSlot* candidateTraversalSlot(
    const MovementTraversalSlotRegistry& registry,
    const MovementTraversalSlotSelection& selection) {
  if (selection.candidateSlotIndex >= registry.slots.size()) {
    return nullptr;
  }
  return &registry.slots[selection.candidateSlotIndex];
}

const char* movementTraversalSlotKindName(MovementTraversalSlotKind kind) {
  switch (kind) {
    case MovementTraversalSlotKind::Vault:
      return "vault";
    case MovementTraversalSlotKind::Clamber:
      return "clamber";
    case MovementTraversalSlotKind::WireWalk:
      return "wire_walk";
  }
  return "clamber";
}

const char* movementTraversalSlotSelectionStatusName(
    MovementTraversalSlotSelectionStatus status) {
  switch (status) {
    case MovementTraversalSlotSelectionStatus::Found:
      return "slot_found";
    case MovementTraversalSlotSelectionStatus::TargetNotFound:
      return "slot_target_not_found";
    case MovementTraversalSlotSelectionStatus::OutOfRange:
      return "slot_out_of_range";
    case MovementTraversalSlotSelectionStatus::NotFacingSlot:
      return "slot_not_facing";
    case MovementTraversalSlotSelectionStatus::HeightRejected:
      return "slot_height_rejected";
    case MovementTraversalSlotSelectionStatus::WidthRejected:
      return "slot_width_rejected";
    case MovementTraversalSlotSelectionStatus::InvalidInput:
      return "slot_invalid_input";
  }
  return "slot_invalid_input";
}

}  // namespace iggy3d
