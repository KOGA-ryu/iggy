#include "StandaloneBrushPalette.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <string>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/Core.hpp"

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

}  // namespace iggy3d_creative_app
