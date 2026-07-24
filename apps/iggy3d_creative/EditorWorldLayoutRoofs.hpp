#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "EditorWorldLayoutContracts.hpp"

#include "app/iggy3d/creative/world/WorldLayoutRoofs.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

inline constexpr double kCreativeEditorWorldLayoutRoofHandleSnapCells = 0.25;
inline constexpr std::size_t kCreativeEditorWorldLayoutRoofHandleCapacity = 5U;

struct CreativeEditorWorldLayoutRoofHandle {
  CreativeEditorWorldLayoutRoofTarget target;
  CreativeEditorWorldLayoutPoint planPosition;
  iggy3d::Vec3 worldPosition{0.0F, 0.0F, 0.0F};
  iggy3d::Vec3 worldAxis{0.0F, 0.0F, 0.0F};
  bool valid = false;
};

struct CreativeEditorWorldLayoutRoofHandleFrame {
  bool accepted = false;
  std::array<CreativeEditorWorldLayoutRoofHandle,
             kCreativeEditorWorldLayoutRoofHandleCapacity>
      handles{};
  std::size_t handleCount = 0U;
  cr::CreativeWorldLayoutRoofPlan roof;
  std::string_view reasonCode =
      "creative_editor_world_layout_roof_handles_not_requested";
};

struct CreativeEditorWorldLayoutRoofHandlePick {
  bool hit = false;
  std::size_t handleIndex = kCreativeEditorWorldLayoutRoofHandleCapacity;
  float distanceSquaredPixels = 0.0F;
};

struct CreativeEditorWorldLayoutRoofEditPlan {
  bool accepted = false;
  CreativeEditorWorldLayoutRoofTarget target;
  double snappedDeltaCells = 0.0;
  CreativeEditorWorldLayoutLevelSettings settings;
  cr::CreativeWorldLayoutRoofPlan roof;
  std::string_view reasonCode =
      "creative_editor_world_layout_roof_edit_not_requested";
};

struct CreativeEditorWorldLayoutRoofLiveEditReceipt {
  bool accepted = false;
  bool changed = false;
  bool worldLayoutChanged = false;
  bool sceneChanged = false;
  std::string reasonCode =
      "creative_editor_world_layout_roof_live_edit_not_requested";
};

[[nodiscard]] CreativeEditorWorldLayoutRoofHandleFrame
buildCreativeEditorWorldLayoutRoofHandleFrame(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    std::size_t levelIndex = cr::kInvalidCreativeWorldLayoutIndex,
    bool includeManipulationPreview = true);

[[nodiscard]] CreativeEditorWorldLayoutRoofTarget
findCreativeEditorWorldLayoutPlanRoofHandle(
    const CreativeEditorWorldLayoutRoofHandleFrame& frame,
    CreativeEditorWorldLayoutPoint point,
    double toleranceCells) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutRoofHandlePick
pickCreativeEditorWorldLayoutRoofHandleAtPixel(
    const CreativeEditorWorldLayoutRoofHandleFrame& frame,
    const iggy3d::RenderCameraFrame& camera,
    iggy3d::RenderContentViewport viewport,
    float pixelX,
    float pixelY,
    float tolerancePixels = 14.0F) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutRoofEditPlan
planCreativeEditorWorldLayoutRoofEdit(
    const cr::CreativeWorldLayout& layout,
    cr::CreativeGridSettings grid,
    CreativeEditorWorldLayoutRoofTarget target,
    double deltaCells);

[[nodiscard]] CreativeEditorWorldLayoutRoofEditPlan
planCreativeEditorWorldLayoutRoofEdit(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    CreativeEditorWorldLayoutRoofTarget target,
    double deltaCells);

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoofManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoofManipulationPhase phase,
    CreativeEditorWorldLayoutRoofTarget target = {},
    double coordinateCells = 0.0,
    cr::CreativeGridSettings grid = {});

[[nodiscard]] CreativeEditorWorldLayoutRoofLiveEditReceipt
applyCreativeEditorWorldLayoutRoofManipulationToDocument(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    CreativeEditorWorldLayoutRoofManipulationPhase phase,
    CreativeEditorWorldLayoutRoofTarget target = {},
    double coordinateCells = 0.0);

}  // namespace iggy3d_creative_app
