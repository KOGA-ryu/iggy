#include "EditorPreviewFrame.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "EditorActionHints.hpp"
#include "EditorFrame.hpp"
#include "EditorCatalog.hpp"
#include "EditorControls.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransform.hpp"
#include "EditorGizmo.hpp"
#include "EditorInteraction.hpp"
#include "EditorPlacement.hpp"
#include "EditorPattern.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorShapePreview.hpp"
#include "EditorState.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"
#include "app/iggy3d/creative/render/CreativeScreenProjection.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "core/math/EulerRotation.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneItem.hpp"
#include "render/debug/DebugHudText.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace creative = iggy3d::creative;
using namespace iggy3d;
namespace {

iggy3d::Vec3 gridDotSizeFor(const iggy3d::ProductMapMakerGridDot& dot,
                            float pitchMeters) {
  const float minorSize = std::clamp(pitchMeters * 0.08F, 0.04F, 0.10F);
  const float majorSize = std::clamp(pitchMeters * 0.14F, 0.07F, 0.16F);
  const float size = dot.major ? majorSize : minorSize;
  return {size, size, size};
}

void appendGridDotsToScene(const iggy3d::ProductMapMakerGridSnapshot& grid,
                           iggy3d::SceneProjectionResult& scene) {
  if (!grid.visible || grid.dots.empty()) {
    return;
  }
  scene.room.meshes.reserve(scene.room.meshes.size() + grid.dots.size());
  std::uint64_t index = 0;
  for (const iggy3d::ProductMapMakerGridDot& dot : grid.dots) {
    // Keep only the ground layer: a small Y-extent still emits a few Y layers
    // (the snap rounds the half-extent out to y=-1,0,1), so filter to planeY.
    if (std::fabs(dot.worldPosition.y - grid.planeY) > grid.pitchMeters * 0.5F) {
      continue;
    }
    iggy3d::SceneRoomMeshItem mesh;
    mesh.id = dot.major ? "creative.grid_major_dot_" : "creative.grid_dot_";
    mesh.id += std::to_string(index);
    mesh.role = "grid";
    mesh.materialId =
        dot.major ? "map_maker_grid_major_dot" : "map_maker_grid_dot";
    mesh.position = dot.worldPosition;
    mesh.size = gridDotSizeFor(dot, grid.pitchMeters);
    scene.room.meshes.push_back(std::move(mesh));
    ++index;
  }
  scene.room.staticMeshCount = scene.room.meshes.size();
  scene.room.loaded = true;
}

[[nodiscard]] RenderLineColor volumeOperationColor(
    cr::CreativeVolumeOperationKind operation) noexcept {
  constexpr std::array<RenderLineColor,
                       static_cast<std::size_t>(
                           cr::CreativeVolumeOperationKind::Count)>
      colors{
          RenderLineColor{0.18F, 0.95F, 0.34F, 1.0F},
          RenderLineColor{0.10F, 0.90F, 0.95F, 1.0F},
          RenderLineColor{1.0F, 0.72F, 0.12F, 1.0F},
          RenderLineColor{1.0F, 0.20F, 0.18F, 1.0F},
          RenderLineColor{0.30F, 0.55F, 1.0F, 1.0F},
      };
  const std::size_t index = static_cast<std::size_t>(operation);
  return index < colors.size() ? colors[index] : colors.front();
}

[[nodiscard]] Mat4 modelMatrix(Vec3 position,
                               Vec3 rotationRadians,
                               Vec3 scale) {
  const Vec3 axisX = rotateEulerXyz({1.0F, 0.0F, 0.0F}, rotationRadians);
  const Vec3 axisY = rotateEulerXyz({0.0F, 1.0F, 0.0F}, rotationRadians);
  const Vec3 axisZ = rotateEulerXyz({0.0F, 0.0F, 1.0F}, rotationRadians);
  Mat4 matrix = identityMat4();
  matrix.m[0] = axisX.x * scale.x;
  matrix.m[4] = axisX.y * scale.x;
  matrix.m[8] = axisX.z * scale.x;
  matrix.m[1] = axisY.x * scale.y;
  matrix.m[5] = axisY.y * scale.y;
  matrix.m[9] = axisY.z * scale.y;
  matrix.m[2] = axisZ.x * scale.z;
  matrix.m[6] = axisZ.y * scale.z;
  matrix.m[10] = axisZ.z * scale.z;
  matrix.m[3] = position.x;
  matrix.m[7] = position.y;
  matrix.m[11] = position.z;
  return matrix;
}

[[nodiscard]] bool previewBoundsTransform(
    const creative::CreativeBounds& bounds,
    float inset,
    Vec3& center,
    Vec3& size) {
  const creative::CreativeBoundsMetrics metrics =
      creative::measureCreativeBounds(bounds);
  const creative::CreativeCoreVec3Conversion coreCenter =
      creative::creativeVec3ToCoreChecked(metrics.center);
  const creative::CreativeCoreVec3Conversion coreSize =
      creative::creativeVec3ToCoreChecked(metrics.size);
  if (!metrics.valid || !coreCenter.converted || !coreSize.converted ||
      !std::isfinite(inset)) {
    return false;
  }
  center = coreCenter.value;
  size = coreSize.value * inset;
  return isFinite(size) && size.x > 0.0F &&
         size.y > 0.0F && size.z > 0.0F;
}

void appendCreativePreview(RenderCreativePreviewFrame& previews,
                           RenderCreativePreviewRole role,
                           const Mat4& clipFromModel,
                           bool includePathWireframe = false) {
  if (previews.itemCount >= previews.items.size()) {
    return;
  }
  previews.items[previews.itemCount++] = {
      role, clipFromModel, includePathWireframe};
}

struct MaterialBrushPreviewPlan {
  bool visible = false;
  bool removing = false;
  bool admitted = false;
  cr::CreativeMaterialBrushStampPlan stamp{};
  std::array<cr::CreativeGridCoord3,
             cr::kCreativeMaterialBrushStampCapacity>
      eligibleCells{};
  std::uint16_t eligibleCellCount = 0U;

  [[nodiscard]] std::span<const cr::CreativeGridCoord3> renderedCells()
      const noexcept {
    return admitted
               ? std::span<const cr::CreativeGridCoord3>{eligibleCells.data(),
                                                         eligibleCellCount}
               : stamp.generatedCells();
  }
};

[[nodiscard]] bool materialBrushCellVisited(
    const CreativeMaterialStrokeState& stroke,
    cr::CreativeGridCoord3 cell) noexcept {
  for (std::size_t index = 0U; index < stroke.visitedCount; ++index) {
    const cr::CreativeGridCoord3 visited = stroke.visited[index].cell;
    if (visited.x == cell.x && visited.y == cell.y && visited.z == cell.z) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] MaterialBrushPreviewPlan materialBrushPreviewPlan(
    const CreativeEditorState& editor,
    const cr::CreativeDocument& document) noexcept {
  MaterialBrushPreviewPlan output;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::MaterialBrush ||
      !editor.interaction.target.grid.valid) {
    return output;
  }
  output.removing = editor.interaction.materialStroke.repeat.active &&
                    editor.interaction.materialStroke.repeat.kind ==
                        CreativeMaterialStrokeKind::Remove;
  if (output.removing && !editor.interaction.target.voxelHit) {
    return output;
  }
  const cr::CreativeGridCoord3 center =
      output.removing ? editor.interaction.target.voxelCell
                      : editor.interaction.target.grid.adjacentCell;
  output.stamp = cr::planCreativeMaterialBrushStamp(
      {editor.toolSettings.materialBrushShape,
       editor.toolSettings.materialBrushSize, center,
       editor.toolSettings.materialBrushAxis});
  output.visible = output.stamp.accepted;
  if (!output.visible) {
    return output;
  }

  const cr::CreativeVoxelField& field = document.voxelField();
  for (cr::CreativeGridCoord3 cell : output.stamp.generatedCells()) {
    const cr::CreativeObjectKind currentMaterial = field.materialAt(cell);
    const bool occupied = currentMaterial != cr::CreativeObjectKind::Unknown;
    const bool allowed =
        output.removing
            ? occupied
            : currentMaterial != held.objectKind &&
                  cr::creativeMaterialBrushMaskAllows(
                      editor.toolSettings.materialBrushMask, occupied);
    if (!allowed || materialBrushCellVisited(
                        editor.interaction.materialStroke, cell)) {
      continue;
    }
    output.eligibleCells[output.eligibleCellCount++] = cell;
  }
  const std::size_t remainingCapacity =
      editor.interaction.materialStroke.visitedCount <=
              editor.interaction.materialStroke.visited.size()
          ? editor.interaction.materialStroke.visited.size() -
                editor.interaction.materialStroke.visitedCount
          : 0U;
  const bool materialValid =
      output.removing || cr::creativeVolumeBrushSupported(held.objectKind);
  output.admitted = materialValid &&
                    !editor.interaction.materialStroke.capacityReached &&
                    output.eligibleCellCount > 0U &&
                    output.eligibleCellCount <= remainingCapacity;
  return output;
}

[[nodiscard]] bool voxelChunkCoordLess(
    cr::CreativeVoxelChunkCoord lhs,
    cr::CreativeVoxelChunkCoord rhs) noexcept {
  if (lhs.z != rhs.z) return lhs.z < rhs.z;
  if (lhs.y != rhs.y) return lhs.y < rhs.y;
  return lhs.x < rhs.x;
}

[[nodiscard]] std::vector<cr::CreativeVoxelCuboid>
refreshVoxelChunkMeshPlans(CreativeEditorSceneCache& cache,
                           const cr::CreativeDocument& document) {
  std::vector<CreativeEditorVoxelChunkMeshCache> next;
  next.reserve(static_cast<std::size_t>(document.voxelField().chunkCount()));
  for (const cr::CreativeVoxelChunk& chunk : document.voxelField().chunks()) {
    const auto found = std::lower_bound(
        cache.voxelChunkMeshes.begin(), cache.voxelChunkMeshes.end(),
        chunk.coord,
        [](const CreativeEditorVoxelChunkMeshCache& entry,
           cr::CreativeVoxelChunkCoord coord) {
          return voxelChunkCoordLess(entry.coord, coord);
        });
    if (found != cache.voxelChunkMeshes.end() &&
        found->coord == chunk.coord && found->revision == chunk.revision) {
      next.push_back(*found);
      continue;
    }

    CreativeEditorVoxelChunkMeshCache rebuilt;
    rebuilt.coord = chunk.coord;
    rebuilt.revision = chunk.revision;
    rebuilt.cuboids = cr::buildCreativeVoxelCuboids(chunk);
    next.push_back(std::move(rebuilt));
    ++cache.voxelChunkMeshBuildCount;
  }
  cache.voxelChunkMeshes = std::move(next);

  std::size_t cuboidCount = 0;
  for (const CreativeEditorVoxelChunkMeshCache& entry :
       cache.voxelChunkMeshes) {
    cuboidCount += entry.cuboids.size();
  }
  std::vector<cr::CreativeVoxelCuboid> cuboids;
  cuboids.reserve(cuboidCount);
  for (const CreativeEditorVoxelChunkMeshCache& entry :
       cache.voxelChunkMeshes) {
    cuboids.insert(cuboids.end(), entry.cuboids.begin(), entry.cuboids.end());
  }
  return cuboids;
}

[[nodiscard]] StandaloneRoomBakePreviewScene
buildStandaloneRoomBakePreviewSceneWithVoxelPlans(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot,
    std::span<const cr::CreativeVoxelCuboid> voxelCuboids,
    bool usePrecomputedVoxelCuboids) {
  iggy3d::creative::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &document;
  bakeRequest.roomId = "iggy3d_creative_preview";
  bakeRequest.sourceName = "apps/iggy3d_creative";
  bakeRequest.sourceSubset = "standalone_preview";
  bakeRequest.usePrecomputedVoxelCuboids = usePrecomputedVoxelCuboids;
  bakeRequest.precomputedVoxelCuboids = voxelCuboids;

  StandaloneRoomBakePreviewScene preview;
  preview.roomBake =
      iggy3d::creative::buildRoomAssetFromCreativeDocument(bakeRequest);

  iggy3d::SessionState emptyRuntimeState;
  preview.scene = iggy3d::buildSceneProjection(emptyRuntimeState,
                                               &preview.roomBake.room);
  appendGridDotsToScene(gridSnapshot, preview.scene);
  preview.standalonePreviewMeshCount = appendStandalonePreviewProxiesToScene(
      document, preview.roomBake.staticMeshSources, preview.scene);
  if (!preview.scene.room.meshes.empty()) {
    preview.scene.room.staticMeshCount = preview.scene.room.meshes.size();
    preview.scene.room.loaded = true;
  }
  return preview;
}

}  // namespace

StandaloneRoomBakePreviewScene buildStandaloneRoomBakePreviewScene(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot) {
  return buildStandaloneRoomBakePreviewSceneWithVoxelPlans(
      document, gridSnapshot, {}, false);
}

bool refreshCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot) {
  if (cache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision()) {
    return false;
  }
  if (cache.documentId != document.id()) {
    cache.voxelChunkMeshes.clear();
  }
  const std::vector<cr::CreativeVoxelCuboid> voxelCuboids =
      refreshVoxelChunkMeshPlans(cache, document);
  cache.preview = buildStandaloneRoomBakePreviewSceneWithVoxelPlans(
      document, gridSnapshot, voxelCuboids, true);
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  ++cache.refreshCount;
  cache.valid = true;
  return true;
}

void invalidateCreativeEditorSceneCache(
    CreativeEditorSceneCache& cache) noexcept {
  cache.valid = false;
  cache.documentId = iggy3d::creative::kInvalidDocumentId;
  cache.documentRevision = 0;
  cache.voxelChunkMeshes.clear();
}

void attachCreativeEditorPlacementPreviews(
    const CreativeEditorState& editor,
    bool captureMode,
    FrameInput& frame,
    const cr::CreativeDocument* document) {
  frame.creativePreview = {};
  const bool modalOpen = editor.catalog.model.open ||
                         editor.catalog.toolWheel.open ||
                         editor.toolOptions.open ||
                         editor.transform.active;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool materialPlacement =
      held.kind == cr::CreativeHeldItemKind::Material;
  const bool materialBrush =
      held.kind == cr::CreativeHeldItemKind::MaterialBrush;
  if (captureMode || modalOpen ||
      (!materialPlacement && !materialBrush) ||
      held.objectKind == cr::CreativeObjectKind::Unknown) {
    return;
  }

  const CreativeBrushPlacementPlan heldPlan =
      planBrushPlacement(held.objectKind, {});
  Vec3 heldCenter{};
  Vec3 heldSize{};
  const cr::CreativeBounds heldBounds =
      creativeBrushHeldPreviewBounds(heldPlan);
  if (!heldPlan.valid ||
      !previewBoundsTransform(heldBounds, 1.0F, heldCenter,
                              heldSize)) {
    return;
  }

  const CreativeEditorPlacementFeedback& feedback =
      editor.interaction.placementFeedback;
  const bool mutationAcceptedThisFrame =
      feedback.frameIndex == editor.frameIndex &&
      feedback.status == CreativeEditorPlacementFeedbackStatus::Placed;
  if (materialPlacement && editor.interaction.target.grid.valid &&
      !mutationAcceptedThisFrame) {
    const CreativeBrushPlacementAdmission admission = admitBrushPlacement(
        held.objectKind, editor.interaction.target.grid,
        editor.toolSettings.placementYaw);
    const CreativeBrushPlacementPlan& targetPlan = admission.plan;
    const cr::CreativeBounds& targetBounds =
        targetPlan.valid ? targetPlan.previewBounds
                         : editor.interaction.target.grid.adjacentCellBounds;
    Vec3 targetCenter{};
    Vec3 targetSize{};
    if (previewBoundsTransform(targetBounds, 1.0F, targetCenter,
                               targetSize)) {
      const bool rejectedThisFrame =
          feedback.frameIndex == editor.frameIndex &&
          feedback.status == CreativeEditorPlacementFeedbackStatus::Rejected;
      const bool duplicate =
          document != nullptr && admission.allowed &&
          creativeBrushPlacementTargetOccupied(*document, targetPlan);
      const bool targetInvalid =
          !admission.allowed || duplicate || rejectedThisFrame ||
          editor.interaction.materialStroke.capacityReached;
      appendCreativePreview(
          frame.creativePreview,
          targetInvalid ? RenderCreativePreviewRole::PlacementInvalid
                        : RenderCreativePreviewRole::PlacementValid,
          frame.camera.clipFromWorld *
              modelMatrix(
                  targetCenter,
                  targetPlan.valid
                      ? cr::creativeVec3ToCoreChecked(
                            targetPlan.transform.rotationEulerRadians)
                            .value
                      : Vec3{},
                  targetSize),
          targetPlan.valid &&
              targetPlan.shapeKind == cr::CreativeObjectShapeKind::Path);
    }
  }

  constexpr float kHeldLongestDimension = 0.32F;
  constexpr float kHeldMinimumAxis = 0.06F;
  const float longest = std::max({heldSize.x, heldSize.y, heldSize.z});
  if (!std::isfinite(longest) || longest <= 0.0F) {
    return;
  }
  const float heldScale = kHeldLongestDimension / longest;
  heldSize = {std::max(kHeldMinimumAxis, heldSize.x * heldScale),
              std::max(kHeldMinimumAxis, heldSize.y * heldScale),
              std::max(kHeldMinimumAxis, heldSize.z * heldScale)};
  constexpr float kDegreesToRadians = 0.01745329251994329577F;
  const Vec3 heldRotation{20.0F * kDegreesToRadians,
                          -35.0F * kDegreesToRadians,
                          8.0F * kDegreesToRadians};
  appendCreativePreview(
      frame.creativePreview, RenderCreativePreviewRole::Held,
      frame.camera.clipFromView *
          modelMatrix({0.42F, -0.32F, -0.82F}, heldRotation, heldSize));
}

void logStandaloneRoomBakeFinal(
    const StandaloneRoomBakePreviewScene& preview) {
  const iggy3d::creative::CreativeRoomBakeReceipt& receipt =
      preview.roomBake.receipt;
  SDL_Log("iggy3d_creative: ROOM_BAKE final status='%s' reasonCode='%s' "
          "accepted=%d objectCount=%llu considered=%llu staticMeshes=%llu "
          "spatialSurfaces=%llu skippedHidden=%llu skippedEditorOnly=%llu "
          "skippedNoBounds=%llu skippedUnsupported=%llu "
          "skippedRoomMetadata=%llu standalonePreviewMeshes=%zu "
          "sceneMeshes=%zu",
          std::string(iggy3d::creative::toString(receipt.status)).c_str(),
          receipt.reasonCode.c_str(), receipt.accepted ? 1 : 0,
          static_cast<unsigned long long>(receipt.objectCount),
          static_cast<unsigned long long>(receipt.consideredObjectCount),
          static_cast<unsigned long long>(receipt.bakedStaticMeshCount),
          static_cast<unsigned long long>(receipt.bakedSpatialSurfaceCount),
          static_cast<unsigned long long>(receipt.skippedHiddenCount),
          static_cast<unsigned long long>(receipt.skippedEditorOnlyCount),
          static_cast<unsigned long long>(receipt.skippedNoBoundsCount),
          static_cast<unsigned long long>(
              receipt.skippedUnsupportedShapeCount),
          static_cast<unsigned long long>(receipt.skippedRoomMetadataCount),
          preview.standalonePreviewMeshCount,
          preview.scene.room.meshes.size());
}

void buildAndAttachCreativeEditorOverlayFrame(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output) {
  cr::CreativeAppState& appState = request.appState;
  const CreativeEditorState& editor = request.editor;
  FrameInput& frame = request.frame;
  const cr::CreativeSpatialProjectionRequest& wireProjReq =
      request.wireProjectionRequest;
  const std::uint32_t drawableWidth = request.drawableWidth;
  const std::uint32_t drawableHeight = request.drawableHeight;
  const float gizmoThickness = request.gizmoThickness;

  attachCreativeEditorPlacementPreviews(
      editor, request.captureMode, frame,
      &request.appState.facade.document());

  const creative::Id selectedId = request.selection.selectedId;
  const creative::CreativeObject* selected = request.selection.selected;
  const bool hasSelection =
      request.selection.hasSelection && !editor.volume.active;
  const auto objectSelected = [&](creative::CreativeObjectId objectId) {
    return std::find(request.selection.selectedObjectIds.begin(),
                     request.selection.selectedObjectIds.end(),
                     objectId) != request.selection.selectedObjectIds.end();
  };
  const std::array<GizmoAxisShaft, 3>& gizmoShafts =
      request.gizmoFrame.shafts;
  const bool selectedIsPathForHandles =
      request.gizmoFrame.selectedIsPathForHandles;

  output.uiRects.clear();
  output.glyphs.clear();
  output.combinedWireLines.clear();
  output.documentWireLineCount = 0;
  output.pointMarkerEdgeCount = 0;
  output.lineMarkerEdgeCount = 0;
  output.pathPointHandleEdgeCount = 0;
  output.ghostEdgeCount = 0;
  output.materialBrushEdgeCount = 0;
  output.volumeEdgeCount = 0;
  output.patternEdgeCount = 0;
  output.transformPreviewEdgeCount = 0;
  output.placementFeedbackEdgeCount = 0;

  // ---- BOUNDS BOX (wireframe) --------------------------------------------
  const creative::CreativeDocumentWireframeSegmentBuildResult segs =
      creative::buildCreativeDocumentWireframeSegments(
          appState.facade.document(), wireProjReq);
  ProductCreativeWireframeDebugLineBuildResult lines =
      buildProductCreativeWireframeDebugLines(segs.segmentList);
  // The renderer turns each line into a world-space tube of `thickness` METRES
  // (creativeDebugLineBox: size = |edge| x thickness x thickness), so keep it
  // thin (a few cm) or a 1 m box fills into a solid blob. Selected edges go
  // bright yellow + a touch fatter for emphasis (the kernel colors by style,
  // not by selection).
  for (ProductCreativeWireframeDebugLine& line : lines.lineList.lines) {
    // Recolor the SELECTED object's edges — matched by id, whatever the kind.
    const bool sel = hasSelection && objectSelected(line.objectId);
    line.thickness = sel ? 0.06F : 0.03F;
    if (sel) {
      line.color = {1.0F, 1.0F, 0.0F, 1.0F};
    }
  }
  CreativeWireframeDebugRenderFrame dbg =
      buildCreativeWireframeDebugRenderFrame(&lines.lineList);

  // ---- GIZMO WIREFRAME ----------------------------------------------------
  // Build ONE combined line vector: the document wireframe lines that draw the
  // yellow selection box (dbg.lines, already converted to render lines) PLUS
  // the 3 axis-aligned gizmo shafts. Point frame.creativeWireframeDebug at THIS
  // vector so the renderer draws both. The vector must outlive submitFrame(),
  // so it lives here in the frame-loop body. When nothing is selected we skip
  // the gizmo and the selection box is empty, so this is just dbg.lines.
  std::vector<RenderCreativeWireframeDebugLine>& combinedWireLines =
      output.combinedWireLines;
  combinedWireLines.reserve(dbg.lines.size() + 48);
  std::size_t& documentWireLineCount = output.documentWireLineCount;
  std::size_t& pointMarkerEdgeCount = output.pointMarkerEdgeCount;
  std::size_t& lineMarkerEdgeCount = output.lineMarkerEdgeCount;
  std::size_t& pathPointHandleEdgeCount = output.pathPointHandleEdgeCount;
  for (const RenderCreativeWireframeDebugLine& line : dbg.lines) {
    const creative::CreativeObject* object =
        line.objectId != creative::kInvalidObjectId
            ? appState.facade.findObject(line.objectId)
            : nullptr;
    if (object != nullptr &&
        creative::describeObject(object->kind).shapeKind ==
            creative::CreativeObjectShapeKind::Line) {
      continue;
    }
    combinedWireLines.push_back(line);
  }
  documentWireLineCount = combinedWireLines.size();
  for (const creative::CreativeObject& obj :
       appState.facade.document().objects()) {
    const creative::CreativeObjectDescriptor& descriptor =
        creative::describeObject(obj.kind);
    if (!obj.visible) {
      continue;
    }
    const bool sel = hasSelection && objectSelected(obj.id);
    if (descriptor.shapeKind != creative::CreativeObjectShapeKind::Point &&
        descriptor.shapeKind != creative::CreativeObjectShapeKind::Line) {
      continue;
    }
    const VisualBounds markerBounds = visualBoundsForObject(obj);
    const std::size_t before = combinedWireLines.size();
    appendStandaloneWireframeBoxEdges(
        combinedWireLines, markerBounds.min, markerBounds.max,
        sel ? RenderLineColor{1.0F, 1.0F, 0.0F, 1.0F}
            : descriptor.shapeKind == creative::CreativeObjectShapeKind::Line
                  ? RenderLineColor{0.86F, 0.68F, 0.28F, 1.0F}
                  : RenderLineColor{0.34F, 0.62F, 0.88F, 1.0F},
        sel ? 0.06F : 0.035F);
    for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
      combinedWireLines[i].objectId = obj.id;
    }
    if (descriptor.shapeKind == creative::CreativeObjectShapeKind::Line) {
      lineMarkerEdgeCount += combinedWireLines.size() - before;
    } else {
      pointMarkerEdgeCount += combinedWireLines.size() - before;
    }
  }
  if (selectedIsPathForHandles) {
    for (const creative::CreativePathPoint& point : selected->pathPoints) {
      const VisualBounds handleBounds = pathPointHandleBounds(point.position);
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines,
          handleBounds.min,
          handleBounds.max,
          RenderLineColor{0.20F, 0.88F, 1.0F, 1.0F},
          0.035F);
      for (std::size_t i = before; i < combinedWireLines.size(); ++i) {
        combinedWireLines[i].objectId =
            static_cast<creative::CreativeObjectId>(selectedId);
      }
      pathPointHandleEdgeCount += combinedWireLines.size() - before;
    }
  }
  if (hasSelection) {
    for (const GizmoAxisShaft& shaft : gizmoShafts) {
      RenderCreativeWireframeDebugLine gizmoLine;
      gizmoLine.start = request.gizmoFrame.center;
      gizmoLine.end = shaft.tip;  // Axis-aligned: only one component differs.
      gizmoLine.color = shaft.color;
      gizmoLine.objectId =
          static_cast<creative::CreativeObjectId>(selectedId);
      gizmoLine.thickness = gizmoThickness;
      combinedWireLines.push_back(gizmoLine);
    }
  }
  const CreativeEditorPlacementFeedback& placementFeedback =
      editor.interaction.placementFeedback;
  if (placementFeedback.status ==
          CreativeEditorPlacementFeedbackStatus::Placed &&
      creativeEditorPlacementFeedbackVisible(placementFeedback,
                                              editor.frameIndex)) {
    const creative::CreativeObject* placedObject =
        appState.facade.findObject(placementFeedback.objectId);
    if (placedObject != nullptr) {
      const VisualBounds placedBounds = visualBoundsForObject(*placedObject);
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines, placedBounds.min, placedBounds.max,
          RenderLineColor{0.25F, 1.0F, 0.35F, 1.0F},
          std::max(0.075F, gizmoThickness * 1.4F));
      for (std::size_t index = before; index < combinedWireLines.size();
           ++index) {
        combinedWireLines[index].objectId = placedObject->id;
      }
      output.placementFeedbackEdgeCount =
          combinedWireLines.size() - before;
    } else if (placementFeedback.voxelPlaced) {
      const cr::CreativeBounds& bounds = placementFeedback.voxelBounds;
      Vec3 center{};
      Vec3 size{};
      if (previewBoundsTransform(bounds, 1.0F, center, size)) {
        const Vec3 minimum =
            cr::creativeVec3ToCoreChecked(bounds.min).value;
        const Vec3 maximum =
            cr::creativeVec3ToCoreChecked(bounds.max).value;
        const std::size_t before = combinedWireLines.size();
        appendStandaloneWireframeBoxEdges(
            combinedWireLines, minimum, maximum,
            RenderLineColor{0.25F, 1.0F, 0.35F, 1.0F},
            std::max(0.075F, gizmoThickness * 1.4F));
        output.placementFeedbackEdgeCount =
            combinedWireLines.size() - before;
      }
    }
  }
  // ---- MATERIAL BRUSH PREVIEW --------------------------------------------
  const MaterialBrushPreviewPlan materialBrushPreview =
      materialBrushPreviewPlan(editor, appState.facade.document());
  if (!editor.transform.active && materialBrushPreview.visible) {
    const RenderLineColor color =
        materialBrushPreview.removing
            ? RenderLineColor{1.0F, 0.2F, 0.16F, 1.0F}
            : materialBrushPreview.admitted
                  ? RenderLineColor{0.22F, 1.0F, 0.34F, 1.0F}
                  : RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F};
    const std::size_t before = combinedWireLines.size();
    appendCreativeMaterialBrushCellOutlines(
        combinedWireLines, materialBrushPreview.renderedCells(),
        appState.facade.document().gridSettings(), color, gizmoThickness);
    output.materialBrushEdgeCount = combinedWireLines.size() - before;
  }

  // ---- VOLUME PREVIEW ----------------------------------------------------
  // The preview is transient editor state, not a document object. Curved
  // outlines stay bounded while the shared planner supplies the exact cell
  // count and admission result used by commit.
  creative::CreativeVolumeSelection volumeSelection;
  creative::CreativeShapeBrushPlanReceipt volumeShapePlan;
  bool volumeSelectionVisible = false;
  bool volumeUsesShapePlan = false;
  if (editor.volume.active && !editor.transform.active) {
    volumeSelection = creativeEditorVolumePreviewSelection(editor.volume);
    if (creative::creativeVolumeSelectionValid(volumeSelection)) {
      volumeSelectionVisible = true;
      volumeUsesShapePlan =
          editor.volume.operation == creative::CreativeVolumeOperationKind::Fill ||
          editor.volume.operation ==
              creative::CreativeVolumeOperationKind::Hollow;
      if (volumeUsesShapePlan) {
        creative::CreativeShapeBrushPlanRequest planRequest;
        planRequest.kind = editor.toolSettings.shapeBrushKind;
        planRequest.axis = editor.toolSettings.shapeBrushAxis;
        planRequest.firstCell = volumeSelection.firstCell;
        planRequest.secondCell = volumeSelection.secondCell;
        planRequest.hollow = editor.volume.operation ==
                             creative::CreativeVolumeOperationKind::Hollow;
        planRequest.maxCandidateCellCount = kCreativeEditorVolumeCellLimit;
        planRequest.maxGeneratedCellCount = kCreativeEditorVolumeCellLimit;
        volumeShapePlan = creative::planCreativeShapeBrush(planRequest);
      }
      const std::size_t before = combinedWireLines.size();
      const RenderLineColor color =
          volumeUsesShapePlan && !volumeShapePlan.accepted
              ? RenderLineColor{1.0F, 0.15F, 0.12F, 1.0F}
              : volumeOperationColor(editor.volume.operation);
      appendCreativeShapeBrushOutline(
          combinedWireLines, volumeSelection,
          volumeUsesShapePlan ? editor.toolSettings.shapeBrushKind
                              : creative::CreativeShapeBrushKind::Box,
          editor.toolSettings.shapeBrushAxis, color, gizmoThickness);
      output.volumeEdgeCount = combinedWireLines.size() - before;
    }
  }
  if (!editor.transform.active) {
    output.patternEdgeCount = appendCreativeEditorArrayPreview(
        appState, editor, gizmoThickness, combinedWireLines);
  }
  output.transformPreviewEdgeCount =
      appendCreativeEditorSelectionTransformPreview(
          editor.transform, gizmoThickness, combinedWireLines);
  std::vector<DebugHudGlyphQuad>& glyphs = output.glyphs;
  appendCreativeEditorInteractionOverlay(
      editor, drawableWidth, drawableHeight, gizmoThickness, output.uiRects,
      glyphs, combinedWireLines);
  appendCreativeEditorTransformOverlay(editor.transform, drawableWidth,
                                       drawableHeight, output.uiRects, glyphs);
  appendCreativeEditorCatalogOverlay(appState, editor, drawableWidth,
                                     drawableHeight, output.uiRects, glyphs);
  appendCreativeEditorToolOptionsOverlay(editor, drawableWidth, drawableHeight,
                                         output.uiRects, glyphs);
  appendCreativeEditorControlsOverlay(editor, drawableWidth, drawableHeight,
                                      output.uiRects, glyphs);

  if (output.volumeEdgeCount > 0U && volumeSelectionVisible) {
    const creative::CreativeGridBounds3 gridBounds =
        creative::creativeVolumeGridBounds(volumeSelection);
    const creative::CreativeBounds bounds =
        creative::creativeVolumeWorldBounds(volumeSelection);
    const creative::CreativeBoundsMetrics metrics =
        creative::measureCreativeBounds(bounds);
    const Vec3 labelPosition = creative::creativeVec3ToCoreChecked(
                                   {metrics.center.x, bounds.max.y,
                                    metrics.center.z})
                                   .value;
    const creative::CreativeScreenPoint screenPoint =
        creative::projectCreativeWorldPointToScreen(
            frame.camera.clipFromWorld, labelPosition, drawableWidth,
            drawableHeight);
    if (screenPoint.valid) {
      const std::uint64_t plannedCellCount =
          volumeUsesShapePlan && volumeShapePlan.accepted
              ? volumeShapePlan.generatedCellCount
              : volumeUsesShapePlan ? 0U
                                    : creative::creativeVolumeCellCount(
                                          volumeSelection);
      const std::string shapeLabel =
          volumeUsesShapePlan
              ? std::string(creative::toString(
                    editor.toolSettings.shapeBrushKind))
              : std::string{"BOX"};
      const std::string axisLabel =
          volumeUsesShapePlan &&
                  editor.toolSettings.shapeBrushKind ==
                      creative::CreativeShapeBrushKind::Cylinder
              ? " " + std::string(creative::toString(
                            editor.toolSettings.shapeBrushAxis))
              : std::string{};
      char labelBuf[128];
      if (editor.volume.lastReceipt.requested) {
        std::snprintf(
            labelBuf, sizeof(labelBuf),
            "%s %s%s %d x %d x %d | %llu cells | %s",
            std::string(creative::toString(editor.volume.operation)).c_str(),
            shapeLabel.c_str(), axisLabel.c_str(),
            gridBounds.max.x - gridBounds.min.x,
            gridBounds.max.y - gridBounds.min.y,
            gridBounds.max.z - gridBounds.min.z,
            static_cast<unsigned long long>(plannedCellCount),
            std::string(
                creative::toString(editor.volume.lastReceipt.status)).c_str());
      } else {
        std::snprintf(
            labelBuf, sizeof(labelBuf),
            "%s %s%s %d x %d x %d | %llu cells",
            std::string(creative::toString(editor.volume.operation)).c_str(),
            shapeLabel.c_str(), axisLabel.c_str(),
            gridBounds.max.x - gridBounds.min.x,
            gridBounds.max.y - gridBounds.min.y,
            gridBounds.max.z - gridBounds.min.z,
            static_cast<unsigned long long>(plannedCellCount));
      }
      const DebugHudLayoutResult labelLayout = layoutDebugHudTextAt(
          labelBuf, static_cast<std::int32_t>(screenPoint.x),
          static_cast<std::int32_t>(screenPoint.y), drawableWidth,
          drawableHeight);
      glyphs.insert(glyphs.end(), labelLayout.quads.begin(),
                    labelLayout.quads.end());
    }
  }

  // ---- DIMENSION LABEL + glyph merge -------------------------------------
  // Merge the inspector-panel glyphs with the dimension-label glyphs into ONE
  // vector so a single .data() pointer stays valid for the whole frame.
  if (hasSelection) {
    const Vec3 center{(request.selection.boxMin.x + request.selection.boxMax.x) * 0.5F,
                      (request.selection.boxMin.y + request.selection.boxMax.y) * 0.5F,
                      (request.selection.boxMin.z + request.selection.boxMax.z) * 0.5F};
    const creative::CreativeScreenPoint screenPoint =
        creative::projectCreativeWorldPointToScreen(
            frame.camera.clipFromWorld, center, drawableWidth,
            drawableHeight);
    if (screenPoint.valid) {
      const float dimW = request.selection.boxMax.x - request.selection.boxMin.x;
      const float dimH = request.selection.boxMax.y - request.selection.boxMin.y;
      const float dimD = request.selection.boxMax.z - request.selection.boxMin.z;
      char labelBuf[64];
      std::snprintf(labelBuf, sizeof(labelBuf), "%.1f x %.1f x %.1f m",
                    static_cast<double>(dimW), static_cast<double>(dimH),
                    static_cast<double>(dimD));
      const DebugHudLayoutResult labelLayout = layoutDebugHudTextAt(
          labelBuf, static_cast<std::int32_t>(screenPoint.x),
          static_cast<std::int32_t>(screenPoint.y), drawableWidth,
          drawableHeight);
      glyphs.insert(glyphs.end(), labelLayout.quads.begin(),
                    labelLayout.quads.end());
    }
  }

  appendCreativeEditorActionHintsOverlay(
      editor, request.inputContext, request.activeControlDevice,
      request.captureMode, drawableWidth, drawableHeight, output.uiRects,
      glyphs);

  // ---- ATTACH overlays to the frame --------------------------------------
  frame.ui.visible = true;
  frame.ui.rects = output.uiRects.data();
  frame.ui.rectCount = output.uiRects.size();
  frame.ui.textGlyphQuads = output.glyphs.data();
  frame.ui.textGlyphQuadCount = output.glyphs.size();
  // The combined vector (selection box + gizmo shafts), NOT dbg.frame.
  RenderCreativeWireframeDebugFrame combinedWireFrame;
  combinedWireFrame.available = true;
  combinedWireFrame.visible = !output.combinedWireLines.empty();
  combinedWireFrame.lines = output.combinedWireLines.data();
  combinedWireFrame.lineCount = output.combinedWireLines.size();
  frame.creativeWireframeDebug = combinedWireFrame;
}

}  // namespace iggy3d_creative_app
