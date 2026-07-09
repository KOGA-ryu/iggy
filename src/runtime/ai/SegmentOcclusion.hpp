#pragma once

#include <cstdint>
#include <span>

#include "runtime/physics/PhysicsAabbCollider.hpp"

namespace iggy3d {

enum class SegmentOcclusionVerdict : std::uint8_t {
  Clear,
  Blocked,
  Unknown,
};

SegmentOcclusionVerdict segmentOcclusion(
    std::span<const PhysicsAabbCollider> colliders,
    Vec3 fromEye,
    Vec3 toEye,
    float marginMeters);

}  // namespace iggy3d
