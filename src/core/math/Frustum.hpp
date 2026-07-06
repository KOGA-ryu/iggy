#pragma once

#include <array>
#include <cstdint>

#include "core/math/Aabb3.hpp"
#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

// A half-space plane: a point p is INSIDE (kept) when dot(normal, p) + distance >= 0.
struct Plane {
  Vec3 normal{};
  float distance = 0.0F;
};

enum class FrustumCull : std::uint8_t {
  Outside,       // the box is wholly outside at least one plane -- cull it
  Intersecting,  // the box straddles the frustum boundary -- partially visible
  Inside,        // the box is wholly inside all six planes
};

// Which clip-space depth range the projection targets. The ONLY plane extraction differs by: the
// near plane. OpenGL clips z in [-1, 1]; Vulkan/D3D clip z in [0, 1].
enum class ClipDepthRange : std::uint8_t {
  NegativeOneToOne,  // OpenGL
  ZeroToOne,         // Vulkan / D3D (this engine)
};

// The six frustum planes, always ordered [left, right, bottom, top, near, far], normals pointing
// INWARD (inside the frustum is the positive half-space).
using FrustumPlanes = std::array<Plane, 6>;

// Normalize so `distance` is a true signed distance. A near-zero normal yields a degenerate plane
// (zero normal), which the classifier treats as non-constraining rather than culling everything.
[[nodiscard]] Plane normalizePlane(Plane plane) noexcept;

// Gribb-Hartmann extraction from a clipFromWorld matrix (clip = M * worldHomogeneous, row-major).
// Planes come out normalized and inward-facing. A non-finite matrix yields six degenerate planes,
// so classification is fail-SAFE (nothing culled) rather than culling everything.
[[nodiscard]] FrustumPlanes frustumPlanesFromClip(const Mat4& clipFromWorld,
                                                  ClipDepthRange depthRange) noexcept;

// The cull test the AabbGridIndex's fourth lane feeds: classify an AABB against the six planes with
// the positive-vertex / negative-vertex test (one dot per plane, no per-corner loop). Degenerate
// (zero-normal) planes are skipped. An invalid box is reported Outside (never drawn).
[[nodiscard]] FrustumCull classifyAabbAgainstFrustum(const FrustumPlanes& planes,
                                                     const Aabb3& box) noexcept;

// Convenience: visible == not fully Outside (Inside or Intersecting).
[[nodiscard]] bool aabbInFrustum(const FrustumPlanes& planes, const Aabb3& box) noexcept;

}  // namespace iggy3d
