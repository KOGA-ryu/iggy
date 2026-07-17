#include "app/iggy3d/creative/spatial/PlacementContact.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "app/iggy3d/creative/Geometry.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] double dot(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] CreativeVec3 add(CreativeVec3 lhs,
                               CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] CreativeVec3 subtract(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] CreativeVec3 multiply(CreativeVec3 value,
                                    double scalar) noexcept {
  return {value.x * scalar, value.y * scalar, value.z * scalar};
}

}  // namespace

CreativePlacementContactPlan resolveCreativePlacementContact(
    const CreativePlacementContactRequest& request) noexcept {
  CreativePlacementContactPlan result;
  if (!request.source.valid ||
      !isFiniteCreativeVec3(request.source.size) ||
      !isPositiveCreativeVec3(request.source.size)) {
    return result;
  }

  result.status = CreativePlacementContactStatus::InvalidTarget;
  if (!isFiniteCreativeVec3(request.targetPoint) ||
      !isFiniteCreativeVec3(request.targetNormal)) {
    return result;
  }
  const double normalLengthSquared = dot(request.targetNormal,
                                         request.targetNormal);
  if (!std::isfinite(normalLengthSquared) ||
      normalLengthSquared <= 1.0e-24) {
    return result;
  }
  const double inverseNormalLength = 1.0 / std::sqrt(normalLengthSquared);
  result.normal = multiply(request.targetNormal, inverseNormalLength);
  result.targetPoint = request.targetPoint;

  std::array<double, 8U> projections{};
  double minimumProjection = std::numeric_limits<double>::infinity();
  for (std::size_t index = 0U; index < request.source.corners.size();
       ++index) {
    const CreativeVec3 corner = request.source.corners[index];
    if (!isFiniteCreativeVec3(corner)) {
      result.status = CreativePlacementContactStatus::InvalidSource;
      return result;
    }
    projections[index] = dot(corner, result.normal);
    minimumProjection = std::min(minimumProjection, projections[index]);
  }

  const double featureTolerance =
      std::max(1.0e-9,
               std::max({request.source.size.x, request.source.size.y,
                         request.source.size.z}) *
                   1.0e-9);
  CreativeVec3 sourceSum{};
  for (std::size_t index = 0U; index < request.source.corners.size();
       ++index) {
    if (projections[index] <= minimumProjection + featureTolerance) {
      sourceSum = add(sourceSum, request.source.corners[index]);
      ++result.sourceFeatureVertexCount;
    }
  }
  if (result.sourceFeatureVertexCount == 0U) {
    result.status = CreativePlacementContactStatus::InvalidSource;
    return result;
  }

  result.sourcePointBeforeTranslation = multiply(
      sourceSum, 1.0 / static_cast<double>(result.sourceFeatureVertexCount));
  result.translation = subtract(result.targetPoint,
                                result.sourcePointBeforeTranslation);
  result.sourcePointAfterTranslation =
      add(result.sourcePointBeforeTranslation, result.translation);
  if (!isFiniteCreativeVec3(result.sourcePointBeforeTranslation) ||
      !isFiniteCreativeVec3(result.translation) ||
      !isFiniteCreativeVec3(result.sourcePointAfterTranslation)) {
    result.status = CreativePlacementContactStatus::InvalidSource;
    return result;
  }

  result.minimumSignedDistanceMeters =
      std::numeric_limits<double>::infinity();
  result.maximumSignedDistanceMeters =
      -std::numeric_limits<double>::infinity();
  for (const CreativeVec3 corner : request.source.corners) {
    const double signedDistance = dot(
        subtract(add(corner, result.translation), result.targetPoint),
        result.normal);
    result.minimumSignedDistanceMeters =
        std::min(result.minimumSignedDistanceMeters, signedDistance);
    result.maximumSignedDistanceMeters =
        std::max(result.maximumSignedDistanceMeters, signedDistance);
  }
  if (!std::isfinite(result.minimumSignedDistanceMeters) ||
      !std::isfinite(result.maximumSignedDistanceMeters) ||
      result.minimumSignedDistanceMeters < -featureTolerance) {
    result.status = CreativePlacementContactStatus::InvalidSource;
    return result;
  }

  result.status = CreativePlacementContactStatus::Ready;
  result.valid = true;
  return result;
}

}  // namespace iggy3d::creative
