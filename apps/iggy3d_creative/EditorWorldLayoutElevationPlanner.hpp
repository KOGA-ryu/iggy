#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "EditorDesktopCommands.hpp"
#include "EditorWorldLayoutState.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutElevationScreenPoint {
  float x = 0.0F;
  float y = 0.0F;
};

struct CreativeEditorWorldLayoutElevationCanvasTransform {
  CreativeEditorWorldLayoutElevationScreenPoint origin;
  float pixelsPerCell = 28.0F;
};

[[nodiscard]] CreativeEditorWorldLayoutElevationScreenPoint
planCreativeEditorWorldLayoutElevationScreenPoint(
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationPoint point) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutElevationPoint
planCreativeEditorWorldLayoutElevationWorldPoint(
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationScreenPoint point) noexcept;

enum class CreativeEditorWorldLayoutElevationCursor : std::uint8_t {
  Default,
  ResizeHorizontal,
  ResizeVertical,
};

inline constexpr std::size_t
    kCreativeEditorWorldLayoutElevationCommandCapacity = 4U;

struct CreativeEditorWorldLayoutElevationCommandPlan {
  std::array<CreativeDesktopCommand,
             kCreativeEditorWorldLayoutElevationCommandCapacity>
      commands{};
  std::size_t count = 0U;
};

struct CreativeEditorWorldLayoutElevationPointerInput {
  bool hovered = false;
  bool primaryPressed = false;
  bool primaryReleased = false;
  bool primaryDown = false;
  bool secondaryPressed = false;
  bool focusLost = false;
  bool cancelPressed = false;
  CreativeEditorWorldLayoutElevationPoint point;
  double handleToleranceCells = 0.25;
};

struct CreativeEditorWorldLayoutElevationInteractionInput {
  const CreativeEditorWorldLayoutState* state = nullptr;
  const CreativeEditorWorldLayoutElevationProjection* projection = nullptr;
  bool interactionEnabled = false;
  CreativeEditorWorldLayoutElevationPointerInput pointer;
};

struct CreativeEditorWorldLayoutElevationInteractionPlan {
  CreativeEditorWorldLayoutElevationCommandPlan commands;
  CreativeEditorWorldLayoutElevationHandle hoveredHandle;
  CreativeEditorWorldLayoutElevationCursor cursor =
      CreativeEditorWorldLayoutElevationCursor::Default;

  bool selectionChanged = false;
  CreativeEditorWorldLayoutSelection selection;
  std::size_t activeLevelIndex = iggy3d::creative::kInvalidCreativeWorldLayoutIndex;

  bool replaceElevationManipulation = false;
  CreativeEditorWorldLayoutElevationManipulationState elevationManipulation;
};

[[nodiscard]] bool creativeEditorWorldLayoutElevationHandleVisible(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationHandle& handle) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutElevationHandle
findVisibleCreativeEditorWorldLayoutElevationHandle(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    CreativeEditorWorldLayoutElevationPoint point,
    double toleranceCells) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutVerticalConnectorTarget
creativeEditorWorldLayoutElevationConnectorTarget(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationHandle& handle);

[[nodiscard]] CreativeEditorWorldLayoutElevationInteractionPlan
planCreativeEditorWorldLayoutElevationInteraction(
    const CreativeEditorWorldLayoutElevationInteractionInput& input);

[[nodiscard]] bool enqueueCreativeEditorWorldLayoutElevationCommandPlan(
    const CreativeEditorWorldLayoutElevationCommandPlan& plan,
    CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
