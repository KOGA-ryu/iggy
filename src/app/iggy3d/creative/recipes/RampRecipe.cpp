#include "app/iggy3d/creative/recipes/RampRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <cmath>
#include <numbers>

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

[[nodiscard]] CreativeVec3 normalized(CreativeVec3 value) noexcept {
  const double length =
      std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
  if (!positiveFinite(length)) {
    return {};
  }
  return {value.x / length, value.y / length, value.z / length};
}

void reject(CreativeRampRecipeResult& result,
            CreativeRampRecipeStatus status,
            std::string_view reasonCode) noexcept {
  result.accepted = false;
  result.walkable = false;
  result.status = status;
  result.reasonCode = reasonCode;
}

[[nodiscard]] bool appendSocket(CreativeRampRecipeResult& result,
                                CreativeRampSocketKind kind,
                                CreativeVec3 localPosition) noexcept {
  if (result.socketCount >= result.sockets.size() ||
      kind >= CreativeRampSocketKind::Count ||
      !isFiniteCreativeVec3(localPosition)) {
    return false;
  }
  result.sockets[result.socketCount++] = {kind, localPosition};
  return true;
}

}  // namespace

std::string_view creativeRampSocketName(CreativeRampSocketKind kind) noexcept {
  switch (kind) {
    case CreativeRampSocketKind::EdgeLeft:
      return "ramp_edge_left";
    case CreativeRampSocketKind::EdgeRight:
      return "ramp_edge_right";
    case CreativeRampSocketKind::Count:
      break;
  }
  return "ramp_socket_invalid";
}

std::string_view creativeRampSocketCompatibility(
    CreativeRampSocketKind kind) noexcept {
  switch (kind) {
    case CreativeRampSocketKind::EdgeLeft:
    case CreativeRampSocketKind::EdgeRight:
      return "ramp.edge";
    case CreativeRampSocketKind::Count:
      break;
  }
  return "ramp.invalid";
}

CreativeRampRecipeResult planCreativeRamp(
    const CreativeRampRecipeRequest& request) noexcept {
  CreativeRampRecipeResult result;
  const CreativeBoundsMetrics authored =
      measureCreativeBounds(request.authoredBounds);
  if (!authored.valid || !isPositiveCreativeVec3(authored.size)) {
    reject(result, CreativeRampRecipeStatus::InvalidBounds,
           "creative_ramp_bounds_invalid");
    return result;
  }
  if (!isFiniteCreativeVec3(request.transform.position) ||
      !isFiniteCreativeVec3(request.transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(request.transform.scale) ||
      std::fabs(request.transform.rotationEulerRadians.x) > kGeometryEpsilon ||
      std::fabs(request.transform.rotationEulerRadians.z) > kGeometryEpsilon) {
    reject(result, CreativeRampRecipeStatus::InvalidTransform,
           "creative_ramp_transform_invalid");
    return result;
  }
  if (!positiveFinite(request.landingDepthMeters) ||
      !positiveFinite(request.availableHeadroomMeters) ||
      !positiveFinite(request.minimumHeadroomMeters) ||
      !positiveFinite(request.maximumWalkableSlopeDegrees) ||
      request.maximumWalkableSlopeDegrees >= 90.0) {
    reject(result, CreativeRampRecipeStatus::InvalidDimensions,
           "creative_ramp_dimensions_invalid");
    return result;
  }
  if (request.material >= CreativeStructuralMaterial::Count) {
    reject(result, CreativeRampRecipeStatus::InvalidMaterial,
           "creative_ramp_material_invalid");
    return result;
  }

  result.widthMeters = authored.size.x * request.transform.scale.x;
  result.riseMeters = authored.size.y * request.transform.scale.y;
  result.runMeters = authored.size.z * request.transform.scale.z;
  result.surfaceLengthMeters = std::hypot(result.runMeters, result.riseMeters);
  result.slopeAngleDegrees =
      std::atan2(result.riseMeters, result.runMeters) * 180.0 /
      std::numbers::pi;
  result.maximumWalkableSlopeDegrees =
      request.maximumWalkableSlopeDegrees;
  result.headroomMeters = request.availableHeadroomMeters;
  result.material = request.material;
  if (!positiveFinite(result.widthMeters) ||
      !positiveFinite(result.riseMeters) ||
      !positiveFinite(result.runMeters) ||
      !positiveFinite(result.surfaceLengthMeters) ||
      !std::isfinite(result.slopeAngleDegrees)) {
    reject(result, CreativeRampRecipeStatus::InvalidDimensions,
           "creative_ramp_dimensions_unrepresentable");
    return result;
  }
  if (result.headroomMeters + kGeometryEpsilon <
      request.minimumHeadroomMeters) {
    reject(result, CreativeRampRecipeStatus::InvalidHeadroom,
           "creative_ramp_headroom_insufficient");
    return result;
  }
  if (result.slopeAngleDegrees > request.maximumWalkableSlopeDegrees +
                                    kGeometryEpsilon) {
    reject(result, CreativeRampRecipeStatus::InvalidSlope,
           "creative_ramp_slope_exceeds_movement_limit");
    return result;
  }

  const CreativeVec3 localCenter =
      subtract(authored.center, request.transform.position);
  const double halfWidth = authored.size.x * 0.5;
  const double halfRise = authored.size.y * 0.5;
  const double halfRun = authored.size.z * 0.5;
  const double halfLandingUnscaled =
      request.landingDepthMeters / request.transform.scale.z * 0.5;
  const CreativeVec3 lowerLandingLocal =
      add(localCenter, {0.0, -halfRise, -halfRun - halfLandingUnscaled});
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

  const CreativeVec3 lowCenterLocal =
      add(localCenter, {0.0, -halfRise, -halfRun});
  const CreativeVec3 highCenterLocal =
      add(localCenter, {0.0, halfRise, halfRun});
  result.lowSurfaceCenterMeters = worldPoint(request.transform, lowCenterLocal);
  result.highSurfaceCenterMeters = worldPoint(request.transform, highCenterLocal);
  result.sideEdges = {{
      {worldPoint(request.transform,
                  add(lowCenterLocal, {-halfWidth, 0.0, 0.0})),
       worldPoint(request.transform,
                  add(highCenterLocal, {-halfWidth, 0.0, 0.0}))},
      {worldPoint(request.transform,
                  add(lowCenterLocal, {halfWidth, 0.0, 0.0})),
       worldPoint(request.transform,
                  add(highCenterLocal, {halfWidth, 0.0, 0.0}))},
  }};

  const CreativeVec3 localNormal =
      normalized({0.0, result.runMeters, -result.riseMeters});
  result.surfaceNormal = normalized(rotateCreativeVectorEulerXyz(
      localNormal, request.transform.rotationEulerRadians));
  if (!isFiniteCreativeVec3(result.surfaceNormal) ||
      result.surfaceNormal.y <= 0.0) {
    reject(result, CreativeRampRecipeStatus::InvalidTransform,
           "creative_ramp_surface_normal_invalid");
    return result;
  }

  if (!appendSocket(result, CreativeRampSocketKind::EdgeLeft,
                    add(localCenter, {-halfWidth, 0.0, 0.0})) ||
      !appendSocket(result, CreativeRampSocketKind::EdgeRight,
                    add(localCenter, {halfWidth, 0.0, 0.0}))) {
    reject(result, CreativeRampRecipeStatus::InvalidDimensions,
           "creative_ramp_socket_capacity_exceeded");
    return result;
  }

  result.walkable = true;
  result.accepted = true;
  result.status = CreativeRampRecipeStatus::Ready;
  result.reasonCode = "creative_ramp_ready";
  return result;
}

}  // namespace iggy3d::creative
