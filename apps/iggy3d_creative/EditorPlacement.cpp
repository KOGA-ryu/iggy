#include "EditorPlacement.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <numbers>
#include <string>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "core/math/Snap.hpp"

namespace iggy3d_creative_app {
namespace {

bool positiveFinite(float value) {
  return std::isfinite(value) && value > 0.0F;
}

bool positiveBounds(const iggy3d::creative::CreativeBounds& bounds) {
  const iggy3d::creative::CreativeBoundsMetrics metrics =
      iggy3d::creative::measureCreativeBounds(bounds);
  return metrics.valid &&
         iggy3d::creative::isPositiveCreativeVec3(metrics.size);
}

std::array<iggy3d::creative::CreativePathPoint,
           kCreativeBrushPathPointCapacity>
fixedInitialPathPoints(iggy3d::Vec3 cellCenter) {
  const double cx = static_cast<double>(cellCenter.x);
  const double cy = static_cast<double>(cellCenter.y);
  const double cz = static_cast<double>(cellCenter.z);
  return {{
      iggy3d::creative::CreativePathPoint{{cx - 1.0, cy, cz - 0.5}},
      iggy3d::creative::CreativePathPoint{{cx + 1.0, cy, cz - 0.5}},
      iggy3d::creative::CreativePathPoint{{cx + 1.0, cy, cz + 1.5}},
  }};
}

iggy3d::creative::CreativeBounds centeredProxyBounds(
    const iggy3d::creative::CreativeBounds& authored,
    float thickness) {
  const double half = static_cast<double>(thickness) * 0.5;
  const iggy3d::creative::CreativeBoundsMetrics metrics =
      iggy3d::creative::measureCreativeBounds(authored);
  const double extentX = metrics.size.x;
  const double extentY = metrics.size.y;
  const double extentZ = metrics.size.z;
  iggy3d::creative::CreativeBounds proxy{
      {metrics.center.x - half, metrics.center.y - half,
       metrics.center.z - half},
      {metrics.center.x + half, metrics.center.y + half,
       metrics.center.z + half}};
  if (extentY > extentX && extentY >= extentZ) {
    proxy.min.y = authored.min.y;
    proxy.max.y = authored.max.y;
  } else if (extentZ > extentX && extentZ > extentY) {
    proxy.min.z = authored.min.z;
    proxy.max.z = authored.max.z;
  } else {
    proxy.min.x = authored.min.x;
    proxy.max.x = authored.max.x;
  }
  return proxy;
}

iggy3d::creative::CreativeBounds pathPreviewBounds(
    const std::array<iggy3d::creative::CreativePathPoint,
                     kCreativeBrushPathPointCapacity>& points) {
  const double half =
      static_cast<double>(kCreativeBrushPathPreviewThicknessMeters) * 0.5;
  iggy3d::creative::CreativeBounds bounds{points[0].position,
                                          points[0].position};
  for (std::size_t index = 1U; index < points.size(); ++index) {
    const iggy3d::creative::CreativeVec3& point = points[index].position;
    bounds.min.x = std::min(bounds.min.x, point.x);
    bounds.min.y = std::min(bounds.min.y, point.y);
    bounds.min.z = std::min(bounds.min.z, point.z);
    bounds.max.x = std::max(bounds.max.x, point.x);
    bounds.max.y = std::max(bounds.max.y, point.y);
    bounds.max.z = std::max(bounds.max.z, point.z);
  }
  bounds.min.x -= half;
  bounds.min.y -= half;
  bounds.min.z -= half;
  bounds.max.x += half;
  bounds.max.y += half;
  bounds.max.z += half;
  return bounds;
}

[[nodiscard]] bool isHorizontalPlacementFace(
    iggy3d::creative::CreativePlacementFace face) noexcept {
  using Face = iggy3d::creative::CreativePlacementFace;
  return face == Face::NegativeX || face == Face::PositiveX ||
         face == Face::NegativeZ || face == Face::PositiveZ;
}

[[nodiscard]] iggy3d::creative::CreativePlacementFace horizontalFaceFrom(
    iggy3d::creative::CreativeVec3 direction) noexcept {
  using Face = iggy3d::creative::CreativePlacementFace;
  if (!std::isfinite(direction.x) || !std::isfinite(direction.z)) {
    return Face::Count;
  }
  const double ax = std::fabs(direction.x);
  const double az = std::fabs(direction.z);
  if (std::max(ax, az) <= 1.0e-12) {
    return Face::Count;
  }
  if (ax >= az) {
    return direction.x < 0.0 ? Face::NegativeX : Face::PositiveX;
  }
  return direction.z < 0.0 ? Face::NegativeZ : Face::PositiveZ;
}

[[nodiscard]] iggy3d::creative::CreativePlacementFace oppositeHorizontalFace(
    iggy3d::creative::CreativePlacementFace face) noexcept {
  using Face = iggy3d::creative::CreativePlacementFace;
  switch (face) {
    case Face::NegativeX:
      return Face::PositiveX;
    case Face::PositiveX:
      return Face::NegativeX;
    case Face::NegativeZ:
      return Face::PositiveZ;
    case Face::PositiveZ:
      return Face::NegativeZ;
    case Face::NegativeY:
    case Face::PositiveY:
    case Face::Count:
      return Face::Count;
  }
  return Face::Count;
}

[[nodiscard]] double yawRadiansForForward(
    iggy3d::creative::CreativePlacementFace face) noexcept {
  using Face = iggy3d::creative::CreativePlacementFace;
  switch (face) {
    case Face::PositiveZ:
      return 0.0;
    case Face::PositiveX:
      return std::numbers::pi * 0.5;
    case Face::NegativeZ:
      return std::numbers::pi;
    case Face::NegativeX:
      return -std::numbers::pi * 0.5;
    case Face::NegativeY:
    case Face::PositiveY:
    case Face::Count:
      return 0.0;
  }
  return 0.0;
}

[[nodiscard]] bool orientCardinalPlan(
    CreativeBrushPlacementPlan& plan,
    iggy3d::creative::CreativePlacementFace hitFace,
    iggy3d::creative::CreativeVec3 placerForward,
    iggy3d::creative::CreativePlacementFace localForward) noexcept {
  using Face = iggy3d::creative::CreativePlacementFace;
  if (!plan.hasTransformOverride ||
      !iggy3d::creative::isFiniteCreativeVec3(placerForward)) {
    return false;
  }
  Face forward = hitFace;
  if (!isHorizontalPlacementFace(hitFace)) {
    forward = oppositeHorizontalFace(horizontalFaceFrom(placerForward));
  }
  if (!isHorizontalPlacementFace(forward) ||
      !isHorizontalPlacementFace(localForward)) {
    return false;
  }
  plan.resolvedForward = forward;
  plan.transform.rotationEulerRadians.y =
      yawRadiansForForward(forward) - yawRadiansForForward(localForward);
  plan.orientationResolved = true;
  return std::isfinite(plan.transform.rotationEulerRadians.y);
}

[[nodiscard]] bool applyPlacementYaw(
    CreativeBrushPlacementPlan& plan,
    iggy3d::creative::CreativePlacementYaw placementYaw) noexcept {
  if (static_cast<std::size_t>(placementYaw) >=
      static_cast<std::size_t>(
          iggy3d::creative::CreativePlacementYaw::Count)) {
    return false;
  }
  if (!creativeBrushSupportsPlacementYaw(plan.brush)) {
    return true;
  }
  if (!plan.hasTransformOverride) {
    return false;
  }
  const double offset =
      iggy3d::creative::creativePlacementYawRadians(placementYaw);
  plan.transform.rotationEulerRadians.y += offset;
  plan.orientationResolved = plan.orientationResolved || offset != 0.0;
  return std::isfinite(plan.transform.rotationEulerRadians.y);
}

}  // namespace

std::vector<iggy3d::creative::CreativePathPoint> initialPathPointsForAnchor(
    iggy3d::Vec3 cellCenter) {
  const auto points = fixedInitialPathPoints(cellCenter);
  return {points.begin(), points.end()};
}

BrushFootprint descriptorBoundsFootprint(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  const iggy3d::creative::CreativeBoundsMetrics metrics =
      iggy3d::creative::measureCreativeBounds(descriptor.defaults.bounds);
  const iggy3d::Vec3 size =
      iggy3d::creative::creativeVec3ToCoreChecked(metrics.size).value;
  return {size.x, size.y, size.z};
}

bool validBrushFootprint(BrushFootprint footprint) {
  return positiveFinite(footprint.sizeX) && positiveFinite(footprint.height) &&
         positiveFinite(footprint.sizeZ);
}

bool descriptorSupportsBoxPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  if (descriptor.kind == iggy3d::creative::CreativeObjectKind::Unknown ||
      !descriptor.placementPolicy.enabled ||
      !descriptor.hasTransform || !descriptor.hasBounds ||
      descriptor.projectionProfile !=
          iggy3d::creative::CreativeSpatialProjectionProfile::BoxProjection) {
    return false;
  }
  const BrushFootprint footprint = descriptorBoundsFootprint(descriptor);
  if (!validBrushFootprint(footprint)) {
    return false;
  }
  switch (descriptor.shapeKind) {
    case iggy3d::creative::CreativeObjectShapeKind::BoxVolume:
    case iggy3d::creative::CreativeObjectShapeKind::Surface:
    case iggy3d::creative::CreativeObjectShapeKind::MeshProxy:
      return true;
    case iggy3d::creative::CreativeObjectShapeKind::Unknown:
    case iggy3d::creative::CreativeObjectShapeKind::Line:
    case iggy3d::creative::CreativeObjectShapeKind::Point:
    case iggy3d::creative::CreativeObjectShapeKind::Path:
      return false;
  }
  return false;
}

bool descriptorSupportsLinePlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  if (descriptor.kind == iggy3d::creative::CreativeObjectKind::Unknown ||
      !descriptor.placementPolicy.enabled ||
      descriptor.shapeKind != iggy3d::creative::CreativeObjectShapeKind::Line ||
      !descriptor.hasTransform || !descriptor.hasBounds) {
    return false;
  }

  const BrushFootprint footprint = descriptorBoundsFootprint(descriptor);
  if (!validBrushFootprint(footprint)) {
    return false;
  }

  return descriptor.projectionProfile ==
             iggy3d::creative::CreativeSpatialProjectionProfile::BoxProjection ||
         descriptor.projectionProfile ==
             iggy3d::creative::CreativeSpatialProjectionProfile::LineProjection;
}

bool descriptorSupportsPointPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  return descriptor.kind != iggy3d::creative::CreativeObjectKind::Unknown &&
         descriptor.placementPolicy.enabled &&
         descriptor.shapeKind == iggy3d::creative::CreativeObjectShapeKind::Point &&
         descriptor.hasTransform && !descriptor.hasBounds;
}

bool descriptorSupportsPathPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  return descriptor.kind != iggy3d::creative::CreativeObjectKind::Unknown &&
         descriptor.placementPolicy.enabled &&
         descriptor.shapeKind == iggy3d::creative::CreativeObjectShapeKind::Path &&
         descriptor.projectionProfile ==
             iggy3d::creative::CreativeSpatialProjectionProfile::PathProjection;
}

bool descriptorSupportsBrushPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  return descriptorSupportsBoxPlacement(descriptor) ||
         descriptorSupportsLinePlacement(descriptor) ||
         descriptorSupportsPointPlacement(descriptor) ||
         descriptorSupportsPathPlacement(descriptor);
}

bool creativeBrushSupportsPlacementYaw(
    iggy3d::creative::CreativeObjectKind brush) noexcept {
  const iggy3d::creative::CreativeObjectDescriptor& descriptor =
      iggy3d::creative::describeObject(brush);
  if (!descriptorSupportsBrushPlacement(descriptor) ||
      descriptor.placementPolicy.storagePolicy !=
          iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject ||
      !descriptor.hasTransform) {
    return false;
  }
  switch (descriptor.shapeKind) {
    case iggy3d::creative::CreativeObjectShapeKind::BoxVolume:
    case iggy3d::creative::CreativeObjectShapeKind::Surface:
    case iggy3d::creative::CreativeObjectShapeKind::Line:
    case iggy3d::creative::CreativeObjectShapeKind::MeshProxy:
      return true;
    case iggy3d::creative::CreativeObjectShapeKind::Unknown:
    case iggy3d::creative::CreativeObjectShapeKind::Point:
    case iggy3d::creative::CreativeObjectShapeKind::Path:
      return false;
  }
  return false;
}

bool descriptorAvailableInStandaloneBrushPalette(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  return iggy3d::creative::descriptorShowsInAuthoringBrushPalette(descriptor);
}

BrushFootprint brushFootprintForDescriptor(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  BrushFootprint footprint = descriptorBoundsFootprint(descriptor);
  if (!validBrushFootprint(footprint)) {
    return {};
  }
  return footprint;
}

CreativeBrushPlacementPlan planBrushPlacement(
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter) noexcept {
  CreativeBrushPlacementPlan plan;
  plan.brush = brush;
  const iggy3d::creative::CreativeObjectDescriptor& descriptor =
      iggy3d::creative::describeObject(brush);
  plan.shapeKind = descriptor.shapeKind;
  plan.storagePolicy = descriptor.placementPolicy.storagePolicy;
  if (!iggy3d::isFinite(cellCenter)) {
    plan.status = CreativeBrushPlacementPlanStatus::InvalidAnchor;
    return plan;
  }
  if (!descriptorSupportsBrushPlacement(descriptor)) {
    plan.status = CreativeBrushPlacementPlanStatus::UnsupportedBrush;
    return plan;
  }

  const double cx = static_cast<double>(cellCenter.x);
  const double cy = static_cast<double>(cellCenter.y);
  const double cz = static_cast<double>(cellCenter.z);
  if (descriptor.shapeKind ==
          iggy3d::creative::CreativeObjectShapeKind::Path &&
      descriptor.projectionProfile ==
          iggy3d::creative::CreativeSpatialProjectionProfile::PathProjection) {
    plan.pathPoints = fixedInitialPathPoints(cellCenter);
    plan.pathPointCount =
        static_cast<std::uint8_t>(plan.pathPoints.size());
    plan.previewBounds = pathPreviewBounds(plan.pathPoints);
    plan.hasPathOverride = true;
  } else if (descriptor.shapeKind ==
                 iggy3d::creative::CreativeObjectShapeKind::Point &&
             descriptor.hasTransform && !descriptor.hasBounds) {
    const double half =
        static_cast<double>(kCreativeBrushPointPreviewSizeMeters) * 0.5;
    plan.transform.position = {cx, cy, cz};
    plan.previewBounds = {{cx - half, cy - half, cz - half},
                          {cx + half, cy + half, cz + half}};
    plan.hasTransformOverride = true;
  } else {
    const BrushFootprint footprint = brushFootprintForDescriptor(descriptor);
    if (!validBrushFootprint(footprint)) {
      plan.status = CreativeBrushPlacementPlanStatus::InvalidGeometry;
      return plan;
    }
    const double halfX = static_cast<double>(footprint.sizeX) * 0.5;
    const double halfZ = static_cast<double>(footprint.sizeZ) * 0.5;
    const double height = static_cast<double>(footprint.height);
    plan.transform.position = {cx, cy + height * 0.5, cz};
    plan.authoredBounds = {{cx - halfX, cy, cz - halfZ},
                           {cx + halfX, cy + height, cz + halfZ}};
    plan.previewBounds =
        descriptor.shapeKind ==
                iggy3d::creative::CreativeObjectShapeKind::Line
            ? centeredProxyBounds(
                  plan.authoredBounds,
                  kCreativeBrushLinePreviewThicknessMeters)
            : plan.authoredBounds;
    plan.hasTransformOverride = true;
    plan.hasBoundsOverride = true;
  }

  if (brush == iggy3d::creative::CreativeObjectKind::MovingPlatform) {
    const iggy3d::creative::CreativeBoundsMetrics platform =
        iggy3d::creative::measureCreativeBounds(plan.authoredBounds);
    if (!platform.valid) {
      plan.status = CreativeBrushPlacementPlanStatus::InvalidGeometry;
      return plan;
    }
    plan.pathPoints[0] = {platform.center};
    plan.pathPoints[1] = {{platform.center.x, platform.center.y + 3.0,
                           platform.center.z}};
    plan.pathPointCount = 2U;
    plan.hasPathOverride = true;
  }

  if (!positiveBounds(plan.previewBounds) ||
      (plan.hasBoundsOverride &&
       !positiveBounds(plan.authoredBounds))) {
    plan.status = CreativeBrushPlacementPlanStatus::InvalidGeometry;
    return plan;
  }
  plan.status = CreativeBrushPlacementPlanStatus::Ready;
  plan.valid = true;
  return plan;
}

std::string_view toString(
    CreativeBrushPlacementAdmissionStatus status) noexcept {
  switch (status) {
    case CreativeBrushPlacementAdmissionStatus::Ready:
      return "creative_placement_ready";
    case CreativeBrushPlacementAdmissionStatus::InvalidTarget:
      return "creative_placement_target_invalid";
    case CreativeBrushPlacementAdmissionStatus::UnsupportedBrush:
      return "creative_placement_brush_unsupported";
    case CreativeBrushPlacementAdmissionStatus::InvalidGeometry:
      return "creative_placement_geometry_invalid";
    case CreativeBrushPlacementAdmissionStatus::UnsupportedPolicy:
      return "creative_placement_policy_unsupported";
    case CreativeBrushPlacementAdmissionStatus::FaceDisallowed:
      return "creative_placement_face_disallowed";
    case CreativeBrushPlacementAdmissionStatus::AttachmentOccupied:
      return "creative_placement_attachment_occupied";
  }
  return "creative_placement_status_invalid";
}

CreativeBrushPlacementAdmission admitBrushPlacement(
    iggy3d::creative::CreativeObjectKind brush,
    const iggy3d::creative::CreativeGridTarget& target,
    iggy3d::creative::CreativePlacementYaw placementYaw) noexcept {
  CreativeBrushPlacementAdmission admission;
  admission.plan.brush = brush;
  if (static_cast<std::size_t>(placementYaw) >=
      static_cast<std::size_t>(
          iggy3d::creative::CreativePlacementYaw::Count)) {
    admission.status =
        CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
    return admission;
  }
  if (!target.valid) {
    return admission;
  }

  const iggy3d::creative::CreativeVec3& anchor = target.placementAnchor;
  const iggy3d::creative::CreativeCoreVec3Conversion coreAnchor =
      iggy3d::creative::creativeVec3ToCoreChecked(anchor);
  if (!coreAnchor.converted) {
    return admission;
  }
  admission.plan = planBrushPlacement(brush, coreAnchor.value);
  if (!admission.plan.valid) {
    switch (admission.plan.status) {
      case CreativeBrushPlacementPlanStatus::InvalidAnchor:
        admission.status =
            CreativeBrushPlacementAdmissionStatus::InvalidTarget;
        break;
      case CreativeBrushPlacementPlanStatus::UnsupportedBrush:
        admission.status =
            CreativeBrushPlacementAdmissionStatus::UnsupportedBrush;
        break;
      case CreativeBrushPlacementPlanStatus::InvalidGeometry:
        admission.status =
            CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
        break;
      case CreativeBrushPlacementPlanStatus::Ready:
        admission.status =
            CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
        break;
    }
    return admission;
  }

  const iggy3d::creative::CreativeObjectPlacementPolicy& policy =
      iggy3d::creative::describeObject(brush).placementPolicy;
  if (!policy.enabled ||
      policy.targetPolicy !=
          iggy3d::creative::CreativePlacementTargetPolicy::AdjacentCell) {
    admission.status =
        CreativeBrushPlacementAdmissionStatus::UnsupportedPolicy;
    return admission;
  }
  if (!iggy3d::creative::creativePlacementPolicyAllowsFace(
          policy, target.faceNormal)) {
    admission.status = CreativeBrushPlacementAdmissionStatus::FaceDisallowed;
    return admission;
  }

  admission.plan.resolvedFace =
      iggy3d::creative::creativePlacementFaceFromNormal(target.faceNormal);
  admission.plan.storagePolicy = policy.storagePolicy;
  if (policy.storagePolicy ==
      iggy3d::creative::CreativePlacementStoragePolicy::VoxelCell) {
    if (policy.occupancyPolicy !=
            iggy3d::creative::CreativePlacementOccupancyPolicy::
                RejectOccupied ||
        !positiveBounds(target.adjacentCellBounds)) {
      admission.status =
          CreativeBrushPlacementAdmissionStatus::UnsupportedPolicy;
      return admission;
    }
    const iggy3d::creative::CreativeBounds bounds =
        target.adjacentCellBounds;
    admission.plan.voxelCell = target.adjacentCell;
    admission.plan.hasVoxelCell = true;
    admission.plan.authoredBounds = bounds;
    admission.plan.previewBounds = bounds;
    admission.plan.transform.position =
        iggy3d::creative::measureCreativeBounds(bounds).center;
    admission.plan.transform.rotationEulerRadians = {};
    admission.plan.transform.scale = {1.0, 1.0, 1.0};
    admission.plan.hasTransformOverride = true;
    admission.plan.hasBoundsOverride = true;
    admission.plan.hasPathOverride = false;
    admission.plan.pathPointCount = 0;
    admission.plan.resolvedForward =
        iggy3d::creative::CreativePlacementFace::Count;
    admission.plan.orientationResolved = false;
    admission.status = CreativeBrushPlacementAdmissionStatus::Ready;
    admission.allowed = true;
    return admission;
  }

  if (policy.storagePolicy !=
          iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject ||
      policy.occupancyPolicy !=
          iggy3d::creative::CreativePlacementOccupancyPolicy::AllowOverlap) {
    admission.status =
        CreativeBrushPlacementAdmissionStatus::UnsupportedPolicy;
    return admission;
  }
  switch (policy.orientationPolicy) {
    case iggy3d::creative::CreativePlacementOrientationPolicy::
        DescriptorDefault:
      break;
    case iggy3d::creative::CreativePlacementOrientationPolicy::
        CardinalFaceOrPlacerFacing:
      if (!orientCardinalPlan(admission.plan, admission.plan.resolvedFace,
                              target.placerForward,
                              policy.localForwardFace)) {
        admission.status =
            CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
        return admission;
      }
      break;
    default:
      admission.status =
          CreativeBrushPlacementAdmissionStatus::UnsupportedPolicy;
      return admission;
  }
  if (!applyPlacementYaw(admission.plan, placementYaw)) {
    admission.status =
        CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
    return admission;
  }

  admission.status = CreativeBrushPlacementAdmissionStatus::Ready;
  admission.allowed = true;
  return admission;
}

bool applyCreativeAssetPlacementBounds(
    CreativeBrushPlacementPlan& plan,
    iggy3d::creative::CreativeBounds sourceBounds) noexcept {
  const iggy3d::creative::CreativeBoundsMetrics source =
      iggy3d::creative::measureCreativeBounds(sourceBounds);
  if (!plan.valid ||
      plan.storagePolicy !=
          iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject ||
      !source.valid ||
      !iggy3d::creative::isPositiveCreativeVec3(source.size) ||
      !plan.hasTransformOverride || !plan.hasBoundsOverride) {
    return false;
  }
  const iggy3d::creative::CreativeVec3 targetBottomCenter{
      plan.transform.position.x, plan.authoredBounds.min.y,
      plan.transform.position.z};
  const iggy3d::creative::CreativeVec3 sourceBottomCenter{
      source.center.x, sourceBounds.min.y, source.center.z};
  const iggy3d::creative::CreativeVec3 scaledSourceBottomCenter{
      sourceBottomCenter.x * plan.transform.scale.x,
      sourceBottomCenter.y * plan.transform.scale.y,
      sourceBottomCenter.z * plan.transform.scale.z};
  const iggy3d::creative::CreativeVec3 orientedSourceBottomCenter =
      iggy3d::creative::rotateCreativeVectorEulerXyz(
          scaledSourceBottomCenter, plan.transform.rotationEulerRadians);
  const iggy3d::creative::CreativeVec3 pivot{
      targetBottomCenter.x - orientedSourceBottomCenter.x,
      targetBottomCenter.y - orientedSourceBottomCenter.y,
      targetBottomCenter.z - orientedSourceBottomCenter.z};
  if (!iggy3d::creative::isFiniteCreativeVec3(pivot)) {
    return false;
  }
  plan.authoredBounds = {
      {pivot.x + sourceBounds.min.x, pivot.y + sourceBounds.min.y,
       pivot.z + sourceBounds.min.z},
      {pivot.x + sourceBounds.max.x, pivot.y + sourceBounds.max.y,
       pivot.z + sourceBounds.max.z}};
  plan.previewBounds = plan.authoredBounds;
  plan.transform.position = pivot;
  return positiveBounds(plan.authoredBounds) &&
         iggy3d::creative::resolveCreativeTransformedBounds(
             plan.authoredBounds, plan.transform)
             .valid;
}

bool applyCreativeAssetPlacementTransform(
    CreativeBrushPlacementPlan& plan,
    iggy3d::creative::CreativeBounds sourceBounds,
    iggy3d::creative::CreativeTransform transform) noexcept {
  const iggy3d::creative::CreativeBoundsMetrics source =
      iggy3d::creative::measureCreativeBounds(sourceBounds);
  if (!plan.valid ||
      plan.storagePolicy !=
          iggy3d::creative::CreativePlacementStoragePolicy::AuthoredObject ||
      !source.valid ||
      !iggy3d::creative::isPositiveCreativeVec3(source.size) ||
      !iggy3d::creative::isFiniteCreativeVec3(transform.position) ||
      !iggy3d::creative::isFiniteCreativeVec3(
          transform.rotationEulerRadians) ||
      !iggy3d::creative::isPositiveCreativeVec3(transform.scale)) {
    return false;
  }
  plan.transform = transform;
  plan.authoredBounds = {
      {transform.position.x + sourceBounds.min.x,
       transform.position.y + sourceBounds.min.y,
       transform.position.z + sourceBounds.min.z},
      {transform.position.x + sourceBounds.max.x,
       transform.position.y + sourceBounds.max.y,
       transform.position.z + sourceBounds.max.z}};
  plan.previewBounds = plan.authoredBounds;
  plan.hasTransformOverride = true;
  plan.hasBoundsOverride = true;
  plan.orientationResolved = true;
  return positiveBounds(plan.authoredBounds) &&
         iggy3d::creative::resolveCreativeTransformedBounds(
             plan.authoredBounds, plan.transform)
             .valid;
}

CreativeBrushPlacementAdmission admitBrushPlacement(
    const iggy3d::creative::CreativeHotbarEntry& held,
    const iggy3d::creative::CreativeGridTarget& target,
    iggy3d::creative::CreativePlacementYaw placementYaw) noexcept {
  CreativeBrushPlacementAdmission admission =
      admitBrushPlacement(held.objectKind, target, placementYaw);
  const std::string_view assetId =
      iggy3d::creative::creativeHotbarAssetId(held);
  if (assetId.empty()) {
    return admission;
  }
  if (!admission.allowed || !held.hasAssetBounds ||
      !applyCreativeAssetPlacementBounds(admission.plan,
                                         held.assetSourceBounds)) {
    admission.status = CreativeBrushPlacementAdmissionStatus::InvalidGeometry;
    admission.allowed = false;
  }
  return admission;
}

iggy3d::creative::CreativeBounds creativeBrushHeldPreviewBounds(
    const CreativeBrushPlacementPlan& plan) noexcept {
  if (plan.storagePolicy !=
      iggy3d::creative::CreativePlacementStoragePolicy::VoxelCell) {
    return plan.previewBounds;
  }
  return {{-0.5, 0.0, -0.5}, {0.5, 1.0, 0.5}};
}

std::vector<iggy3d::creative::CreativeObjectKind>
buildBrushPaletteFromDescriptors() {
  std::vector<iggy3d::creative::CreativeObjectKind> palette;
  std::uint64_t eligibleBeforeHygiene = 0;
  std::string removed;
  for (const iggy3d::creative::CreativeObjectDescriptor& descriptor :
       iggy3d::creative::allObjectDescriptors()) {
    if (!descriptorSupportsBrushPlacement(descriptor)) {
      continue;
    }

    ++eligibleBeforeHygiene;
    if (!descriptorAvailableInStandaloneBrushPalette(descriptor)) {
      if (!removed.empty()) {
        removed += ",";
      }
      removed += std::string(descriptor.name);
      continue;
    }

    palette.push_back(descriptor.kind);
  }
  SDL_Log("iggy3d_creative: brush palette hygiene before=%llu after=%llu "
          "removed=%llu predicate='descriptorShowsInAuthoringBrushPalette' "
          "removed='%s'",
          static_cast<unsigned long long>(eligibleBeforeHygiene),
          static_cast<unsigned long long>(palette.size()),
          static_cast<unsigned long long>(eligibleBeforeHygiene -
                                          palette.size()),
          removed.c_str());
  return palette;
}

iggy3d::creative::CreativeObjectKind firstBrushKind(
    const std::vector<iggy3d::creative::CreativeObjectKind>& palette) {
  return palette.empty() ? iggy3d::creative::CreativeObjectKind::Unknown
                         : palette.front();
}

iggy3d::creative::CreativeObjectKind nextBrushKind(
    const std::vector<iggy3d::creative::CreativeObjectKind>& palette,
    iggy3d::creative::CreativeObjectKind current) {
  if (palette.empty()) {
    return iggy3d::creative::CreativeObjectKind::Unknown;
  }

  const auto it = std::find(palette.begin(), palette.end(), current);
  if (it == palette.end()) {
    return palette.front();
  }
  const auto next = std::next(it);
  return next == palette.end() ? palette.front() : *next;
}

iggy3d::Vec3 snapGroundToCellCenter(double worldX,
                                    double worldZ,
                                    double cellSize) {
  const float cell = static_cast<float>(cellSize);
  const iggy3d::Vec3 snapped = iggy3d::snapVec3ToCellCenter(
      {static_cast<float>(worldX), 0.0F, static_cast<float>(worldZ)},
      {cell, cell, cell},
      {0.0F, 0.0F, 0.0F},
      0x5u);
  return {snapped.x, 0.0F, snapped.z};
}

std::string pathPointsSummary(
    const std::vector<iggy3d::creative::CreativePathPoint>& points) {
  std::string summary;
  for (std::size_t index = 0; index < points.size(); ++index) {
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "%s(%.3f,%.3f,%.3f)",
                  index == 0U ? "" : "->", points[index].position.x,
                  points[index].position.y, points[index].position.z);
    summary += buffer;
  }
  return summary;
}

}  // namespace iggy3d_creative_app
