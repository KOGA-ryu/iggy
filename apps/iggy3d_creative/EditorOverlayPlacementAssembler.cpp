#include "EditorOverlayAssemblersInternal.hpp"

#include <algorithm>
#include <array>

#include "EditorPlacementFeedback.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorPreviewFrameInternal.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
using namespace iggy3d;

void appendCreativeEditorOverlayAssetCollision(
    const CreativeEditorOverlayPlacementAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  if (request.visualization == nullptr ||
      !request.visualization->targetAvailable ||
      !request.visualization->attemptedTransformAvailable ||
      request.assetCatalog == nullptr) {
    return;
  }
  const std::string_view assetId =
      cr::creativeHotbarAssetId(request.held);
  const StaticMeshAssetCatalogEntry* asset =
      request.assetCatalog->find(assetId);
  if (asset == nullptr ||
      asset->authoringMetadata.collisionMode ==
          StaticMeshCollisionMode::None) {
    return;
  }

  constexpr std::array<std::array<std::uint8_t, 2U>, 12U> kEdges{{
      {{0U, 1U}}, {{1U, 2U}}, {{2U, 3U}}, {{3U, 0U}},
      {{4U, 5U}}, {{5U, 6U}}, {{6U, 7U}}, {{7U, 4U}},
      {{0U, 4U}}, {{1U, 5U}}, {{2U, 6U}}, {{3U, 7U}},
  }};
  const auto appendPart = [&](Vec3 minimum, Vec3 maximum) {
    const cr::CreativeBounds localBounds{
        {minimum.x, minimum.y, minimum.z},
        {maximum.x, maximum.y, maximum.z}};
    const cr::CreativeTransformedBounds transformed =
        cr::resolveCreativeTransformedBounds(
            localBounds, request.visualization->attemptedTransform);
    if (!transformed.valid) {
      return;
    }
    std::array<Vec3, 8U> corners{};
    for (std::size_t index = 0U; index < corners.size(); ++index) {
      const cr::CreativeCoreVec3Conversion converted =
          cr::creativeVec3ToCoreChecked(transformed.corners[index]);
      if (!converted.converted) {
        return;
      }
      corners[index] = converted.value;
    }
    for (const auto& edge : kEdges) {
      RenderCreativeWireframeDebugLine line;
      line.start = corners[edge[0]];
      line.end = corners[edge[1]];
      line.color = {1.0F, 0.62F, 0.12F, 0.92F};
      line.thickness =
          std::max(0.025F, request.gizmoThickness * 0.65F);
      output.combinedWireLines.push_back(line);
      ++output.assetCollisionPreviewEdgeCount;
    }
  };

  if (asset->authoringMetadata.collisionMode ==
      StaticMeshCollisionMode::CompoundBounds) {
    for (const StaticMeshCollisionPart& part : asset->collisionParts) {
      appendPart(part.boundsMin, part.boundsMax);
    }
    return;
  }
  if (asset->authoringMetadata.collisionMode ==
      StaticMeshCollisionMode::Bounds) {
    appendPart(asset->boundsMin, asset->boundsMax);
  }
}

void appendCreativeEditorOverlayPlacementFeedback(
    const CreativeEditorOverlayPlacementAssemblyRequest& request,
    CreativeEditorOverlayFrame& output) {
  std::vector<RenderCreativeWireframeDebugLine>& lines =
      output.combinedWireLines;
  if (request.placementFeedback.status !=
          CreativeEditorPlacementFeedbackStatus::Placed ||
      !creativeEditorPlacementFeedbackVisible(
          request.placementFeedback, request.frameIndex)) {
    return;
  }
  const cr::CreativeObject* placedObject =
      request.document.findObject(request.placementFeedback.objectId);
  if (placedObject != nullptr) {
    const VisualBounds placedBounds =
        visualBoundsForObject(*placedObject);
    const std::size_t before = lines.size();
    appendStandaloneWireframeBoxEdges(
        lines, placedBounds.min, placedBounds.max,
        RenderLineColor{0.25F, 1.0F, 0.35F, 1.0F},
        std::max(0.075F, request.gizmoThickness * 1.4F));
    for (std::size_t index = before; index < lines.size(); ++index) {
      lines[index].objectId = placedObject->id;
    }
    output.placementFeedbackEdgeCount = lines.size() - before;
    return;
  }
  if (!request.placementFeedback.voxelPlaced) {
    return;
  }
  Vec3 center{};
  Vec3 size{};
  if (!creativePreviewBoundsTransform(
          request.placementFeedback.voxelBounds, 1.0F, center, size)) {
    return;
  }
  const Vec3 minimum =
      cr::creativeVec3ToCoreChecked(
          request.placementFeedback.voxelBounds.min).value;
  const Vec3 maximum =
      cr::creativeVec3ToCoreChecked(
          request.placementFeedback.voxelBounds.max).value;
  const std::size_t before = lines.size();
  appendStandaloneWireframeBoxEdges(
      lines, minimum, maximum,
      RenderLineColor{0.25F, 1.0F, 0.35F, 1.0F},
      std::max(0.075F, request.gizmoThickness * 1.4F));
  output.placementFeedbackEdgeCount = lines.size() - before;
}

}  // namespace iggy3d_creative_app
