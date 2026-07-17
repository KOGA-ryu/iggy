#include "app/iggy3d/creative/spatial/PlacementOrientation.hpp"

#include <cmath>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d::creative {
namespace {

constexpr double kMinimumLengthSquared = 1.0e-24;

[[nodiscard]] double dot(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] CreativeVec3 cross(CreativeVec3 lhs,
                                 CreativeVec3 rhs) noexcept {
  return {lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

[[nodiscard]] CreativeVec3 add(CreativeVec3 lhs,
                               CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] CreativeVec3 multiply(CreativeVec3 value,
                                    double scalar) noexcept {
  return {value.x * scalar, value.y * scalar, value.z * scalar};
}

[[nodiscard]] bool normalize(CreativeVec3 value,
                             CreativeVec3& output) noexcept {
  const double lengthSquared = dot(value, value);
  if (!std::isfinite(lengthSquared) ||
      lengthSquared <= kMinimumLengthSquared) {
    return false;
  }
  output = multiply(value, 1.0 / std::sqrt(lengthSquared));
  return isFiniteCreativeVec3(output);
}

[[nodiscard]] CreativeVec3 projectedOntoPlane(CreativeVec3 value,
                                               CreativeVec3 normal) noexcept {
  return add(value, multiply(normal, -dot(value, normal)));
}

[[nodiscard]] CreativeVec3 mappedLocalAxis(
    CreativeVec3 localAxis,
    CreativeVec3 localRight,
    CreativeVec3 localUp,
    CreativeVec3 localForward,
    CreativeVec3 surfaceRight,
    CreativeVec3 surfaceUp,
    CreativeVec3 surfaceNormal) noexcept {
  return add(add(multiply(surfaceRight, dot(localAxis, localRight)),
                 multiply(surfaceUp, dot(localAxis, localUp))),
             multiply(surfaceNormal, dot(localAxis, localForward)));
}

}  // namespace

CreativePlacementSurfaceFramePlan resolveCreativePlacementSurfaceFrame(
    const CreativePlacementSurfaceFrameRequest& request) noexcept {
  CreativePlacementSurfaceFramePlan result;
  if (!isFiniteCreativeVec3(request.surfaceNormal) ||
      !normalize(request.surfaceNormal, result.surfaceNormal)) {
    return result;
  }
  result.status = CreativePlacementSurfaceFrameStatus::InvalidReference;
  if (!isFiniteCreativeVec3(request.placerForward)) {
    return result;
  }
  result.status = CreativePlacementSurfaceFrameStatus::InvalidLocalForward;
  CreativeVec3 localForward;
  if (!isFiniteCreativeVec3(request.localForward) ||
      !normalize(request.localForward, localForward)) {
    return result;
  }
  result.status = CreativePlacementSurfaceFrameStatus::InvalidRotation;
  if (!std::isfinite(request.rotationAroundNormalRadians)) {
    return result;
  }

  CreativeVec3 localUpHint{0.0, 1.0, 0.0};
  CreativeVec3 localUp;
  if (!normalize(projectedOntoPlane(localUpHint, localForward), localUp)) {
    localUpHint = {0.0, 0.0, 1.0};
    if (!normalize(projectedOntoPlane(localUpHint, localForward), localUp)) {
      localUpHint = {1.0, 0.0, 0.0};
      if (!normalize(projectedOntoPlane(localUpHint, localForward), localUp)) {
        result.status = CreativePlacementSurfaceFrameStatus::DegenerateFrame;
        return result;
      }
    }
  }
  if (!normalize(cross(localUp, localForward), result.localRight) ||
      !normalize(cross(localForward, result.localRight), result.localUp)) {
    result.status = CreativePlacementSurfaceFrameStatus::DegenerateFrame;
    return result;
  }

  CreativeVec3 surfaceUp;
  if (!normalize(projectedOntoPlane({0.0, 1.0, 0.0}, result.surfaceNormal),
                 surfaceUp)) {
    result.usedPlacerFallback = true;
    if (!normalize(projectedOntoPlane(
                       multiply(request.placerForward, -1.0),
                       result.surfaceNormal),
                   surfaceUp) &&
        !normalize(projectedOntoPlane({0.0, 0.0, 1.0},
                                     result.surfaceNormal),
                   surfaceUp) &&
        !normalize(projectedOntoPlane({1.0, 0.0, 0.0},
                                     result.surfaceNormal),
                   surfaceUp)) {
      result.status = CreativePlacementSurfaceFrameStatus::DegenerateFrame;
      return result;
    }
  }
  CreativeVec3 surfaceRight;
  if (!normalize(cross(surfaceUp, result.surfaceNormal), surfaceRight) ||
      !normalize(cross(result.surfaceNormal, surfaceRight), surfaceUp)) {
    result.status = CreativePlacementSurfaceFrameStatus::DegenerateFrame;
    return result;
  }

  const double cosine = std::cos(request.rotationAroundNormalRadians);
  const double sine = std::sin(request.rotationAroundNormalRadians);
  result.surfaceRight =
      add(multiply(surfaceRight, cosine), multiply(surfaceUp, sine));
  result.surfaceUp =
      add(multiply(surfaceUp, cosine), multiply(surfaceRight, -sine));
  if (!isFiniteCreativeVec3(result.surfaceRight) ||
      !isFiniteCreativeVec3(result.surfaceUp)) {
    result.status = CreativePlacementSurfaceFrameStatus::DegenerateFrame;
    return result;
  }

  const CreativeVec3 basisX = mappedLocalAxis(
      {1.0, 0.0, 0.0}, result.localRight, result.localUp, localForward,
      result.surfaceRight, result.surfaceUp, result.surfaceNormal);
  const CreativeVec3 basisY = mappedLocalAxis(
      {0.0, 1.0, 0.0}, result.localRight, result.localUp, localForward,
      result.surfaceRight, result.surfaceUp, result.surfaceNormal);
  const CreativeVec3 basisZ = mappedLocalAxis(
      {0.0, 0.0, 1.0}, result.localRight, result.localUp, localForward,
      result.surfaceRight, result.surfaceUp, result.surfaceNormal);
  result.rotationEulerRadians =
      creativeEulerXyzFromBasis(basisX, basisY, basisZ);
  if (!isFiniteCreativeVec3(result.rotationEulerRadians)) {
    result.status = CreativePlacementSurfaceFrameStatus::DegenerateFrame;
    return result;
  }

  result.status = CreativePlacementSurfaceFrameStatus::Ready;
  result.valid = true;
  return result;
}

}  // namespace iggy3d::creative
