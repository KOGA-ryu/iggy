#include "app/iggy3d/creative/recipes/StairRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <cmath>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

[[nodiscard]] bool positiveFinite(double value) noexcept {
  return std::isfinite(value) && value > kGeometryEpsilon;
}

[[nodiscard]] CreativeVec3 add(CreativeVec3 lhs,
                               CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] CreativeVec3 subtract(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] CreativeVec3 multiply(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

[[nodiscard]] CreativeVec3 worldPoint(const CreativeTransform& transform,
                                      CreativeVec3 localPoint) noexcept {
  return add(transform.position,
             rotateCreativeVectorEulerXyz(
                 multiply(localPoint, transform.scale),
                 transform.rotationEulerRadians));
}

void reject(CreativeStairRecipeResult& result,
            CreativeStairRecipeStatus status,
            std::string_view reasonCode) noexcept {
  result = {};
  result.status = status;
  result.reasonCode = reasonCode;
}

[[nodiscard]] bool appendSocket(CreativeStairRecipeResult& result,
                                CreativeStairSocketKind kind,
                                CreativeVec3 localPosition) noexcept {
  if (result.socketCount >= result.sockets.size() ||
      kind >= CreativeStairSocketKind::Count ||
      !isFiniteCreativeVec3(localPosition)) {
    return false;
  }
  result.sockets[result.socketCount++] = {kind, localPosition};
  return true;
}

}  // namespace

std::string_view creativeStairSocketName(
    CreativeStairSocketKind kind) noexcept {
  switch (kind) {
    case CreativeStairSocketKind::RailLeft:
      return "stair_rail_left";
    case CreativeStairSocketKind::RailRight:
      return "stair_rail_right";
    case CreativeStairSocketKind::StringerLeft:
      return "stair_stringer_left";
    case CreativeStairSocketKind::StringerRight:
      return "stair_stringer_right";
    case CreativeStairSocketKind::Count:
      break;
  }
  return "stair_socket_invalid";
}

std::string_view creativeStairSocketCompatibility(
    CreativeStairSocketKind kind) noexcept {
  switch (kind) {
    case CreativeStairSocketKind::RailLeft:
    case CreativeStairSocketKind::RailRight:
      return "stair.rail";
    case CreativeStairSocketKind::StringerLeft:
    case CreativeStairSocketKind::StringerRight:
      return "stair.stringer";
    case CreativeStairSocketKind::Count:
      break;
  }
  return "stair.invalid";
}

CreativeStairRecipeResult planCreativeStair(
    const CreativeStairRecipeRequest& request) noexcept {
  CreativeStairRecipeResult result;
  const CreativeBoundsMetrics authored =
      measureCreativeBounds(request.authoredBounds);
  if (!authored.valid || !isPositiveCreativeVec3(authored.size)) {
    reject(result, CreativeStairRecipeStatus::InvalidBounds,
           "creative_stair_bounds_invalid");
    return result;
  }
  if (!isFiniteCreativeVec3(request.transform.position) ||
      !isFiniteCreativeVec3(request.transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(request.transform.scale) ||
      std::fabs(request.transform.rotationEulerRadians.x) > kGeometryEpsilon ||
      std::fabs(request.transform.rotationEulerRadians.z) > kGeometryEpsilon) {
    reject(result, CreativeStairRecipeStatus::InvalidTransform,
           "creative_stair_transform_invalid");
    return result;
  }
  if (!positiveFinite(request.maximumRiserHeightMeters) ||
      !positiveFinite(request.minimumTreadDepthMeters) ||
      !positiveFinite(request.landingDepthMeters) ||
      !positiveFinite(request.availableHeadroomMeters) ||
      !positiveFinite(request.minimumHeadroomMeters)) {
    reject(result, CreativeStairRecipeStatus::InvalidDimensions,
           "creative_stair_dimensions_invalid");
    return result;
  }
  if (request.availableHeadroomMeters + kGeometryEpsilon <
      request.minimumHeadroomMeters) {
    reject(result, CreativeStairRecipeStatus::InvalidHeadroom,
           "creative_stair_headroom_insufficient");
    return result;
  }

  result.widthMeters = authored.size.x * request.transform.scale.x;
  result.riseMeters = authored.size.y * request.transform.scale.y;
  result.runMeters = authored.size.z * request.transform.scale.z;
  result.headroomMeters = request.availableHeadroomMeters;
  if (!positiveFinite(result.widthMeters) ||
      !positiveFinite(result.riseMeters) ||
      !positiveFinite(result.runMeters)) {
    reject(result, CreativeStairRecipeStatus::InvalidDimensions,
           "creative_stair_dimensions_unrepresentable");
    return result;
  }

  const double requiredSteps =
      std::ceil(result.riseMeters / request.maximumRiserHeightMeters);
  const double maximumSteps =
      std::floor(result.runMeters / request.minimumTreadDepthMeters);
  if (!std::isfinite(requiredSteps) || !std::isfinite(maximumSteps) ||
      requiredSteps <= 0.0 || maximumSteps <= 0.0 ||
      requiredSteps > maximumSteps) {
    reject(result, CreativeStairRecipeStatus::InvalidDimensions,
           "creative_stair_tread_run_insufficient");
    return result;
  }
  if (requiredSteps >
      static_cast<double>(kMaximumCreativeGeneratedGeometrySegmentCount)) {
    reject(result, CreativeStairRecipeStatus::StepCapacityExceeded,
           "creative_stair_step_capacity_exceeded");
    return result;
  }

  result.stepCount = static_cast<std::uint16_t>(requiredSteps);
  result.riserHeightMeters =
      result.riseMeters / static_cast<double>(result.stepCount);
  result.treadDepthMeters =
      result.runMeters / static_cast<double>(result.stepCount);

  const CreativeVec3 localCenter =
      subtract(authored.center, request.transform.position);
  const double halfWidth = authored.size.x * 0.5;
  const double halfRise = authored.size.y * 0.5;
  const double halfRun = authored.size.z * 0.5;
  const double halfLandingUnscaled =
      request.landingDepthMeters / request.transform.scale.z * 0.5;
  const double halfTreadUnscaled =
      result.treadDepthMeters / request.transform.scale.z * 0.5;
  const double riserUnscaled =
      result.riserHeightMeters / request.transform.scale.y;

  const CreativeVec3 lowerLandingLocal =
      add(localCenter,
          {0.0, -halfRise, -halfRun - halfLandingUnscaled});
  const CreativeVec3 upperLandingLocal =
      add(localCenter, {0.0, halfRise, halfRun + halfLandingUnscaled});
  result.lowerLanding = {
      worldPoint(request.transform, lowerLandingLocal),
      {result.widthMeters, 0.0, request.landingDepthMeters},
      request.transform.rotationEulerRadians};
  result.upperLanding = {
      worldPoint(request.transform, upperLandingLocal),
      {result.widthMeters, 0.0, request.landingDepthMeters},
      request.transform.rotationEulerRadians};
  result.lowTreadCenterMeters = worldPoint(
      request.transform,
      add(localCenter,
          {0.0, -halfRise + riserUnscaled,
           -halfRun + halfTreadUnscaled}));
  result.highTreadCenterMeters = worldPoint(
      request.transform,
      add(localCenter,
          {0.0, halfRise, halfRun - halfTreadUnscaled}));

  const CreativeVec3 leftCenter =
      add(localCenter, {-halfWidth, 0.0, 0.0});
  const CreativeVec3 rightCenter =
      add(localCenter, {halfWidth, 0.0, 0.0});
  if (!appendSocket(result, CreativeStairSocketKind::RailLeft, leftCenter) ||
      !appendSocket(result, CreativeStairSocketKind::RailRight, rightCenter) ||
      !appendSocket(result, CreativeStairSocketKind::StringerLeft,
                    leftCenter) ||
      !appendSocket(result, CreativeStairSocketKind::StringerRight,
                    rightCenter)) {
    reject(result, CreativeStairRecipeStatus::StepCapacityExceeded,
           "creative_stair_socket_capacity_exceeded");
    return result;
  }

  result.accepted = true;
  result.status = CreativeStairRecipeStatus::Ready;
  result.reasonCode = "creative_stair_ready";
  return result;
}

}  // namespace iggy3d::creative
