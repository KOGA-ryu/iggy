#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/map_maker/Grid.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorState;
struct CreativeEditorSelectionFrame;
struct CreativeEditorGizmoFrame;

struct CreativeEditorOverlayFrameRequest {
  iggy3d::creative::CreativeAppState& appState;
  const CreativeEditorState& editor;
  const CreativeEditorSelectionFrame& selection;
  const CreativeEditorGizmoFrame& gizmoFrame;
  iggy3d::FrameInput& frame;
  const iggy3d::creative::CreativeSpatialProjectionRequest&
      wireProjectionRequest;
  iggy3d::Vec3 aimCellCenter;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  float gizmoThickness = 0.0F;
};

struct CreativeEditorOverlayFrame {
  std::vector<iggy3d::RenderUiRect> uiRects;
  std::vector<iggy3d::DebugHudGlyphQuad> glyphs;
  std::vector<iggy3d::RenderCreativeWireframeDebugLine> combinedWireLines;
  std::size_t documentWireLineCount = 0;
  std::size_t pointMarkerEdgeCount = 0;
  std::size_t lineMarkerEdgeCount = 0;
  std::size_t pathPointHandleEdgeCount = 0;
  std::size_t ghostEdgeCount = 0;
};

void buildAndAttachCreativeEditorOverlayFrame(
    const CreativeEditorOverlayFrameRequest& request,
    CreativeEditorOverlayFrame& output);

struct StandaloneRoomBakePreviewScene {
  iggy3d::SceneProjectionResult scene;
  iggy3d::creative::CreativeRoomBakeResult roomBake;
  std::size_t standalonePreviewMeshCount = 0;
};

[[nodiscard]] StandaloneRoomBakePreviewScene buildStandaloneRoomBakePreviewScene(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::ProductMapMakerGridSnapshot& gridSnapshot);

void logStandaloneRoomBakeFinal(
    const StandaloneRoomBakePreviewScene& preview);

}  // namespace iggy3d_creative_app
