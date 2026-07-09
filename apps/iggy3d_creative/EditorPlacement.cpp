#include "EditorPlacement.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <string>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "core/math/Snap.hpp"

namespace iggy3d_creative_app {
namespace {

bool positiveFinite(float value) {
  return std::isfinite(value) && value > 0.0F;
}

}  // namespace

std::vector<iggy3d::creative::CreativePathPoint> initialPathPointsForAnchor(
    iggy3d::Vec3 cellCenter) {
  const double cx = static_cast<double>(cellCenter.x);
  const double cz = static_cast<double>(cellCenter.z);
  return {
      iggy3d::creative::CreativePathPoint{{cx - 1.0, 0.0, cz - 0.5}},
      iggy3d::creative::CreativePathPoint{{cx + 1.0, 0.0, cz - 0.5}},
      iggy3d::creative::CreativePathPoint{{cx + 1.0, 0.0, cz + 1.5}},
  };
}

BrushFootprint descriptorBoundsFootprint(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  const iggy3d::creative::CreativeBounds& bounds = descriptor.defaults.bounds;
  return {static_cast<float>(bounds.max.x - bounds.min.x),
          static_cast<float>(bounds.max.y - bounds.min.y),
          static_cast<float>(bounds.max.z - bounds.min.z)};
}

bool validBrushFootprint(BrushFootprint footprint) {
  return positiveFinite(footprint.sizeX) && positiveFinite(footprint.height) &&
         positiveFinite(footprint.sizeZ);
}

bool isStandingSurfaceFootprint(BrushFootprint footprint) {
  return footprint.height > std::min(footprint.sizeX, footprint.sizeZ);
}

bool descriptorSupportsBoxPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  if (descriptor.kind == iggy3d::creative::CreativeObjectKind::Unknown ||
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
         descriptor.shapeKind == iggy3d::creative::CreativeObjectShapeKind::Point &&
         descriptor.hasTransform && !descriptor.hasBounds;
}

bool descriptorSupportsPathPlacement(
    const iggy3d::creative::CreativeObjectDescriptor& descriptor) {
  return descriptor.kind != iggy3d::creative::CreativeObjectKind::Unknown &&
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

  if (descriptor.shapeKind == iggy3d::creative::CreativeObjectShapeKind::Surface &&
      descriptor.occupancyKind ==
          iggy3d::creative::CreativeSpatialOccupancyKind::Structural &&
      isStandingSurfaceFootprint(footprint)) {
    // Preserve the current wall brush proof while deriving the decision from
    // descriptor shape/occupancy. A future descriptor placement-footprint column
    // can delete these standalone editing constants.
    return {std::max(footprint.sizeX, footprint.sizeZ), 2.5F, 0.25F};
  }

  return footprint;
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

iggy3d::creative::CreativeDocumentCreateRequest buildBrushCreateRequest(
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter,
    std::uint64_t ordinal) {
  const iggy3d::creative::CreativeObjectDescriptor& descriptor =
      iggy3d::creative::describeObject(brush);
  const double cx = static_cast<double>(cellCenter.x);
  const double cz = static_cast<double>(cellCenter.z);

  iggy3d::creative::CreativeDocumentCreateRequest request;
  request.kind = brush;
  request.name = std::string(iggy3d::creative::toString(brush)) + " placed#" +
                 std::to_string(ordinal);
  if (descriptor.shapeKind == iggy3d::creative::CreativeObjectShapeKind::Path &&
      descriptor.projectionProfile ==
          iggy3d::creative::CreativeSpatialProjectionProfile::PathProjection) {
    request.hasPathOverride = true;
    request.pathPoints = initialPathPointsForAnchor(cellCenter);
  } else if (descriptor.shapeKind ==
                 iggy3d::creative::CreativeObjectShapeKind::Point &&
             descriptor.hasTransform && !descriptor.hasBounds) {
    // Point-shape authored truth is the transform anchor. The marker box used
    // for render/hit/wire feedback is app-local visualization only.
    request.transform.position = {cx, 0.0, cz};
    request.hasTransformOverride = true;
  } else {
    const BrushFootprint fp = brushFootprintForDescriptor(descriptor);
    const double halfX = static_cast<double>(fp.sizeX) * 0.5;
    const double halfZ = static_cast<double>(fp.sizeZ) * 0.5;
    const double height = static_cast<double>(fp.height);
    // Position = cell center at half-height so the box straddles the footprint
    // and rests on Y=0.
    request.transform.position = {cx, height * 0.5, cz};
    request.hasTransformOverride = true;
    // Footprint centered in XZ on the cell, min.y=0 so it sits ON the ground.
    request.bounds = {{cx - halfX, 0.0, cz - halfZ},
                      {cx + halfX, height, cz + halfZ}};
    request.hasBoundsOverride = true;
  }
  request.visible = true;
  request.hasVisibleOverride = true;
  request.locked = false;
  request.hasLockedOverride = true;

  return request;
}

iggy3d::creative::CreativeDocumentCreateReceipt placeBrushObject(
    iggy3d::creative::Facade& facade,
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter,
    std::uint64_t ordinal) {
  const iggy3d::creative::CreativeObjectDescriptor& descriptor =
      iggy3d::creative::describeObject(brush);
  const iggy3d::creative::CreativeDocumentCreateRequest request =
      buildBrushCreateRequest(brush, cellCenter, ordinal);

  const iggy3d::creative::CreativeDocumentCreateReceipt receipt =
      facade.createDocumentObject(request);
  SDL_Log("iggy3d_creative: PLACE dropped objectId=%llu kind='%s' "
          "pos=(%.3f, %.3f, %.3f) bounds=[(%.3f,%.3f,%.3f)..(%.3f,%.3f,%.3f)] "
          "shape='%s' transformOverride=%d boundsOverride=%d pathOverride=%d "
          "pathPointCount=%zu pathPoints='%s' accepted=%d",
          static_cast<unsigned long long>(receipt.objectId),
          std::string(iggy3d::creative::toString(receipt.objectKind)).c_str(),
          request.transform.position.x, request.transform.position.y,
          request.transform.position.z, request.bounds.min.x,
          request.bounds.min.y, request.bounds.min.z, request.bounds.max.x,
          request.bounds.max.y, request.bounds.max.z,
          std::string(iggy3d::creative::toString(descriptor.shapeKind)).c_str(),
          request.hasTransformOverride ? 1 : 0,
          request.hasBoundsOverride ? 1 : 0,
          request.hasPathOverride ? 1 : 0,
          request.pathPoints.size(),
          pathPointsSummary(request.pathPoints).c_str(),
          receipt.accepted ? 1 : 0);
  return receipt;
}

iggy3d::creative::CreativeDocumentCreateReceipt placeBrushObjectWithUndo(
    iggy3d::creative::Facade& facade,
    StandaloneUndoStack& undoStack,
    iggy3d::creative::CreativeObjectKind brush,
    iggy3d::Vec3 cellCenter,
    std::uint64_t ordinal,
    std::string_view source) {
  const std::size_t undoDepthBefore = undoStack.documents.size();
  pushUndoSnapshot(undoStack, facade, source);
  iggy3d::creative::CreativeDocumentCreateReceipt receipt =
      placeBrushObject(facade, brush, cellCenter, ordinal);
  if (!receipt.accepted || !receipt.objectCreated) {
    discardUndoSnapshot(undoStack, undoDepthBefore, source, receipt.reasonCode);
  }
  SDL_Log("iggy3d_creative: UNDO create source='%s' objectId=%llu "
          "accepted=%d created=%d objectCount=%llu depthBefore=%zu "
          "depthAfter=%zu reasonCode='%s'",
          std::string(source).c_str(),
          static_cast<unsigned long long>(receipt.objectId),
          receipt.accepted ? 1 : 0, receipt.objectCreated ? 1 : 0,
          static_cast<unsigned long long>(facade.document().objectCount()),
          undoDepthBefore, undoStack.documents.size(),
          std::string(receipt.reasonCode).c_str());
  return receipt;
}

}  // namespace iggy3d_creative_app
