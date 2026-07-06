#pragma once

#include <array>

#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

// An oriented bounding box: a local axis-aligned box (localBounds) carried by a full TRS
// transform. This is what a ROTATED or scaled creative object needs -- an AABB can only bound
// it loosely, so a rotated object can never be picked at its true footprint. Built from an
// object's Transform3 + local CreativeBounds (converted to Aabb3).
//
// Three readers, one primitive:
//   - orientedBoxCorners()  -> the 8 world corners for wireframe render.
//   - orientedBoxWorldAabb() -> the enclosing AABB, the BRIDGE that lets a rotated object be
//     inserted into / queried against the AabbGridIndex broadphase (which keys AABBs).
//   - contains() / intersectsRay() -> the exact rotated narrow phase, run only on the few
//     candidates the broadphase gathers.
// Rotation convention: this kernel is the first place Transform3::rotationEulerRadians is actually
// honored (Transform3::transformPoint applies only scale + translation). It uses intrinsic
// X-then-Y-then-Z Euler (R = Rz * Ry * Rx), Y-up, and places a local point as
// world = position + R * (scale . local).
struct OrientedBox {
  Transform3 transform{};  // world placement: position + euler rotation + scale
  Aabb3 localBounds{};     // the box in the transform's local frame
};

[[nodiscard]] OrientedBox makeOrientedBox(const Transform3& transform,
                                          const Aabb3& localBounds);

// The 8 world-space corners. Deterministic order: corner bit i selects min/max on
// axis i (bit0 = x, bit1 = y, bit2 = z).
[[nodiscard]] std::array<Vec3, 8> orientedBoxCorners(const OrientedBox& box);

// Axis-aligned bound enclosing the oriented box -- the broadphase bridge. Returns a
// zero-extent box at the origin for invalid input.
[[nodiscard]] Aabb3 orientedBoxWorldAabb(const OrientedBox& box);

// Exact point-in-box test in the box's oriented frame. False for invalid input.
[[nodiscard]] bool contains(const OrientedBox& box, Vec3 worldPoint);

struct OrientedBoxRayHit {
  bool hit = false;
  float distanceMeters = 0.0F;  // along a unit-normalized direction; 0 if origin starts inside
  Vec3 pointMeters{};
  bool startInside = false;
};

// Ray-vs-OBB slab test -- the rotated-object pick primitive. `direction` need not be unit
// (it is normalized internally so distanceMeters is in meters). No hit for invalid input,
// zero/degenerate direction, negative maxDistance, or a box the ray never enters within range.
[[nodiscard]] OrientedBoxRayHit intersectsRay(const OrientedBox& box, Vec3 origin,
                                              Vec3 direction,
                                              float maxDistanceMeters);

}  // namespace iggy3d
