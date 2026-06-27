#include "runtime/physics/PhysicsAabbContact.hpp"

#include <algorithm>
#include <array>

namespace iggy3d {
namespace {

struct AxisCandidate {
  float penetrationMeters = 0.0F;
  Vec3 normal;
};

template <std::size_t Count, typename Enum>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1089
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

PhysicsAabbContactResult contactResult(PhysicsAabbContactStatus status,
                                       bool ok) {
  PhysicsAabbContactResult result;
  result.ok = ok;
  result.status = status;
  result.reasonCode = physicsAabbContactStatusName(status);
  return result;
}

float overlapMeters(float lhsMin,
                    float lhsMax,
                    float rhsMin,
                    float rhsMax) {
  return std::min(lhsMax, rhsMax) - std::max(lhsMin, rhsMin);
}

AxisCandidate axisCandidate(float penetrationMeters,
                            float deltaMeters,
                            Vec3 positiveNormal) {
  AxisCandidate candidate;
  candidate.penetrationMeters = penetrationMeters;
  candidate.normal = positiveNormal;
  // branch-gate: BG-1089
  if (deltaMeters < 0.0F) {
    candidate.normal = positiveNormal * -1.0F;
  }
  return candidate;
}

AxisCandidate shallowestAxis(const std::array<AxisCandidate, 3>& axes) {
  AxisCandidate best = axes[0];
  for (std::size_t index = 1U; index < axes.size(); ++index) {
    // branch-gate: BG-1089
    if (axes[index].penetrationMeters < best.penetrationMeters) {
      best = axes[index];
    }
  }
  return best;
}

bool bodyMatchesPair(const PhysicsAabbCollider& first,
                     const PhysicsAabbCollider& second,
                     const PhysicsBroadphasePair& pair) {
  return first.bodyId.value == pair.firstBodyId.value &&
         second.bodyId.value == pair.secondBodyId.value;
}

Vec3 contactPointFromIntersection(const Aabb3& lhs, const Aabb3& rhs) {
  const Aabb3 intersection = makeAabb3(
      {std::max(lhs.min.x, rhs.min.x),
       std::max(lhs.min.y, rhs.min.y),
       std::max(lhs.min.z, rhs.min.z)},
      {std::min(lhs.max.x, rhs.max.x),
       std::min(lhs.max.y, rhs.max.y),
       std::min(lhs.max.z, rhs.max.z)});
  return center(intersection);
}

}  // namespace

std::string_view physicsAabbContactStatusName(
    PhysicsAabbContactStatus status) {
  static constexpr std::array<std::string_view, 8> kNames{
      "physics_aabb_contact_generated",
      "physics_aabb_contact_missing_colliders",
      "physics_aabb_contact_missing_pair",
      "physics_aabb_contact_pair_index_out_of_range",
      "physics_aabb_contact_invalid_collider",
      "physics_aabb_contact_pair_body_mismatch",
      "physics_aabb_contact_same_body_pair",
      "physics_aabb_contact_no_overlap",
  };
  return enumName(status, kNames, "physics_aabb_contact_invalid_collider");
}

PhysicsAabbContactResult generatePhysicsAabbContact(
    const PhysicsAabbContactRequest& request) {
  // branch-gate: BG-1089
  if (request.colliders == nullptr) {
    return contactResult(PhysicsAabbContactStatus::MissingColliders, false);
  }
  // branch-gate: BG-1089
  if (request.pair == nullptr) {
    return contactResult(PhysicsAabbContactStatus::MissingPair, false);
  }
  // branch-gate: BG-1089
  if (request.pair->firstColliderIndex >= request.colliders->size() ||
      request.pair->secondColliderIndex >= request.colliders->size()) {
    return contactResult(PhysicsAabbContactStatus::PairIndexOutOfRange, false);
  }

  const PhysicsAabbCollider& first =
      (*request.colliders)[request.pair->firstColliderIndex];
  const PhysicsAabbCollider& second =
      (*request.colliders)[request.pair->secondColliderIndex];
  // branch-gate: BG-1089
  if (!isValidPhysicsAabbCollider(first)) {
    PhysicsAabbContactResult result =
        contactResult(PhysicsAabbContactStatus::InvalidCollider, false);
    result.invalidColliderIndex = request.pair->firstColliderIndex;
    return result;
  }
  // branch-gate: BG-1089
  if (!isValidPhysicsAabbCollider(second)) {
    PhysicsAabbContactResult result =
        contactResult(PhysicsAabbContactStatus::InvalidCollider, false);
    result.invalidColliderIndex = request.pair->secondColliderIndex;
    return result;
  }
  // branch-gate: BG-1089
  if (!bodyMatchesPair(first, second, *request.pair)) {
    return contactResult(PhysicsAabbContactStatus::PairBodyMismatch, false);
  }
  // branch-gate: BG-1089
  if (first.bodyId.value == second.bodyId.value) {
    return contactResult(PhysicsAabbContactStatus::SameBodyPair, false);
  }

  const float overlapX = overlapMeters(
      first.bounds.min.x, first.bounds.max.x, second.bounds.min.x,
      second.bounds.max.x);
  const float overlapY = overlapMeters(
      first.bounds.min.y, first.bounds.max.y, second.bounds.min.y,
      second.bounds.max.y);
  const float overlapZ = overlapMeters(
      first.bounds.min.z, first.bounds.max.z, second.bounds.min.z,
      second.bounds.max.z);
  // branch-gate: BG-1089
  if (overlapX < 0.0F || overlapY < 0.0F || overlapZ < 0.0F) {
    return contactResult(PhysicsAabbContactStatus::NoOverlap, false);
  }

  const Vec3 delta = second.worldCenterMeters - first.worldCenterMeters;
  const std::array<AxisCandidate, 3> axes{
      axisCandidate(overlapX, delta.x, vec3UnitX()),
      axisCandidate(overlapY, delta.y, vec3UnitY()),
      axisCandidate(overlapZ, delta.z, vec3UnitZ()),
  };
  const AxisCandidate axis = shallowestAxis(axes);

  PhysicsAabbContactResult result =
      contactResult(PhysicsAabbContactStatus::ContactGenerated, true);
  result.contact.firstBodyId = first.bodyId;
  result.contact.secondBodyId = second.bodyId;
  result.contact.firstColliderIndex = request.pair->firstColliderIndex;
  result.contact.secondColliderIndex = request.pair->secondColliderIndex;
  result.contact.normalFromFirstToSecond = axis.normal;
  result.contact.penetrationMeters = axis.penetrationMeters;
  result.contact.pointMeters =
      contactPointFromIntersection(first.bounds, second.bounds);
  result.contact.includesSensor = request.pair->includesSensor ||
                                  first.sensor || second.sensor;
  return result;
}

}  // namespace iggy3d
