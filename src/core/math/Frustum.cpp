#include "core/math/Frustum.hpp"

#include <cmath>

namespace iggy3d {

namespace {

// A homogeneous plane row (a, b, c, d): a*x + b*y + c*z + d.
struct PlaneRow {
  float a, b, c, d;
};

PlaneRow row(const Mat4& m, std::uint32_t r) {
  return PlaneRow{at(m, r, 0), at(m, r, 1), at(m, r, 2), at(m, r, 3)};
}

PlaneRow add(const PlaneRow& p, const PlaneRow& q) {
  return PlaneRow{p.a + q.a, p.b + q.b, p.c + q.c, p.d + q.d};
}

PlaneRow sub(const PlaneRow& p, const PlaneRow& q) {
  return PlaneRow{p.a - q.a, p.b - q.b, p.c - q.c, p.d - q.d};
}

Plane toPlane(const PlaneRow& p) {
  return normalizePlane(Plane{Vec3{p.a, p.b, p.c}, p.d});
}

}  // namespace

Plane normalizePlane(Plane plane) noexcept {
  const float lengthSq = lengthSquared(plane.normal);
  if (!(lengthSq > 1e-20F) || !std::isfinite(lengthSq) ||
      !std::isfinite(plane.distance)) {
    return Plane{Vec3{0.0F, 0.0F, 0.0F}, 0.0F};
  }
  const float inv = 1.0F / std::sqrt(lengthSq);
  return Plane{plane.normal * inv, plane.distance * inv};
}

FrustumPlanes frustumPlanesFromClip(const Mat4& clipFromWorld,
                                    ClipDepthRange depthRange) noexcept {
  if (!isFinite(clipFromWorld)) {
    return FrustumPlanes{};  // six degenerate planes -> fail-safe, nothing culled
  }
  const PlaneRow r0 = row(clipFromWorld, 0);
  const PlaneRow r1 = row(clipFromWorld, 1);
  const PlaneRow r2 = row(clipFromWorld, 2);
  const PlaneRow r3 = row(clipFromWorld, 3);

  const PlaneRow near =
      depthRange == ClipDepthRange::ZeroToOne ? r2 : add(r3, r2);

  return FrustumPlanes{
      toPlane(add(r3, r0)),  // left:   x_clip >= -w
      toPlane(sub(r3, r0)),  // right:  x_clip <=  w
      toPlane(add(r3, r1)),  // bottom: y_clip >= -w
      toPlane(sub(r3, r1)),  // top:    y_clip <=  w
      toPlane(near),         // near
      toPlane(sub(r3, r2)),  // far:    z_clip <=  w
  };
}

FrustumCull classifyAabbAgainstFrustum(const FrustumPlanes& planes,
                                       const Aabb3& box) noexcept {
  if (!isValid(box)) {
    return FrustumCull::Outside;
  }

  bool intersecting = false;
  for (const Plane& plane : planes) {
    if (!(lengthSquared(plane.normal) > 1e-20F)) {
      continue;  // degenerate plane: non-constraining
    }
    // Positive vertex: the box corner farthest along the (inward) normal.
    const Vec3 positive{plane.normal.x >= 0.0F ? box.max.x : box.min.x,
                        plane.normal.y >= 0.0F ? box.max.y : box.min.y,
                        plane.normal.z >= 0.0F ? box.max.z : box.min.z};
    if (dot(plane.normal, positive) + plane.distance < 0.0F) {
      return FrustumCull::Outside;  // even the best corner is behind -> wholly outside
    }
    // Negative vertex: the opposite corner. If it is behind, the box straddles this plane.
    const Vec3 negative{plane.normal.x >= 0.0F ? box.min.x : box.max.x,
                        plane.normal.y >= 0.0F ? box.min.y : box.max.y,
                        plane.normal.z >= 0.0F ? box.min.z : box.max.z};
    if (dot(plane.normal, negative) + plane.distance < 0.0F) {
      intersecting = true;
    }
  }
  return intersecting ? FrustumCull::Intersecting : FrustumCull::Inside;
}

bool aabbInFrustum(const FrustumPlanes& planes, const Aabb3& box) noexcept {
  return classifyAabbAgainstFrustum(planes, box) != FrustumCull::Outside;
}

}  // namespace iggy3d
