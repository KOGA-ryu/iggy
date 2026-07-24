#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

#include "EditorWorldLayoutState.hpp"

#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

inline constexpr std::size_t
    kCreativeEditorWorldLayoutVerticalConnectorHandleCapacity = 6U;

struct CreativeEditorWorldLayoutVerticalConnectorHandle {
  CreativeEditorWorldLayoutVerticalConnectorTarget target;
  CreativeEditorWorldLayoutPoint planPosition;
  iggy3d::Vec3 worldPosition{0.0F, 0.0F, 0.0F};
  iggy3d::Vec3 worldAxis{0.0F, 0.0F, 0.0F};
  bool planeSample = false;
  bool valid = false;
};

struct CreativeEditorWorldLayoutVerticalConnectorHandleFrame {
  bool accepted = false;
  std::array<CreativeEditorWorldLayoutVerticalConnectorHandle,
             kCreativeEditorWorldLayoutVerticalConnectorHandleCapacity>
      handles{};
  std::size_t handleCount = 0U;
  cr::CreativeWorldLayoutVerticalConnectorPlan connector;
  std::string_view reasonCode =
      "creative_editor_world_layout_vertical_connector_handles_not_requested";
};

struct CreativeEditorWorldLayoutVerticalConnectorHandlePick {
  bool hit = false;
  std::size_t handleIndex =
      kCreativeEditorWorldLayoutVerticalConnectorHandleCapacity;
  float distanceSquaredPixels = 0.0F;
};

struct CreativeEditorWorldLayoutVerticalConnectorLiveEditReceipt {
  bool accepted = false;
  bool changed = false;
  bool worldLayoutChanged = false;
  bool sceneChanged = false;
  std::string reasonCode =
      "creative_editor_world_layout_vertical_connector_live_edit_not_requested";
};

[[nodiscard]] CreativeEditorWorldLayoutVerticalConnectorHandleFrame
buildCreativeEditorWorldLayoutVerticalConnectorHandleFrame(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    std::size_t connectorIndex = cr::kInvalidCreativeWorldLayoutIndex,
    bool includeManipulationPreview = true);

[[nodiscard]] const CreativeEditorWorldLayoutVerticalConnectorHandle*
findCreativeEditorWorldLayoutVerticalConnectorHandle(
    const CreativeEditorWorldLayoutVerticalConnectorHandleFrame& frame,
    CreativeEditorWorldLayoutVerticalConnectorTarget target) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutVerticalConnectorHandlePick
pickCreativeEditorWorldLayoutVerticalConnectorHandleAtPixel(
    const CreativeEditorWorldLayoutVerticalConnectorHandleFrame& frame,
    const iggy3d::RenderCameraFrame& camera,
    iggy3d::RenderContentViewport viewport,
    float pixelX,
    float pixelY,
    float tolerancePixels = 14.0F) noexcept;

[[nodiscard]] bool
sampleCreativeEditorWorldLayoutVerticalConnectorHandlePoint(
    const CreativeEditorWorldLayoutVerticalConnectorHandle& handle,
    iggy3d::Vec3 rayOrigin,
    iggy3d::Vec3 rayDirection,
    cr::CreativeGridSettings grid,
    CreativeEditorWorldLayoutPoint& output) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutVerticalConnectorLiveEditReceipt
applyCreativeEditorWorldLayoutVerticalConnectorManipulationToDocument(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeAppState& appState,
    CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    CreativeEditorWorldLayoutVerticalConnectorTarget target = {},
    double toleranceCells = 0.25);

}  // namespace iggy3d_creative_app
