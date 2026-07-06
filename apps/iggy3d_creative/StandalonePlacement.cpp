#include "StandalonePlacement.hpp"

#include <cstdio>

#include <SDL3/SDL.h>

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "core/math/Snap.hpp"

#include "StandaloneBrushPalette.hpp"

namespace iggy3d_creative_app {

iggy3d::Vec3 snapGroundToCellCenter(double worldX,
                                    double worldZ,
                                    double cellSize) {
  const float cell = static_cast<float>(cellSize);
  const iggy3d::Vec3 snapped = iggy3d::snapVec3ToGrid(
      {static_cast<float>(worldX), 0.0F, static_cast<float>(worldZ)},
      {cell, cell, cell},
      {cell * 0.5F, cell * 0.5F, cell * 0.5F},
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
