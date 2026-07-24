#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorWorldLayoutPlanView.hpp"
#include "EditorWorldLayoutState.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutCanvasScreenPoint {
  float x = 0.0F;
  float y = 0.0F;
};

struct CreativeEditorWorldLayoutCanvasTransform {
  CreativeEditorWorldLayoutCanvasScreenPoint origin;
  float pixelsPerCell = 28.0F;
};

[[nodiscard]] CreativeEditorWorldLayoutCanvasScreenPoint
planCreativeEditorWorldLayoutCanvasScreenPoint(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    double x, double z) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutPoint
planCreativeEditorWorldLayoutCanvasWorldPoint(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    CreativeEditorWorldLayoutCanvasScreenPoint screen) noexcept;

enum class CreativeEditorWorldLayoutCanvasHoverTargetKind : std::uint8_t {
  None,
  Opening,
  Wall,
  Object,
  Building,
  VerticalConnectorDirection,
  VerticalConnector,
  Room,
  RoomCorner,
  RoomBoundary,
  Box,
  Roof,
  RoofAperture,
  Source,
  Count,
};

struct CreativeEditorWorldLayoutCanvasTargetSelection {
  CreativeEditorWorldLayoutOpeningTarget opening;
  CreativeEditorWorldLayoutWallTarget wall;
  CreativeEditorWorldLayoutVerticalConnectorTarget verticalConnector;
  CreativeEditorWorldLayoutRoomTarget room;
  CreativeEditorWorldLayoutRoomBoundaryTarget roomBoundary;
  CreativeEditorWorldLayoutRoomCornerTarget roomCorner;
  CreativeEditorWorldLayoutBoxTarget box;
  CreativeEditorWorldLayoutRoofApertureTarget roofAperture;
  CreativeEditorWorldLayoutRoofTarget roof;
  std::size_t objectIndex = cr::kInvalidCreativeWorldLayoutIndex;
  bool building = false;
  CreativeEditorWorldLayoutCanvasHoverTargetKind hoverKind =
      CreativeEditorWorldLayoutCanvasHoverTargetKind::None;
};

[[nodiscard]] CreativeEditorWorldLayoutPlanHitStack
planCreativeEditorWorldLayoutCanvasHitStack(
    const CreativeEditorWorldLayoutPlanViewCache& planView,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeWorldLayout& displaySource,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool enabled);

[[nodiscard]] CreativeEditorWorldLayoutCanvasTargetSelection
planCreativeEditorWorldLayoutCanvasTargets(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeGridSettings grid,
    const CreativeEditorWorldLayoutPlanHit& hit,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool enabled);

[[nodiscard]] CreativeEditorWorldLayoutCanvasHoverTargetKind
resolveCreativeEditorWorldLayoutCanvasHoverTarget(
    const CreativeEditorWorldLayoutCanvasTargetSelection& targets,
    bool sourceHit) noexcept;

using CreativeEditorWorldLayoutCanvasCommandPayload = std::variant<
    std::monostate,
    CreativeDesktopWorldLayoutSourcePayload,
    CreativeDesktopWorldLayoutGesturePayload,
    CreativeDesktopWorldLayoutPointPayload,
    CreativeDesktopWorldLayoutRoomManipulationPayload,
    CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload,
    CreativeDesktopWorldLayoutRoomCornerManipulationPayload,
    CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload,
    CreativeDesktopWorldLayoutBuildingManipulationPayload,
    CreativeDesktopWorldLayoutBoxManipulationPayload,
    CreativeDesktopWorldLayoutWallManipulationPayload,
    CreativeDesktopWorldLayoutOpeningManipulationPayload,
    CreativeDesktopWorldLayoutRoofApertureManipulationPayload,
    CreativeDesktopWorldLayoutRoofManipulationPayload>;

struct CreativeEditorWorldLayoutCanvasPlannedCommand {
  CreativeDesktopCommandId id = CreativeDesktopCommandId::None;
  CreativeEditorWorldLayoutCanvasCommandPayload payload;
};

inline constexpr std::size_t kCreativeEditorWorldLayoutCanvasCommandCapacity =
    12U;

struct CreativeEditorWorldLayoutCanvasCommandPlan {
  std::array<CreativeEditorWorldLayoutCanvasPlannedCommand,
             kCreativeEditorWorldLayoutCanvasCommandCapacity>
      commands{};
  std::size_t count = 0U;
  bool overflowed = false;
};

enum class CreativeEditorWorldLayoutCanvasPressKind : std::uint8_t {
  None,
  SelectSource,
  SelectBuilding,
  BeginRoof,
  BeginBuilding,
  InteractSource,
  BeginRegionSelection,
  BeginGesture,
  ApplyPoint,
  Count,
};

struct CreativeEditorWorldLayoutCanvasPressInput {
  bool pressed = false;
  bool selectionToolActive = false;
  bool additive = false;
  bool toggle = false;
  bool dragToolActive = false;
  CreativeEditorWorldLayoutPoint pointerPoint;
  CreativeEditorWorldLayoutPoint hoveredPoint;
  double toleranceCells = 0.25;
};

struct CreativeEditorWorldLayoutCanvasPressPlan {
  CreativeEditorWorldLayoutCanvasPressKind kind =
      CreativeEditorWorldLayoutCanvasPressKind::None;
  CreativeEditorSelectionComposition composition =
      CreativeEditorSelectionComposition::Replace;
  CreativeEditorWorldLayoutPlanHit hit;
  cr::CreativeWorldLayoutSourceRef source;
  CreativeEditorWorldLayoutPlanRegionSelectionGesture regionSelection;
  CreativeEditorWorldLayoutCanvasCommandPlan commands;
};

[[nodiscard]] CreativeEditorSelectionComposition
creativeEditorWorldLayoutCanvasSelectionComposition(
    bool additive, bool toggle) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutCanvasPressPlan
planCreativeEditorWorldLayoutCanvasPress(
    const CreativeEditorWorldLayoutCanvasPressInput& input,
    const CreativeEditorWorldLayoutCanvasTargetSelection& targets,
    const CreativeEditorWorldLayoutPlanHitStack& hitStack,
    CreativeEditorWorldLayoutSelection selection);

struct CreativeEditorWorldLayoutCanvasSourceInteractionPlan {
  bool requested = false;
  bool accepted = false;
  bool beginObjectManipulation = false;
  CreativeEditorWorldLayoutPlanHit hit;
  CreativeEditorWorldLayoutCanvasCommandPlan commands;
};

[[nodiscard]] CreativeEditorWorldLayoutCanvasSourceInteractionPlan
planCreativeEditorWorldLayoutCanvasSourceInteraction(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutPlanHit& hit,
    CreativeEditorWorldLayoutPoint point, double toleranceCells);

struct CreativeEditorWorldLayoutCanvasManipulationSnapshot {
  bool objectActive = false;
  bool roofActive = false;
  CreativeEditorWorldLayoutRoofTarget roofTarget;
  bool buildingActive = false;
  bool openingActive = false;
  bool wallActive = false;
  bool roomActive = false;
  bool roomBoundaryActive = false;
  bool roomCornerActive = false;
  bool verticalConnectorActive = false;
  bool boxActive = false;
  bool roofApertureActive = false;
  bool anchorActive = false;
  bool terrainPathDraftActive = false;
  bool dragToolActive = false;
};

struct CreativeEditorWorldLayoutCanvasPointerSnapshot {
  bool hovered = false;
  bool focusLost = false;
  bool secondaryPressed = false;
  bool cancelPressed = false;
  bool primaryReleased = false;
  bool primaryDown = false;
  bool itemDeactivated = false;
  bool confirmPressed = false;
  bool primaryDoubleClicked = false;
  CreativeEditorWorldLayoutPoint pointerPoint;
  CreativeEditorWorldLayoutPoint hoveredPoint;
  double toleranceCells = 0.25;
};

[[nodiscard]] CreativeEditorWorldLayoutCanvasCommandPlan
planCreativeEditorWorldLayoutCanvasManipulations(
    const CreativeEditorWorldLayoutCanvasManipulationSnapshot& manipulations,
    const CreativeEditorWorldLayoutCanvasPointerSnapshot& pointer);

void enqueueCreativeEditorWorldLayoutCanvasCommandPlan(
    CreativeDesktopCommandFrame& frame,
    CreativeEditorWorldLayoutCanvasCommandPlan plan);

}  // namespace iggy3d_creative_app
