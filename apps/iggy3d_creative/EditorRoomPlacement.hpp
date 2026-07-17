#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/recipes/BuildingRecipe.hpp"
#include "app/iggy3d/creative/spatial/PlacementGrid.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorRoomPlacementState {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  iggy3d::creative::CreativeBounds firstCellBounds;
  iggy3d::creative::CreativeGridCoord3 firstCell;
  double floorTopY = 0.0;
  bool active = false;
};

enum class CreativeEditorRoomPlacementStatus : std::uint8_t {
  NotRequested,
  InvalidTarget,
  FirstCornerSet,
  InvalidGeometry,
  InvalidLayout,
  ApplyRejected,
  Applied,
};

struct CreativeEditorRoomPlacementReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeEditorRoomPlacementStatus status =
      CreativeEditorRoomPlacementStatus::NotRequested;
  iggy3d::creative::CreativeWorldLayoutStatus layoutStatus =
      iggy3d::creative::CreativeWorldLayoutStatus::NotRequested;
  iggy3d::creative::CreativeWorldLayoutStatus applyStatus =
      iggy3d::creative::CreativeWorldLayoutStatus::NotRequested;
  std::uint64_t generatedObjectCount = 0U;
  std::string_view reasonCode = "creative_editor_room_not_requested";
};

[[nodiscard]] iggy3d::creative::CreativeRectangularRoomGeometryPlan
creativeEditorRoomPlacementPreview(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor) noexcept;

[[nodiscard]] CreativeEditorRoomPlacementReceipt
advanceCreativeEditorRoomPlacement(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source);

[[nodiscard]] bool cancelCreativeEditorRoomPlacement(
    CreativeEditorState& editor) noexcept;

[[nodiscard]] std::size_t appendCreativeEditorRoomPlacementWireframe(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

}  // namespace iggy3d_creative_app
