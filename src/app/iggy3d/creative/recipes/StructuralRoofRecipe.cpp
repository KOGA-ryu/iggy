#include "app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/StructuralSurfaceRecipe.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace iggy3d::creative {
namespace {

constexpr double kGeometryEpsilon = 1.0e-9;

void reject(CreativeStructuralRoofRecipeResult& result,
            CreativeStructuralRoofRecipeStatus status,
            std::string_view reasonCode) noexcept {
  result = {};
  result.status = status;
  result.reasonCode = reasonCode;
}

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

[[nodiscard]] CreativeVec3 multiply(CreativeVec3 value,
                                    double scalar) noexcept {
  return {value.x * scalar, value.y * scalar, value.z * scalar};
}

[[nodiscard]] CreativeVec3 cross(CreativeVec3 lhs,
                                 CreativeVec3 rhs) noexcept {
  return {lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

[[nodiscard]] CreativeVec3 normalized(CreativeVec3 value) noexcept {
  const double length =
      std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
  return positiveFinite(length) ? multiply(value, 1.0 / length)
                                : CreativeVec3{};
}

[[nodiscard]] CreativeBounds centeredBounds(CreativeVec3 center,
                                            CreativeVec3 size) noexcept {
  const CreativeVec3 half = multiply(size, 0.5);
  return {subtract(center, half), add(center, half)};
}

[[nodiscard]] CreativeBounds mergedBounds(CreativeBounds lhs,
                                          CreativeBounds rhs) noexcept {
  return {{std::min(lhs.min.x, rhs.min.x), std::min(lhs.min.y, rhs.min.y),
           std::min(lhs.min.z, rhs.min.z)},
          {std::max(lhs.max.x, rhs.max.x), std::max(lhs.max.y, rhs.max.y),
           std::max(lhs.max.z, rhs.max.z)}};
}

[[nodiscard]] bool appendPart(CreativeStructuralRoofRecipeResult& result,
                              CreativeStructuralRoofPartKind partKind,
                              CreativeObjectKind kind,
                              CreativeVec3 center,
                              CreativeVec3 size,
                              CreativeVec3 rotation) noexcept {
  if (result.partCount >= result.parts.size() ||
      partKind >= CreativeStructuralRoofPartKind::Count ||
      !isFiniteCreativeVec3(center) || !isPositiveCreativeVec3(size) ||
      !isFiniteCreativeVec3(rotation)) {
    return false;
  }
  const CreativeBounds bounds = centeredBounds(center, size);
  const CreativeTransformedBounds resolved =
      resolveCreativeTransformedBounds(bounds, {center, rotation, {1.0, 1.0, 1.0}});
  if (!resolved.valid) {
    return false;
  }
  result.parts[result.partCount++] = {partKind, kind, bounds, rotation};
  result.worldBounds = result.partCount == 1U
                           ? resolved.worldBounds
                           : mergedBounds(result.worldBounds,
                                          resolved.worldBounds);
  return true;
}

[[nodiscard]] bool appendSlopedPanel(
    CreativeStructuralRoofRecipeResult& result,
    CreativeStructuralRoofPartKind partKind,
    CreativeObjectKind kind,
    CreativeVec3 lowEdgeCenter,
    CreativeVec3 uphill,
    double eaveLength,
    double horizontalRun,
    double rise,
    double thickness) noexcept {
  const CreativeVec3 horizontal = normalized({uphill.x, 0.0, uphill.z});
  const double slopeLength = std::hypot(horizontalRun, rise);
  if (!isFiniteCreativeVec3(horizontal) || !positiveFinite(eaveLength) ||
      !positiveFinite(horizontalRun) || !positiveFinite(rise) ||
      !positiveFinite(slopeLength) || !positiveFinite(thickness)) {
    return false;
  }
  const CreativeVec3 basisX{horizontal.z, 0.0, -horizontal.x};
  const CreativeVec3 basisZ = normalized(
      {horizontal.x * horizontalRun, rise,
       horizontal.z * horizontalRun});
  const CreativeVec3 basisY = normalized(cross(basisZ, basisX));
  const CreativeVec3 rotation =
      creativeEulerXyzFromBasis(basisX, basisY, basisZ);
  const CreativeVec3 highEdgeCenter =
      add(lowEdgeCenter,
          {horizontal.x * horizontalRun, rise,
           horizontal.z * horizontalRun});
  // The authored slope is the exposed weather face. Keep render and height
  // collision on that plane, with panel thickness extending into the roof.
  const CreativeVec3 center =
      subtract(multiply(add(lowEdgeCenter, highEdgeCenter), 0.5),
               multiply(basisY, thickness * 0.5));
  return basisY.y > 0.0 &&
         appendPart(result, partKind, kind, center,
                    {eaveLength, thickness, slopeLength}, rotation);
}

[[nodiscard]] bool drainageEligible(
    CreativeStructuralRoofStyle style,
    CreativeStructuralRoofRidgeAxis ridgeAxis,
    CreativeStructuralRoofSlopeDirection slopeDirection,
    CreativeStructuralRoofPerimeterEdge edge) noexcept {
  if (style == CreativeStructuralRoofStyle::Flat ||
      style == CreativeStructuralRoofStyle::Hip) {
    return true;
  }
  if (style == CreativeStructuralRoofStyle::Gable) {
    return ridgeAxis == CreativeStructuralRoofRidgeAxis::X
               ? edge == CreativeStructuralRoofPerimeterEdge::North ||
                     edge == CreativeStructuralRoofPerimeterEdge::South
               : edge == CreativeStructuralRoofPerimeterEdge::East ||
                     edge == CreativeStructuralRoofPerimeterEdge::West;
  }
  switch (slopeDirection) {
    case CreativeStructuralRoofSlopeDirection::PositiveX:
      return edge == CreativeStructuralRoofPerimeterEdge::East;
    case CreativeStructuralRoofSlopeDirection::NegativeX:
      return edge == CreativeStructuralRoofPerimeterEdge::West;
    case CreativeStructuralRoofSlopeDirection::PositiveZ:
      return edge == CreativeStructuralRoofPerimeterEdge::South;
    case CreativeStructuralRoofSlopeDirection::NegativeZ:
      return edge == CreativeStructuralRoofPerimeterEdge::North;
    case CreativeStructuralRoofSlopeDirection::Count:
      break;
  }
  return false;
}

[[nodiscard]] bool appendPerimeterFacts(
    CreativeStructuralRoofRecipeResult& result,
    CreativeStructuralRoofStyle style,
    CreativeStructuralRoofRidgeAxis ridgeAxis,
    CreativeStructuralRoofSlopeDirection slopeDirection,
    double minimumX,
    double maximumX,
    double minimumZ,
    double maximumZ,
    double elevation) noexcept {
  const std::array<CreativeStructuralRoofEdgePlan, 4U> edges{{
      {CreativeStructuralRoofPerimeterEdge::North,
       CreativeStructuralRoofEdgeKind::Verge,
       {minimumX, elevation, minimumZ}, {maximumX, elevation, minimumZ},
       {0.0, 0.0, -1.0}},
      {CreativeStructuralRoofPerimeterEdge::East,
       CreativeStructuralRoofEdgeKind::Verge,
       {maximumX, elevation, minimumZ}, {maximumX, elevation, maximumZ},
       {1.0, 0.0, 0.0}},
      {CreativeStructuralRoofPerimeterEdge::South,
       CreativeStructuralRoofEdgeKind::Verge,
       {maximumX, elevation, maximumZ}, {minimumX, elevation, maximumZ},
       {0.0, 0.0, 1.0}},
      {CreativeStructuralRoofPerimeterEdge::West,
       CreativeStructuralRoofEdgeKind::Verge,
       {minimumX, elevation, maximumZ}, {minimumX, elevation, minimumZ},
       {-1.0, 0.0, 0.0}},
  }};
  for (CreativeStructuralRoofEdgePlan edge : edges) {
    const bool eligible =
        drainageEligible(style, ridgeAxis, slopeDirection, edge.edge);
    edge.kind = eligible ? CreativeStructuralRoofEdgeKind::Eave
                         : CreativeStructuralRoofEdgeKind::Verge;
    edge.drainageEligible = eligible;
    if (result.edgeCount >= result.edges.size()) {
      return false;
    }
    result.edges[result.edgeCount++] = edge;
    if (!eligible) {
      continue;
    }
    if (result.drainageSocketCount >= result.drainageSockets.size()) {
      return false;
    }
    result.drainageSockets[result.drainageSocketCount++] = {
        edge.edge, multiply(add(edge.startMeters, edge.endMeters), 0.5),
        edge.outward};
  }
  return true;
}

[[nodiscard]] CreativeVec3 lowEdgeCenter(
    CreativeStructuralRoofPerimeterEdge edge,
    double minimumX,
    double maximumX,
    double minimumZ,
    double maximumZ,
    double elevation) noexcept {
  const double centerX = (minimumX + maximumX) * 0.5;
  const double centerZ = (minimumZ + maximumZ) * 0.5;
  switch (edge) {
    case CreativeStructuralRoofPerimeterEdge::North:
      return {centerX, elevation, minimumZ};
    case CreativeStructuralRoofPerimeterEdge::East:
      return {maximumX, elevation, centerZ};
    case CreativeStructuralRoofPerimeterEdge::South:
      return {centerX, elevation, maximumZ};
    case CreativeStructuralRoofPerimeterEdge::West:
      return {minimumX, elevation, centerZ};
    case CreativeStructuralRoofPerimeterEdge::Count:
      break;
  }
  return {};
}

[[nodiscard]] double dot(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] CreativeVec3 worldVectorToLocal(
    CreativeVec3 world,
    CreativeVec3 rotationEulerRadians) noexcept {
  const CreativeVec3 axisX =
      rotateCreativeVectorEulerXyz({1.0, 0.0, 0.0}, rotationEulerRadians);
  const CreativeVec3 axisY =
      rotateCreativeVectorEulerXyz({0.0, 1.0, 0.0}, rotationEulerRadians);
  const CreativeVec3 axisZ =
      rotateCreativeVectorEulerXyz({0.0, 0.0, 1.0}, rotationEulerRadians);
  return {dot(world, axisX), dot(world, axisY), dot(world, axisZ)};
}

[[nodiscard]] CreativeStructuralRoofPerimeterEdge edgeForOutward(
    CreativeVec3 outward) noexcept {
  if (!isFiniteCreativeVec3(outward) ||
      std::fabs(outward.y) > 1.0e-4) {
    return CreativeStructuralRoofPerimeterEdge::Count;
  }
  if (std::fabs(outward.x) >= std::fabs(outward.z)) {
    return outward.x >= 0.0 ? CreativeStructuralRoofPerimeterEdge::East
                            : CreativeStructuralRoofPerimeterEdge::West;
  }
  return outward.z >= 0.0 ? CreativeStructuralRoofPerimeterEdge::South
                          : CreativeStructuralRoofPerimeterEdge::North;
}

[[nodiscard]] bool appendPartSocket(
    CreativeStructuralRoofPartSocketResult& result,
    CreativeStructuralRoofPerimeterEdge edge,
    CreativeVec3 localPosition,
    CreativeVec3 localForward,
    CreativeVec3 localUp) noexcept {
  if (result.socketCount >= result.sockets.size() ||
      edge >= CreativeStructuralRoofPerimeterEdge::Count ||
      !isFiniteCreativeVec3(localPosition) ||
      !isFiniteCreativeVec3(localForward) ||
      !isFiniteCreativeVec3(localUp)) {
    return false;
  }
  result.sockets[result.socketCount++] =
      {edge, localPosition, localForward, localUp};
  return true;
}

struct LocalRoofAperture {
  std::size_t apertureIndex = 0U;
  std::size_t sourcePartIndex = 0U;
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
};

struct LocalRoofRect {
  double minimumX = 0.0;
  double maximumX = 0.0;
  double minimumZ = 0.0;
  double maximumZ = 0.0;
};

[[nodiscard]] bool near(double lhs, double rhs) noexcept {
  return std::fabs(lhs - rhs) <= kGeometryEpsilon;
}

[[nodiscard]] bool validAperture(
    const CreativeStructuralRoofAperture& aperture) noexcept {
  return aperture.kind < CreativeStructuralRoofApertureKind::Count &&
         std::isfinite(aperture.minimumX) &&
         std::isfinite(aperture.maximumX) &&
         std::isfinite(aperture.minimumZ) &&
         std::isfinite(aperture.maximumZ) &&
         aperture.minimumX < aperture.maximumX &&
         aperture.minimumZ < aperture.maximumZ;
}

[[nodiscard]] bool aperturesHaveClearance(
    const CreativeStructuralRoofAperture& lhs,
    const CreativeStructuralRoofAperture& rhs,
    double clearance) noexcept {
  return lhs.maximumX + clearance <= rhs.minimumX + kGeometryEpsilon ||
         rhs.maximumX + clearance <= lhs.minimumX + kGeometryEpsilon ||
         lhs.maximumZ + clearance <= rhs.minimumZ + kGeometryEpsilon ||
         rhs.maximumZ + clearance <= lhs.minimumZ + kGeometryEpsilon;
}

[[nodiscard]] bool projectApertureToPanel(
    const CreativeStructuralRoofPart& part,
    std::size_t sourcePartIndex,
    const CreativeStructuralRoofAperture& aperture,
    std::size_t apertureIndex,
    double clearance,
    LocalRoofAperture& output) noexcept {
  const CreativeBoundsMetrics panel = measureCreativeBounds(part.bounds);
  if (!panel.valid) {
    return false;
  }
  const CreativeVec3 axisX = rotateCreativeVectorEulerXyz(
      {1.0, 0.0, 0.0}, part.rotationEulerRadians);
  const CreativeVec3 axisY = rotateCreativeVectorEulerXyz(
      {0.0, 1.0, 0.0}, part.rotationEulerRadians);
  const CreativeVec3 axisZ = rotateCreativeVectorEulerXyz(
      {0.0, 0.0, 1.0}, part.rotationEulerRadians);
  if (!isFiniteCreativeVec3(axisX) || !isFiniteCreativeVec3(axisY) ||
      !isFiniteCreativeVec3(axisZ) || axisY.y <= kGeometryEpsilon) {
    return false;
  }
  const CreativeVec3 weatherPlane =
      add(panel.center, multiply(axisY, panel.size.y * 0.5));
  const std::array<std::array<double, 2U>, 4U> planCorners{{
      {aperture.minimumX, aperture.minimumZ},
      {aperture.maximumX, aperture.minimumZ},
      {aperture.maximumX, aperture.maximumZ},
      {aperture.minimumX, aperture.maximumZ},
  }};

  output = {apertureIndex, sourcePartIndex,
            std::numeric_limits<double>::max(),
            -std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max(),
            -std::numeric_limits<double>::max()};
  for (const auto& corner : planCorners) {
    const double worldY =
        weatherPlane.y -
        (axisY.x * (corner[0] - weatherPlane.x) +
         axisY.z * (corner[1] - weatherPlane.z)) /
            axisY.y;
    const CreativeVec3 offset =
        subtract({corner[0], worldY, corner[1]}, panel.center);
    const double localX = dot(offset, axisX);
    const double localY = dot(offset, axisY);
    const double localZ = dot(offset, axisZ);
    if (!std::isfinite(worldY) || !std::isfinite(localX) ||
        !std::isfinite(localY) || !std::isfinite(localZ) ||
        !near(localY, panel.size.y * 0.5)) {
      return false;
    }
    output.minimumX = std::min(output.minimumX, localX);
    output.maximumX = std::max(output.maximumX, localX);
    output.minimumZ = std::min(output.minimumZ, localZ);
    output.maximumZ = std::max(output.maximumZ, localZ);
  }

  const double halfX = panel.size.x * 0.5;
  const double halfZ = panel.size.z * 0.5;
  return output.minimumX >= -halfX + clearance - kGeometryEpsilon &&
         output.maximumX <= halfX - clearance + kGeometryEpsilon &&
         output.minimumZ >= -halfZ + clearance - kGeometryEpsilon &&
         output.maximumZ <= halfZ - clearance + kGeometryEpsilon &&
         positiveFinite(output.maximumX - output.minimumX) &&
         positiveFinite(output.maximumZ - output.minimumZ);
}

[[nodiscard]] bool appendUniqueBoundary(
    std::array<double, 2U + 2U * kCreativeStructuralRoofApertureCapacity>&
        boundaries,
    std::size_t& count,
    double value) noexcept {
  if (count >= boundaries.size() || !std::isfinite(value)) {
    return false;
  }
  boundaries[count++] = value;
  return true;
}

void sortAndUniqueBoundaries(
    std::array<double, 2U + 2U * kCreativeStructuralRoofApertureCapacity>&
        boundaries,
    std::size_t& count) noexcept {
  std::sort(boundaries.begin(),
            boundaries.begin() + static_cast<std::ptrdiff_t>(count));
  std::size_t uniqueCount = 0U;
  for (std::size_t index = 0U; index < count; ++index) {
    if (uniqueCount == 0U ||
        !near(boundaries[index], boundaries[uniqueCount - 1U])) {
      boundaries[uniqueCount++] = boundaries[index];
    }
  }
  count = uniqueCount;
}

[[nodiscard]] bool localPointInsideAperture(
    double x,
    double z,
    const std::array<LocalRoofAperture,
                     kCreativeStructuralRoofApertureCapacity>& apertures,
    std::size_t apertureCount,
    std::size_t sourcePartIndex) noexcept {
  for (std::size_t index = 0U; index < apertureCount; ++index) {
    const LocalRoofAperture& aperture = apertures[index];
    if (aperture.sourcePartIndex == sourcePartIndex &&
        x > aperture.minimumX - kGeometryEpsilon &&
        x < aperture.maximumX + kGeometryEpsilon &&
        z > aperture.minimumZ - kGeometryEpsilon &&
        z < aperture.maximumZ + kGeometryEpsilon) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool appendOrExtendRect(
    std::array<LocalRoofRect,
               kCreativeStructuralRoofAperturePieceCapacity>& rects,
    std::size_t& rectCount,
    LocalRoofRect candidate) noexcept {
  for (std::size_t index = 0U; index < rectCount; ++index) {
    LocalRoofRect& existing = rects[index];
    if (near(existing.minimumX, candidate.minimumX) &&
        near(existing.maximumX, candidate.maximumX) &&
        near(existing.maximumZ, candidate.minimumZ)) {
      existing.maximumZ = candidate.maximumZ;
      return true;
    }
  }
  if (rectCount >= rects.size()) {
    return false;
  }
  rects[rectCount++] = candidate;
  return true;
}

[[nodiscard]] bool buildSubpanel(
    const CreativeStructuralRoofPart& source,
    LocalRoofRect rect,
    CreativeStructuralRoofPart& output) noexcept {
  const CreativeBoundsMetrics panel = measureCreativeBounds(source.bounds);
  const CreativeVec3 size{rect.maximumX - rect.minimumX, panel.size.y,
                          rect.maximumZ - rect.minimumZ};
  if (!panel.valid || !isPositiveCreativeVec3(size)) {
    return false;
  }
  const CreativeVec3 localCenter{
      (rect.minimumX + rect.maximumX) * 0.5, 0.0,
      (rect.minimumZ + rect.maximumZ) * 0.5};
  const CreativeVec3 worldCenter =
      add(panel.center,
          rotateCreativeVectorEulerXyz(localCenter,
                                       source.rotationEulerRadians));
  const CreativeBounds bounds = centeredBounds(worldCenter, size);
  const CreativeTransformedBounds resolved = resolveCreativeTransformedBounds(
      bounds, {worldCenter, source.rotationEulerRadians, {1.0, 1.0, 1.0}});
  if (!resolved.valid) {
    return false;
  }
  output = source;
  output.bounds = bounds;
  return true;
}

[[nodiscard]] bool buildApertureInsert(
    const CreativeStructuralRoofPart& source,
    const LocalRoofAperture& aperture,
    CreativeStructuralRoofApertureInsertPlan& output) noexcept {
  CreativeStructuralRoofPart panel;
  if (!buildSubpanel(
          source,
          {aperture.minimumX, aperture.maximumX, aperture.minimumZ,
           aperture.maximumZ},
          panel)) {
    return false;
  }
  output.apertureIndex = aperture.apertureIndex;
  output.sourcePartIndex = aperture.sourcePartIndex;
  output.kind = CreativeObjectKind::Window;
  output.bounds = panel.bounds;
  output.rotationEulerRadians = panel.rotationEulerRadians;
  return true;
}

void rejectApertures(CreativeStructuralRoofApertureResult& result,
                     CreativeStructuralRoofApertureStatus status,
                     std::string_view reasonCode,
                     std::size_t failedApertureIndex =
                         kInvalidCreativeStructuralRoofApertureIndex) noexcept {
  result.accepted = false;
  result.status = status;
  result.pieces = {};
  result.pieceCount = 0U;
  result.inserts = {};
  result.insertCount = 0U;
  result.failedApertureIndex = failedApertureIndex;
  result.reasonCode = reasonCode;
}

}  // namespace

bool validCreativeStructuralRoofSettings(
    CreativeStructuralRoofStyle style,
    CreativeStructuralRoofRidgeAxis ridgeAxis,
    CreativeStructuralRoofSlopeDirection slopeDirection,
    double pitchDegrees,
    double overhangMeters,
    CreativeStructuralMaterial material) noexcept {
  return style < CreativeStructuralRoofStyle::Count &&
         ridgeAxis < CreativeStructuralRoofRidgeAxis::Count &&
         slopeDirection < CreativeStructuralRoofSlopeDirection::Count &&
         std::isfinite(pitchDegrees) &&
         pitchDegrees >= kMinimumCreativeStructuralRoofPitchDegrees &&
         pitchDegrees <= kMaximumCreativeStructuralRoofPitchDegrees &&
         std::isfinite(overhangMeters) && overhangMeters >= 0.0 &&
         material < CreativeStructuralMaterial::Count;
}

std::string_view toString(CreativeStructuralRoofStyle style) noexcept {
  switch (style) {
    case CreativeStructuralRoofStyle::Flat:
      return "Flat";
    case CreativeStructuralRoofStyle::Gable:
      return "Gable";
    case CreativeStructuralRoofStyle::Shed:
      return "Shed";
    case CreativeStructuralRoofStyle::Hip:
      return "Hip";
    case CreativeStructuralRoofStyle::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(CreativeStructuralRoofRidgeAxis axis) noexcept {
  switch (axis) {
    case CreativeStructuralRoofRidgeAxis::X:
      return "X axis";
    case CreativeStructuralRoofRidgeAxis::Z:
      return "Z axis";
    case CreativeStructuralRoofRidgeAxis::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(
    CreativeStructuralRoofSlopeDirection direction) noexcept {
  switch (direction) {
    case CreativeStructuralRoofSlopeDirection::PositiveX:
      return "Downhill +X";
    case CreativeStructuralRoofSlopeDirection::NegativeX:
      return "Downhill -X";
    case CreativeStructuralRoofSlopeDirection::PositiveZ:
      return "Downhill +Z";
    case CreativeStructuralRoofSlopeDirection::NegativeZ:
      return "Downhill -Z";
    case CreativeStructuralRoofSlopeDirection::Count:
      break;
  }
  return "Invalid";
}

std::string_view creativeStructuralRoofDrainageSocketName(
    CreativeStructuralRoofPerimeterEdge edge) noexcept {
  switch (edge) {
    case CreativeStructuralRoofPerimeterEdge::North:
      return "roof_drainage_north";
    case CreativeStructuralRoofPerimeterEdge::East:
      return "roof_drainage_east";
    case CreativeStructuralRoofPerimeterEdge::South:
      return "roof_drainage_south";
    case CreativeStructuralRoofPerimeterEdge::West:
      return "roof_drainage_west";
    case CreativeStructuralRoofPerimeterEdge::Count:
      break;
  }
  return "roof_drainage_invalid";
}

std::string_view creativeStructuralRoofDrainageCompatibility() noexcept {
  return "roof.drainage";
}

CreativeStructuralRoofPartSocketResult planCreativeStructuralRoofPartSockets(
    CreativeObjectKind kind,
    const CreativeBounds& authoredBounds,
    const CreativeTransform& transform) noexcept {
  CreativeStructuralRoofPartSocketResult result;
  const CreativeBoundsMetrics metrics = measureCreativeBounds(authoredBounds);
  if (!metrics.valid || !isFiniteCreativeVec3(transform.position) ||
      !isFiniteCreativeVec3(transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(transform.scale)) {
    result.reasonCode =
        "creative_structural_roof_part_sockets_geometry_invalid";
    return result;
  }

  const CreativeVec3 localWorldUp = worldVectorToLocal(
      {0.0, 1.0, 0.0}, transform.rotationEulerRadians);
  const CreativeVec3 half = multiply(metrics.size, 0.5);
  if (kind == CreativeObjectKind::Roof) {
    struct LocalEdge {
      CreativeVec3 position;
      CreativeVec3 outward;
    };
    const std::array<LocalEdge, 4U> edges{{
        {{0.0, half.y, -half.z}, {0.0, 0.0, -1.0}},
        {{half.x, half.y, 0.0}, {1.0, 0.0, 0.0}},
        {{0.0, half.y, half.z}, {0.0, 0.0, 1.0}},
        {{-half.x, half.y, 0.0}, {-1.0, 0.0, 0.0}},
    }};
    for (const LocalEdge& local : edges) {
      const CreativeVec3 worldOutward = rotateCreativeVectorEulerXyz(
          local.outward, transform.rotationEulerRadians);
      const CreativeVec3 horizontalOutward =
          normalized({worldOutward.x, 0.0, worldOutward.z});
      if (!appendPartSocket(
              result, edgeForOutward(horizontalOutward), local.position,
              worldVectorToLocal(horizontalOutward,
                                 transform.rotationEulerRadians),
              localWorldUp)) {
        result = {};
        result.reasonCode =
            "creative_structural_roof_part_sockets_capacity_exceeded";
        return result;
      }
    }
  } else if (kind == CreativeObjectKind::RoofSlope ||
             kind == CreativeObjectKind::HipRoof) {
    const CreativeVec3 worldUphill = rotateCreativeVectorEulerXyz(
        {0.0, 0.0, 1.0}, transform.rotationEulerRadians);
    const CreativeVec3 worldOutward =
        normalized({-worldUphill.x, 0.0, -worldUphill.z});
    if (!appendPartSocket(
            result, edgeForOutward(worldOutward),
            {0.0, half.y, -half.z},
            worldVectorToLocal(worldOutward,
                               transform.rotationEulerRadians),
            localWorldUp)) {
      result.reasonCode =
          "creative_structural_roof_part_sockets_geometry_invalid";
      return result;
    }
  } else {
    result.reasonCode = "creative_structural_roof_part_sockets_kind_invalid";
    return result;
  }

  result.accepted = result.socketCount > 0U;
  result.reasonCode = result.accepted
                          ? "creative_structural_roof_part_sockets_ready"
                          : "creative_structural_roof_part_sockets_empty";
  return result;
}

CreativeStructuralRoofRecipeResult planCreativeStructuralRoof(
    const CreativeStructuralRoofRecipeRequest& request) noexcept {
  CreativeStructuralRoofRecipeResult result;
  if (!std::isfinite(request.minimumX) ||
      !std::isfinite(request.maximumX) ||
      !std::isfinite(request.minimumZ) ||
      !std::isfinite(request.maximumZ) ||
      request.minimumX >= request.maximumX ||
      request.minimumZ >= request.maximumZ) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidFootprint,
           "creative_structural_roof_footprint_invalid");
    return result;
  }
  if (!std::isfinite(request.supportPlaneMeters)) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidSupportPlane,
           "creative_structural_roof_support_invalid");
    return result;
  }
  if (request.layerCount == 0U) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidLayerCount,
           "creative_structural_roof_layers_invalid");
    return result;
  }
  if (request.style >= CreativeStructuralRoofStyle::Count) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidStyle,
           "creative_structural_roof_style_invalid");
    return result;
  }
  if (request.ridgeAxis >= CreativeStructuralRoofRidgeAxis::Count) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidRidgeAxis,
           "creative_structural_roof_ridge_axis_invalid");
    return result;
  }
  if (request.slopeDirection >=
      CreativeStructuralRoofSlopeDirection::Count) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidSlopeDirection,
           "creative_structural_roof_slope_direction_invalid");
    return result;
  }
  if (!std::isfinite(request.pitchDegrees) ||
      request.pitchDegrees < kMinimumCreativeStructuralRoofPitchDegrees ||
      request.pitchDegrees > kMaximumCreativeStructuralRoofPitchDegrees) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidPitch,
           "creative_structural_roof_pitch_invalid");
    return result;
  }
  if (!std::isfinite(request.overhangMeters) ||
      request.overhangMeters < 0.0) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidOverhang,
           "creative_structural_roof_overhang_invalid");
    return result;
  }
  if (request.material >= CreativeStructuralMaterial::Count) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidMaterial,
           "creative_structural_roof_material_invalid");
    return result;
  }

  const double minimumX = request.minimumX - request.overhangMeters;
  const double maximumX = request.maximumX + request.overhangMeters;
  const double minimumZ = request.minimumZ - request.overhangMeters;
  const double maximumZ = request.maximumZ + request.overhangMeters;
  const double width = maximumX - minimumX;
  const double depth = maximumZ - minimumZ;
  if (!positiveFinite(width) || !positiveFinite(depth)) {
    reject(result, CreativeStructuralRoofRecipeStatus::UnrepresentableGeometry,
           "creative_structural_roof_overhang_unrepresentable");
    return result;
  }

  const CreativeStructuralSurfaceRecipeResult thickness =
      planCreativeStructuralSurface(
          {CreativeObjectKind::Roof, minimumX, maximumX, minimumZ, maximumZ,
           request.supportPlaneMeters, request.layerCount});
  if (!thickness.accepted || !positiveFinite(thickness.totalThicknessMeters)) {
    reject(result, CreativeStructuralRoofRecipeStatus::UnrepresentableGeometry,
           thickness.reasonCode);
    return result;
  }
  result.thicknessMeters = thickness.totalThicknessMeters;
  result.material = request.material;

  if (!appendPerimeterFacts(result, request.style, request.ridgeAxis,
                            request.slopeDirection, minimumX, maximumX,
                            minimumZ, maximumZ,
                            request.style == CreativeStructuralRoofStyle::Flat
                                ? thickness.bounds.max.y
                                : request.supportPlaneMeters)) {
    reject(result, CreativeStructuralRoofRecipeStatus::CapacityExceeded,
           "creative_structural_roof_edge_capacity_exceeded");
    return result;
  }

  if (request.style == CreativeStructuralRoofStyle::Flat) {
    const CreativeBoundsMetrics panel = measureCreativeBounds(thickness.bounds);
    if (!panel.valid ||
        !appendPart(result, CreativeStructuralRoofPartKind::FlatPanel,
                    CreativeObjectKind::Roof, panel.center, panel.size, {})) {
      reject(result, CreativeStructuralRoofRecipeStatus::CapacityExceeded,
             "creative_structural_roof_part_capacity_exceeded");
      return result;
    }
    result.accepted = true;
    result.status = CreativeStructuralRoofRecipeStatus::Ready;
    result.reasonCode = "creative_structural_roof_flat_ready";
    return result;
  }

  const double pitchRadians =
      request.pitchDegrees * std::numbers::pi / 180.0;
  const auto riseForRun = [pitchRadians](double run) {
    return std::tan(pitchRadians) * run;
  };
  const double centerX = (minimumX + maximumX) * 0.5;
  const double centerZ = (minimumZ + maximumZ) * 0.5;

  if (request.style == CreativeStructuralRoofStyle::Shed) {
    CreativeStructuralRoofPerimeterEdge lowEdge =
        CreativeStructuralRoofPerimeterEdge::South;
    CreativeVec3 uphill{0.0, 0.0, -1.0};
    double run = depth;
    double eaveLength = width;
    switch (request.slopeDirection) {
      case CreativeStructuralRoofSlopeDirection::PositiveX:
        lowEdge = CreativeStructuralRoofPerimeterEdge::East;
        uphill = {-1.0, 0.0, 0.0};
        run = width;
        eaveLength = depth;
        break;
      case CreativeStructuralRoofSlopeDirection::NegativeX:
        lowEdge = CreativeStructuralRoofPerimeterEdge::West;
        uphill = {1.0, 0.0, 0.0};
        run = width;
        eaveLength = depth;
        break;
      case CreativeStructuralRoofSlopeDirection::PositiveZ:
        break;
      case CreativeStructuralRoofSlopeDirection::NegativeZ:
        lowEdge = CreativeStructuralRoofPerimeterEdge::North;
        uphill = {0.0, 0.0, 1.0};
        break;
      case CreativeStructuralRoofSlopeDirection::Count:
        break;
    }
    result.riseMeters = riseForRun(run);
    if (!positiveFinite(result.riseMeters) ||
        !appendSlopedPanel(
            result, CreativeStructuralRoofPartKind::ShedPanel,
            CreativeObjectKind::RoofSlope,
            lowEdgeCenter(lowEdge, minimumX, maximumX, minimumZ, maximumZ,
                          request.supportPlaneMeters),
            uphill, eaveLength, run, result.riseMeters,
            result.thicknessMeters)) {
      reject(result,
             CreativeStructuralRoofRecipeStatus::UnrepresentableGeometry,
             "creative_structural_roof_shed_unrepresentable");
      return result;
    }
    const CreativeVec3 highCenter = add(
        lowEdgeCenter(lowEdge, minimumX, maximumX, minimumZ, maximumZ,
                      request.supportPlaneMeters),
        {uphill.x * run, result.riseMeters, uphill.z * run});
    const CreativeVec3 halfEave{uphill.z * eaveLength * 0.5, 0.0,
                               -uphill.x * eaveLength * 0.5};
    result.ridgeStart = subtract(highCenter, halfEave);
    result.ridgeEnd = add(highCenter, halfEave);
    result.accepted = true;
    result.status = CreativeStructuralRoofRecipeStatus::Ready;
    result.reasonCode = "creative_structural_roof_shed_ready";
    return result;
  }

  const bool ridgeAlongX =
      request.ridgeAxis == CreativeStructuralRoofRidgeAxis::X;
  const double ridgeLength = ridgeAlongX ? width : depth;
  const double crossSpan = ridgeAlongX ? depth : width;
  const double run = crossSpan * 0.5;
  result.riseMeters = riseForRun(run);
  if (!positiveFinite(ridgeLength) || !positiveFinite(run) ||
      !positiveFinite(result.riseMeters)) {
    reject(result, CreativeStructuralRoofRecipeStatus::UnrepresentableGeometry,
           "creative_structural_roof_slope_unrepresentable");
    return result;
  }

  if (request.style == CreativeStructuralRoofStyle::Gable) {
    const bool appended =
        ridgeAlongX
            ? appendSlopedPanel(
                  result, CreativeStructuralRoofPartKind::GableFirst,
                  CreativeObjectKind::RoofSlope,
                  {centerX, request.supportPlaneMeters, minimumZ},
                  {0.0, 0.0, 1.0}, width, run, result.riseMeters,
                  result.thicknessMeters) &&
                  appendSlopedPanel(
                      result, CreativeStructuralRoofPartKind::GableSecond,
                      CreativeObjectKind::RoofSlope,
                      {centerX, request.supportPlaneMeters, maximumZ},
                      {0.0, 0.0, -1.0}, width, run, result.riseMeters,
                      result.thicknessMeters)
            : appendSlopedPanel(
                  result, CreativeStructuralRoofPartKind::GableFirst,
                  CreativeObjectKind::RoofSlope,
                  {minimumX, request.supportPlaneMeters, centerZ},
                  {1.0, 0.0, 0.0}, depth, run, result.riseMeters,
                  result.thicknessMeters) &&
                  appendSlopedPanel(
                      result, CreativeStructuralRoofPartKind::GableSecond,
                      CreativeObjectKind::RoofSlope,
                      {maximumX, request.supportPlaneMeters, centerZ},
                      {-1.0, 0.0, 0.0}, depth, run, result.riseMeters,
                      result.thicknessMeters);
    if (!appended) {
      reject(result, CreativeStructuralRoofRecipeStatus::CapacityExceeded,
             "creative_structural_roof_part_capacity_exceeded");
      return result;
    }
    if (ridgeAlongX) {
      result.ridgeStart = {minimumX, request.supportPlaneMeters + result.riseMeters,
                           centerZ};
      result.ridgeEnd = {maximumX, request.supportPlaneMeters + result.riseMeters,
                         centerZ};
    } else {
      result.ridgeStart = {centerX, request.supportPlaneMeters + result.riseMeters,
                           minimumZ};
      result.ridgeEnd = {centerX, request.supportPlaneMeters + result.riseMeters,
                         maximumZ};
    }
    result.accepted = true;
    result.status = CreativeStructuralRoofRecipeStatus::Ready;
    result.reasonCode = "creative_structural_roof_gable_ready";
    return result;
  }

  if (ridgeLength + kGeometryEpsilon < crossSpan) {
    reject(result, CreativeStructuralRoofRecipeStatus::InvalidRidgeSpan,
           "creative_structural_roof_hip_ridge_axis_too_short");
    return result;
  }
  const double ridgeInset = run;
  const bool appended =
      ridgeAlongX
          ? appendSlopedPanel(
                result, CreativeStructuralRoofPartKind::HipNorth,
                CreativeObjectKind::HipRoof,
                {centerX, request.supportPlaneMeters, minimumZ},
                {0.0, 0.0, 1.0}, width, run, result.riseMeters,
                result.thicknessMeters) &&
                appendSlopedPanel(
                    result, CreativeStructuralRoofPartKind::HipSouth,
                    CreativeObjectKind::HipRoof,
                    {centerX, request.supportPlaneMeters, maximumZ},
                    {0.0, 0.0, -1.0}, width, run, result.riseMeters,
                    result.thicknessMeters) &&
                appendSlopedPanel(
                    result, CreativeStructuralRoofPartKind::HipWest,
                    CreativeObjectKind::HipRoof,
                    {minimumX, request.supportPlaneMeters, centerZ},
                    {1.0, 0.0, 0.0}, depth, run, result.riseMeters,
                    result.thicknessMeters) &&
                appendSlopedPanel(
                    result, CreativeStructuralRoofPartKind::HipEast,
                    CreativeObjectKind::HipRoof,
                    {maximumX, request.supportPlaneMeters, centerZ},
                    {-1.0, 0.0, 0.0}, depth, run, result.riseMeters,
                    result.thicknessMeters)
          : appendSlopedPanel(
                result, CreativeStructuralRoofPartKind::HipWest,
                CreativeObjectKind::HipRoof,
                {minimumX, request.supportPlaneMeters, centerZ},
                {1.0, 0.0, 0.0}, depth, run, result.riseMeters,
                result.thicknessMeters) &&
                appendSlopedPanel(
                    result, CreativeStructuralRoofPartKind::HipEast,
                    CreativeObjectKind::HipRoof,
                    {maximumX, request.supportPlaneMeters, centerZ},
                    {-1.0, 0.0, 0.0}, depth, run, result.riseMeters,
                    result.thicknessMeters) &&
                appendSlopedPanel(
                    result, CreativeStructuralRoofPartKind::HipNorth,
                    CreativeObjectKind::HipRoof,
                    {centerX, request.supportPlaneMeters, minimumZ},
                    {0.0, 0.0, 1.0}, width, run, result.riseMeters,
                    result.thicknessMeters) &&
                appendSlopedPanel(
                    result, CreativeStructuralRoofPartKind::HipSouth,
                    CreativeObjectKind::HipRoof,
                    {centerX, request.supportPlaneMeters, maximumZ},
                    {0.0, 0.0, -1.0}, width, run, result.riseMeters,
                    result.thicknessMeters);
  if (!appended) {
    reject(result, CreativeStructuralRoofRecipeStatus::CapacityExceeded,
           "creative_structural_roof_part_capacity_exceeded");
    return result;
  }
  if (ridgeAlongX) {
    result.ridgeStart = {minimumX + ridgeInset,
                         request.supportPlaneMeters + result.riseMeters,
                         centerZ};
    result.ridgeEnd = {maximumX - ridgeInset,
                       request.supportPlaneMeters + result.riseMeters,
                       centerZ};
  } else {
    result.ridgeStart = {centerX,
                         request.supportPlaneMeters + result.riseMeters,
                         minimumZ + ridgeInset};
    result.ridgeEnd = {centerX,
                       request.supportPlaneMeters + result.riseMeters,
                       maximumZ - ridgeInset};
  }
  result.accepted = true;
  result.status = CreativeStructuralRoofRecipeStatus::Ready;
  result.reasonCode = "creative_structural_roof_hip_ready";
  return result;
}

CreativeStructuralRoofApertureResult planCreativeStructuralRoofApertures(
    const CreativeStructuralRoofApertureRequest& request) noexcept {
  CreativeStructuralRoofApertureResult result;
  result.roof = planCreativeStructuralRoof(request.roof);
  if (!result.roof.accepted) {
    rejectApertures(result,
                    CreativeStructuralRoofApertureStatus::InvalidRoof,
                    result.roof.reasonCode);
    return result;
  }
  if (request.apertureCount > request.apertures.size()) {
    rejectApertures(result,
                    CreativeStructuralRoofApertureStatus::InvalidCount,
                    "creative_structural_roof_aperture_count_invalid");
    return result;
  }
  if (!std::isfinite(request.minimumClearanceMeters) ||
      request.minimumClearanceMeters < 0.0) {
    rejectApertures(result,
                    CreativeStructuralRoofApertureStatus::InvalidClearance,
                    "creative_structural_roof_aperture_clearance_invalid");
    return result;
  }

  if (request.apertureCount == 0U) {
    for (std::size_t partIndex = 0U;
         partIndex < result.roof.partCount; ++partIndex) {
      result.pieces[result.pieceCount++] =
          {partIndex, result.roof.parts[partIndex]};
    }
    result.accepted = true;
    result.status = CreativeStructuralRoofApertureStatus::Ready;
    result.reasonCode = "creative_structural_roof_aperture_ready";
    return result;
  }
  if (request.roof.style == CreativeStructuralRoofStyle::Hip) {
    rejectApertures(
        result, CreativeStructuralRoofApertureStatus::UnsupportedRoofStyle,
        "creative_structural_roof_aperture_hip_polygon_storage_required", 0U);
    return result;
  }

  const double minimumRoofX =
      request.roof.minimumX - request.roof.overhangMeters;
  const double maximumRoofX =
      request.roof.maximumX + request.roof.overhangMeters;
  const double minimumRoofZ =
      request.roof.minimumZ - request.roof.overhangMeters;
  const double maximumRoofZ =
      request.roof.maximumZ + request.roof.overhangMeters;
  for (std::size_t apertureIndex = 0U;
       apertureIndex < request.apertureCount; ++apertureIndex) {
    const CreativeStructuralRoofAperture& aperture =
        request.apertures[apertureIndex];
    if (!validAperture(aperture)) {
      rejectApertures(result,
                      CreativeStructuralRoofApertureStatus::InvalidAperture,
                      "creative_structural_roof_aperture_invalid",
                      apertureIndex);
      return result;
    }
    if (aperture.minimumX < minimumRoofX +
                                    request.minimumClearanceMeters -
                                    kGeometryEpsilon ||
        aperture.maximumX > maximumRoofX -
                                    request.minimumClearanceMeters +
                                    kGeometryEpsilon ||
        aperture.minimumZ < minimumRoofZ +
                                    request.minimumClearanceMeters -
                                    kGeometryEpsilon ||
        aperture.maximumZ > maximumRoofZ -
                                    request.minimumClearanceMeters +
                                    kGeometryEpsilon) {
      rejectApertures(
          result,
          CreativeStructuralRoofApertureStatus::ApertureOutsideRoof,
          "creative_structural_roof_aperture_outside_roof", apertureIndex);
      return result;
    }
    for (std::size_t otherIndex = 0U; otherIndex < apertureIndex;
         ++otherIndex) {
      if (!aperturesHaveClearance(
              aperture, request.apertures[otherIndex],
              request.minimumClearanceMeters)) {
        rejectApertures(
            result,
            CreativeStructuralRoofApertureStatus::AperturesTooClose,
            "creative_structural_roof_apertures_too_close", apertureIndex);
        return result;
      }
    }
  }

  std::array<LocalRoofAperture,
             kCreativeStructuralRoofApertureCapacity>
      localApertures{};
  for (std::size_t apertureIndex = 0U;
       apertureIndex < request.apertureCount; ++apertureIndex) {
    std::size_t matchCount = 0U;
    LocalRoofAperture matched;
    for (std::size_t partIndex = 0U; partIndex < result.roof.partCount;
         ++partIndex) {
      LocalRoofAperture candidate;
      if (projectApertureToPanel(
              result.roof.parts[partIndex], partIndex,
              request.apertures[apertureIndex], apertureIndex,
              request.minimumClearanceMeters, candidate)) {
        matched = candidate;
        ++matchCount;
      }
    }
    if (matchCount != 1U) {
      rejectApertures(
          result,
          CreativeStructuralRoofApertureStatus::
              ApertureCrossesPanelBoundary,
          "creative_structural_roof_aperture_crosses_panel_boundary",
          apertureIndex);
      return result;
    }
    localApertures[apertureIndex] = matched;
  }

  for (std::size_t partIndex = 0U; partIndex < result.roof.partCount;
       ++partIndex) {
    const CreativeStructuralRoofPart& source = result.roof.parts[partIndex];
    const CreativeBoundsMetrics panel = measureCreativeBounds(source.bounds);
    if (!panel.valid) {
      rejectApertures(
          result,
          CreativeStructuralRoofApertureStatus::UnrepresentableGeometry,
          "creative_structural_roof_aperture_panel_invalid");
      return result;
    }
    std::size_t panelApertureCount = 0U;
    for (std::size_t apertureIndex = 0U;
         apertureIndex < request.apertureCount; ++apertureIndex) {
      panelApertureCount +=
          localApertures[apertureIndex].sourcePartIndex == partIndex ? 1U : 0U;
    }
    if (panelApertureCount == 0U) {
      if (result.pieceCount >= result.pieces.size()) {
        rejectApertures(
            result,
            CreativeStructuralRoofApertureStatus::CapacityExceeded,
            "creative_structural_roof_aperture_piece_capacity_exceeded");
        return result;
      }
      result.pieces[result.pieceCount++] = {partIndex, source};
      continue;
    }

    std::array<double,
               2U + 2U * kCreativeStructuralRoofApertureCapacity>
        xBoundaries{};
    std::array<double,
               2U + 2U * kCreativeStructuralRoofApertureCapacity>
        zBoundaries{};
    std::size_t xCount = 0U;
    std::size_t zCount = 0U;
    if (!appendUniqueBoundary(xBoundaries, xCount, -panel.size.x * 0.5) ||
        !appendUniqueBoundary(xBoundaries, xCount, panel.size.x * 0.5) ||
        !appendUniqueBoundary(zBoundaries, zCount, -panel.size.z * 0.5) ||
        !appendUniqueBoundary(zBoundaries, zCount, panel.size.z * 0.5)) {
      rejectApertures(
          result,
          CreativeStructuralRoofApertureStatus::UnrepresentableGeometry,
          "creative_structural_roof_aperture_boundaries_invalid");
      return result;
    }
    for (std::size_t apertureIndex = 0U;
         apertureIndex < request.apertureCount; ++apertureIndex) {
      const LocalRoofAperture& aperture = localApertures[apertureIndex];
      if (aperture.sourcePartIndex != partIndex) {
        continue;
      }
      if (!appendUniqueBoundary(xBoundaries, xCount, aperture.minimumX) ||
          !appendUniqueBoundary(xBoundaries, xCount, aperture.maximumX) ||
          !appendUniqueBoundary(zBoundaries, zCount, aperture.minimumZ) ||
          !appendUniqueBoundary(zBoundaries, zCount, aperture.maximumZ)) {
        rejectApertures(
            result,
            CreativeStructuralRoofApertureStatus::CapacityExceeded,
            "creative_structural_roof_aperture_boundary_capacity_exceeded");
        return result;
      }
    }
    sortAndUniqueBoundaries(xBoundaries, xCount);
    sortAndUniqueBoundaries(zBoundaries, zCount);

    std::array<LocalRoofRect,
               kCreativeStructuralRoofAperturePieceCapacity>
        rects{};
    std::size_t rectCount = 0U;
    for (std::size_t zIndex = 0U; zIndex + 1U < zCount; ++zIndex) {
      const double zMidpoint =
          (zBoundaries[zIndex] + zBoundaries[zIndex + 1U]) * 0.5;
      std::size_t xIndex = 0U;
      while (xIndex + 1U < xCount) {
        const double xMidpoint =
            (xBoundaries[xIndex] + xBoundaries[xIndex + 1U]) * 0.5;
        if (localPointInsideAperture(
                xMidpoint, zMidpoint, localApertures,
                request.apertureCount, partIndex)) {
          ++xIndex;
          continue;
        }
        const std::size_t runStart = xIndex;
        do {
          ++xIndex;
          if (xIndex + 1U >= xCount) {
            break;
          }
          const double nextMidpoint =
              (xBoundaries[xIndex] + xBoundaries[xIndex + 1U]) * 0.5;
          if (localPointInsideAperture(
                  nextMidpoint, zMidpoint, localApertures,
                  request.apertureCount, partIndex)) {
            break;
          }
        } while (true);
        if (!appendOrExtendRect(
                rects, rectCount,
                {xBoundaries[runStart], xBoundaries[xIndex],
                 zBoundaries[zIndex], zBoundaries[zIndex + 1U]})) {
          rejectApertures(
              result,
              CreativeStructuralRoofApertureStatus::CapacityExceeded,
              "creative_structural_roof_aperture_piece_capacity_exceeded");
          return result;
        }
      }
    }
    if (rectCount == 0U) {
      rejectApertures(
          result,
          CreativeStructuralRoofApertureStatus::UnrepresentableGeometry,
          "creative_structural_roof_aperture_consumes_panel");
      return result;
    }
    for (std::size_t rectIndex = 0U; rectIndex < rectCount; ++rectIndex) {
      if (result.pieceCount >= result.pieces.size()) {
        rejectApertures(
            result,
            CreativeStructuralRoofApertureStatus::CapacityExceeded,
            "creative_structural_roof_aperture_piece_capacity_exceeded");
        return result;
      }
      CreativeStructuralRoofPart part;
      if (!buildSubpanel(source, rects[rectIndex], part)) {
        rejectApertures(
            result,
            CreativeStructuralRoofApertureStatus::
                UnrepresentableGeometry,
            "creative_structural_roof_aperture_piece_unrepresentable");
        return result;
      }
      result.pieces[result.pieceCount++] = {partIndex, part};
    }
  }

  for (std::size_t apertureIndex = 0U;
       apertureIndex < request.apertureCount; ++apertureIndex) {
    if (request.apertures[apertureIndex].kind !=
        CreativeStructuralRoofApertureKind::Skylight) {
      continue;
    }
    if (result.insertCount >= result.inserts.size() ||
        !buildApertureInsert(
            result.roof.parts[
                localApertures[apertureIndex].sourcePartIndex],
            localApertures[apertureIndex],
            result.inserts[result.insertCount])) {
      rejectApertures(
          result,
          CreativeStructuralRoofApertureStatus::UnrepresentableGeometry,
          "creative_structural_roof_aperture_insert_unrepresentable",
          apertureIndex);
      return result;
    }
    ++result.insertCount;
  }

  result.accepted = true;
  result.status = CreativeStructuralRoofApertureStatus::Ready;
  result.reasonCode = "creative_structural_roof_aperture_ready";
  return result;
}

}  // namespace iggy3d::creative
