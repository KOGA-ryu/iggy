#include "EditorPreviewFrame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorPlacement.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/document/DocumentWireframe.hpp"
#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/window/FramePresenter.hpp"
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

}  // namespace

StandaloneRoomBakePreviewScene buildStandaloneRoomBakePreviewScene(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot) {
  iggy3d::creative::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &document;
  bakeRequest.roomId = "iggy3d_creative_preview";
  bakeRequest.sourceName = "apps/iggy3d_creative";
  bakeRequest.sourceSubset = "standalone_preview";

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
  const Vec3 aimCellCenter = request.aimCellCenter;
  const std::uint32_t drawableWidth = request.drawableWidth;
  const std::uint32_t drawableHeight = request.drawableHeight;
  const float gizmoThickness = request.gizmoThickness;

  const creative::Id selectedId = request.selection.selectedId;
  const creative::CreativeObject* selected = request.selection.selected;
  const bool hasSelection = request.selection.hasSelection;
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

  // ---- INSPECTOR UI (draw list -> menu frame rects + glyphs) -------------
  ProductCreativeUiProjectionRequest uiReq;
  uiReq.creative = &appState;  // model=nullptr -> facade.buildUiModel().
  uiReq.virtualWidth = 1280;
  uiReq.virtualHeight = 720;
  const ProductCreativeUiProjection uiProj =
      buildProductCreativeUiProjection(uiReq);

  ProductVulkanMenuFrameRequest menuReq;
  menuReq.uiDrawList = &uiProj.drawList;
  menuReq.frameIndex = editor.frameIndex;
  menuReq.drawableWidth = drawableWidth;
  menuReq.drawableHeight = drawableHeight;
  ProductVulkanMenuFrame menuFrame =
      buildProductVulkanStarterMenuFrame(menuReq);
  output.uiRects = std::move(menuFrame.rects);

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
    const bool sel =
        hasSelection &&
        line.objectId == static_cast<creative::CreativeObjectId>(selectedId);
    line.thickness = sel ? 0.06F : 0.03F;
    if (sel) {
      line.color = {1.0F, 1.0F, 0.0F, 1.0F};
    }
  }
  ProductCreativeWireframeDebugRenderFrame dbg =
      buildProductCreativeWireframeDebugRenderFrame(&lines.lineList);

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
    const bool sel =
        hasSelection &&
        obj.id == static_cast<creative::CreativeObjectId>(selectedId);
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
  // ---- GHOST PREVIEW ------------------------------------------------------
  // In Place mode, draw a GREEN (0,1,0,1) axis-aligned wireframe box at the
  // aimed cell sized to the current brush footprint (min.y=0..height, XZ
  // centered on the cell) — the placement preview. It rides the SAME combined
  // wireframe vector as the selection box + gizmo, so it needs no new render
  // path. It is NOT a document object (objectId=0); it vanishes on the drop's
  // next frame if the aim moves.
  std::size_t& ghostEdgeCount = output.ghostEdgeCount;
  if (editor.placeMode) {
    const creative::CreativeObjectDescriptor& brushDescriptor =
        creative::describeObject(editor.placeBrush);
    Vec3 ghostMin{};
    Vec3 ghostMax{};
    if (brushDescriptor.shapeKind == creative::CreativeObjectShapeKind::Path) {
      const std::vector<creative::CreativePathPoint> ghostPath =
          initialPathPointsForAnchor(aimCellCenter);
      const std::size_t before = combinedWireLines.size();
      appendPathPolylineLines(combinedWireLines,
                              ghostPath,
                              RenderLineColor{0.0F, 1.0F, 0.0F, 1.0F},
                              gizmoThickness);
      ghostEdgeCount = combinedWireLines.size() - before;
    } else if (brushDescriptor.shapeKind ==
               creative::CreativeObjectShapeKind::Point) {
      const VisualBounds markerBounds = pointMarkerBounds(
          creative::CreativeVec3{aimCellCenter.x, 0.0, aimCellCenter.z});
      ghostMin = markerBounds.min;
      ghostMax = markerBounds.max;
    } else {
      const BrushFootprint fp = brushFootprintForDescriptor(brushDescriptor);
      const VisualBounds authoredGhost{
          {aimCellCenter.x - fp.sizeX * 0.5F, 0.0F,
           aimCellCenter.z - fp.sizeZ * 0.5F},
          {aimCellCenter.x + fp.sizeX * 0.5F, fp.height,
           aimCellCenter.z + fp.sizeZ * 0.5F}};
      const VisualBounds ghostBounds =
          brushDescriptor.shapeKind == creative::CreativeObjectShapeKind::Line
              ? lineProxyBounds(authoredGhost)
              : authoredGhost;
      ghostMin = ghostBounds.min;
      ghostMax = ghostBounds.max;
    }
    if (brushDescriptor.shapeKind != creative::CreativeObjectShapeKind::Path) {
      const std::size_t before = combinedWireLines.size();
      appendStandaloneWireframeBoxEdges(
          combinedWireLines, ghostMin, ghostMax,
          RenderLineColor{0.0F, 1.0F, 0.0F, 1.0F},
          gizmoThickness);
      ghostEdgeCount = combinedWireLines.size() - before;
    }
  }
  std::vector<DebugHudGlyphQuad>& glyphs = output.glyphs;
  glyphs = menuFrame.textGlyphQuads;

  // ---- DIMENSION LABEL + glyph merge -------------------------------------
  // Merge the inspector-panel glyphs with the dimension-label glyphs into ONE
  // vector so a single .data() pointer stays valid for the whole frame.
  if (hasSelection) {
    const Vec3 center{(request.selection.boxMin.x + request.selection.boxMax.x) * 0.5F,
                      (request.selection.boxMin.y + request.selection.boxMax.y) * 0.5F,
                      (request.selection.boxMin.z + request.selection.boxMax.z) * 0.5F};
    const ProjectedPoint3 projected =
        projectPoint(frame.camera.clipFromWorld, center);
    if (std::isfinite(projected.w) && projected.w > 0.0F) {
      const Vec3 ndc = projected.ndc;
      const float px = (ndc.x * 0.5F + 0.5F) * static_cast<float>(drawableWidth);
      const float py = (1.0F - (ndc.y * 0.5F + 0.5F)) *
                       static_cast<float>(drawableHeight);
      const float dimW = request.selection.boxMax.x - request.selection.boxMin.x;
      const float dimH = request.selection.boxMax.y - request.selection.boxMin.y;
      const float dimD = request.selection.boxMax.z - request.selection.boxMin.z;
      char labelBuf[64];
      std::snprintf(labelBuf, sizeof(labelBuf), "%.1f x %.1f x %.1f m",
                    static_cast<double>(dimW), static_cast<double>(dimH),
                    static_cast<double>(dimD));
      const DebugHudLayoutResult labelLayout = layoutDebugHudTextAt(
          labelBuf, static_cast<std::int32_t>(px),
          static_cast<std::int32_t>(py), drawableWidth, drawableHeight);
      glyphs.insert(glyphs.end(), labelLayout.quads.begin(),
                    labelLayout.quads.end());
    }
  }

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
