#include "core/math/OrientedBox.hpp"

#include "core/math/EulerRotation.hpp"

#include <cmath>

namespace iggy3d {

namespace {

// Full TRS placement of a local point: world = translate + rotate(scale . local).
Vec3 placeLocal(const Transform3& transform, Vec3 localPoint) {
  const Vec3 scaled{localPoint.x * transform.scale.x,
                    localPoint.y * transform.scale.y,
                    localPoint.z * transform.scale.z};
  return transform.position + rotateEulerXyz(scaled, transform.rotationEulerRadians);
}

// The oriented box's world frame: its center and its three world-space edge axes. Each axis is
// R * S * unit_i -- orthogonal to the others, with length equal to that axis' scale. Kept
// un-normalized on purpose: the slab/point tests below use |dot(p, e)| <= half * dot(e, e),
// which folds the scale in exactly and needs no sqrt.
struct BoxFrame {
  Vec3 center;
  Vec3 axis[3];
  float half[3];
};

bool buildFrame(const OrientedBox& box, BoxFrame& frame) {
  if (!isFinite(box.transform) || !isValid(box.localBounds)) {
    return false;
  }
  const Vec3 localCenter = center(box.localBounds);
  const Vec3 localHalf = extents(box.localBounds);
  frame.center = placeLocal(box.transform, localCenter);
  frame.axis[0] = placeLocal(box.transform, localCenter + vec3UnitX()) - frame.center;
  frame.axis[1] = placeLocal(box.transform, localCenter + vec3UnitY()) - frame.center;
  frame.axis[2] = placeLocal(box.transform, localCenter + vec3UnitZ()) - frame.center;
  frame.half[0] = localHalf.x;
  frame.half[1] = localHalf.y;
  frame.half[2] = localHalf.z;
  return true;
}

}  // namespace

OrientedBox makeOrientedBox(const Transform3& transform, const Aabb3& localBounds) {
  return OrientedBox{transform, localBounds};
}

std::array<Vec3, 8> orientedBoxCorners(const OrientedBox& box) {
  const Vec3& lo = box.localBounds.min;
  const Vec3& hi = box.localBounds.max;
  std::array<Vec3, 8> corners{};
  for (int i = 0; i < 8; ++i) {
    const Vec3 local{(i & 1) ? hi.x : lo.x, (i & 2) ? hi.y : lo.y,
                     (i & 4) ? hi.z : lo.z};
    corners[static_cast<std::size_t>(i)] = placeLocal(box.transform, local);
  }
  return corners;
}

Aabb3 orientedBoxWorldAabb(const OrientedBox& box) {
  if (!isFinite(box.transform) || !isValid(box.localBounds)) {
    return makeAabb3(vec3Zero(), vec3Zero());
  }
  const std::array<Vec3, 8> corners = orientedBoxCorners(box);
  Vec3 lo = corners[0];
  Vec3 hi = corners[0];
  for (std::size_t i = 1; i < corners.size(); ++i) {
    lo.x = std::fmin(lo.x, corners[i].x);
    lo.y = std::fmin(lo.y, corners[i].y);
    lo.z = std::fmin(lo.z, corners[i].z);
    hi.x = std::fmax(hi.x, corners[i].x);
    hi.y = std::fmax(hi.y, corners[i].y);
    hi.z = std::fmax(hi.z, corners[i].z);
  }
  return makeAabb3(lo, hi);
}

bool contains(const OrientedBox& box, Vec3 worldPoint) {
  BoxFrame frame;
  if (!buildFrame(box, frame) || !isFinite(worldPoint)) {
    return false;
  }
  const Vec3 p0 = worldPoint - frame.center;
  for (int i = 0; i < 3; ++i) {
    const float bound = frame.half[i] * dot(frame.axis[i], frame.axis[i]);
    if (std::fabs(dot(p0, frame.axis[i])) > bound) {
      return false;
    }
  }
  return true;
}

OrientedBoxRayHit intersectsRay(const OrientedBox& box, Vec3 origin,
                                Vec3 direction, float maxDistanceMeters) {
  OrientedBoxRayHit result;
  BoxFrame frame;
  if (!buildFrame(box, frame) || !isFinite(origin) || !isFinite(direction) ||
      !(maxDistanceMeters >= 0.0F)) {
    return result;
  }
  const float dirLenSquared = lengthSquared(direction);
  if (!(dirLenSquared > 1e-12F)) {
    return result;
  }
  const Vec3 unit = direction / std::sqrt(dirLenSquared);
  const Vec3 p0 = origin - frame.center;

  constexpr float kParallelEps = 1e-7F;
  float tEnter = 0.0F;
  float tExit = maxDistanceMeters;
  for (int i = 0; i < 3; ++i) {
    const float bound = frame.half[i] * dot(frame.axis[i], frame.axis[i]);
    const float e = dot(p0, frame.axis[i]);
    const float f = dot(unit, frame.axis[i]);
    if (std::fabs(f) > kParallelEps) {
      float t1 = (-bound - e) / f;
      float t2 = (bound - e) / f;
      if (t1 > t2) {
        const float swap = t1;
        t1 = t2;
        t2 = swap;
      }
      if (t1 > tEnter) {
        tEnter = t1;
      }
      if (t2 < tExit) {
        tExit = t2;
      }
      if (tEnter > tExit) {
        return result;
      }
    } else if (e < -bound || e > bound) {
      return result;  // parallel to this slab and outside it
    }
  }

  result.hit = true;
  result.distanceMeters = tEnter;
  result.pointMeters = origin + unit * tEnter;
  result.startInside = contains(box, origin);
  return result;
}

}  // namespace iggy3d
