#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

enum class CreativeEditorWorldLayoutTool : std::uint8_t {
  Select,
  Room,
  Floor,
  Wall,
  Door,
  Window,
  Count,
};

enum class CreativeEditorWorldLayoutSelectionKind : std::uint8_t {
  None,
  Room,
  Box,
  Wall,
  Opening,
};

struct CreativeEditorWorldLayoutSelection {
  CreativeEditorWorldLayoutSelectionKind kind =
      CreativeEditorWorldLayoutSelectionKind::None;
  std::size_t index = cr::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeEditorWorldLayoutPoint {
  double x = 0.0;
  double z = 0.0;
};

struct CreativeEditorWorldLayoutRoomSettings {
  cr::CreativeWorldLayoutRect footprint;
  std::int32_t baseLayer = 0;
  std::uint16_t wallHeightCells =
      cr::kDefaultCreativeWorldLayoutWallHeightCells;
  double wallThicknessCells =
      cr::kDefaultCreativeWorldLayoutWallThicknessCells;
  std::uint16_t floorThicknessCells = 1U;
};

enum class CreativeEditorWorldLayoutRoomHandle : std::uint8_t {
  None,
  Move,
  North,
  East,
  South,
  West,
  NorthWest,
  NorthEast,
  SouthEast,
  SouthWest,
  Count,
};

struct CreativeEditorWorldLayoutRoomTarget {
  std::size_t roomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutRoomHandle handle =
      CreativeEditorWorldLayoutRoomHandle::None;
};

enum class CreativeEditorWorldLayoutRoomManipulationPhase : std::uint8_t {
  Begin,
  Update,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutRoomManipulationState {
  bool active = false;
  std::uint64_t sourceRevision = 0U;
  CreativeEditorWorldLayoutRoomTarget target;
  CreativeEditorWorldLayoutPoint startPoint;
  cr::CreativeWorldLayoutRect originalFootprint;
  cr::CreativeWorldLayoutRect previewFootprint;
  bool previewValid = false;
  std::string reasonCode =
      "creative_editor_world_layout_room_manipulation_inactive";
};

enum class CreativeEditorWorldLayoutGesturePhase : std::uint8_t {
  Begin,
  Commit,
  Cancel,
  Count,
};

struct CreativeEditorWorldLayoutState {
  cr::CreativeWorldLayout source;
  std::uint64_t revision = 1U;
  std::uint64_t savedRevision = 1U;
  std::uint64_t generatedRevision = 0U;
  std::uint64_t nextStableOrdinal = 1U;

  CreativeEditorWorldLayoutTool tool = CreativeEditorWorldLayoutTool::Select;
  CreativeEditorWorldLayoutSelection selection;
  bool anchorActive = false;
  cr::CreativeTerrainCoord2 anchor{};
  CreativeEditorWorldLayoutRoomManipulationState roomManipulation;

  bool previewVisible = false;
  std::uint64_t previewLayoutRevision = 0U;
  cr::CreativeWorldLayoutPreviewResult preview;
  std::string statusMessage = "layout ready";

  // Canvas-only view state. It is neither document nor layout truth.
  float canvasPixelsPerCell = 28.0F;
  float canvasPanX = 0.0F;
  float canvasPanZ = 0.0F;
};

struct CreativeEditorWorldLayoutEditReceipt {
  bool accepted = false;
  bool changed = false;
  std::string reasonCode = "creative_editor_world_layout_not_requested";
};

struct CreativeEditorWorldLayoutPreviewReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutStatus status =
      cr::CreativeWorldLayoutStatus::NotRequested;
  std::string reasonCode = "creative_editor_world_layout_preview_not_requested";
};

struct CreativeEditorWorldLayoutApplyReceipt {
  bool accepted = false;
  bool changed = false;
  cr::CreativeWorldLayoutApplyReceipt apply;
  std::string reasonCode = "creative_editor_world_layout_apply_not_requested";
};

[[nodiscard]] const char* creativeEditorWorldLayoutToolLabel(
    CreativeEditorWorldLayoutTool tool) noexcept;

void resetCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                    std::string layoutKey = "world_layout");
void installCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                      cr::CreativeWorldLayout layout);
void markCreativeEditorWorldLayoutSaved(
    CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] bool creativeEditorWorldLayoutDirty(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] bool creativeEditorWorldLayoutPreviewActive(
    const CreativeEditorWorldLayoutState& state) noexcept;
[[nodiscard]] const cr::CreativeDocument&
creativeEditorWorldLayoutRenderDocument(
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& liveDocument) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutTool(CreativeEditorWorldLayoutState& state,
                                 CreativeEditorWorldLayoutTool tool);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutPoint(CreativeEditorWorldLayoutState& state,
                                    CreativeEditorWorldLayoutPoint point);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutGesture(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutGesturePhase phase,
    CreativeEditorWorldLayoutPoint point = {});
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutRoomSettings(
    CreativeEditorWorldLayoutState& state, std::size_t roomIndex,
    CreativeEditorWorldLayoutRoomSettings settings);
[[nodiscard]] CreativeEditorWorldLayoutRoomTarget
findCreativeEditorWorldLayoutRoomTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutRoomManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutRoomManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point = {},
    double toleranceCells = 0.25);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
deleteCreativeEditorWorldLayoutSelection(CreativeEditorWorldLayoutState& state);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt
cancelCreativeEditorWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutPreviewReceipt
previewCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 const cr::CreativeDocument& document);
[[nodiscard]] CreativeEditorWorldLayoutApplyReceipt
confirmCreativeEditorWorldLayout(CreativeEditorWorldLayoutState& state,
                                 cr::CreativeAppState& appState);

}  // namespace iggy3d_creative_app
