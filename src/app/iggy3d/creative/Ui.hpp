#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Ghost.hpp"
#include "app/iggy3d/creative/Inspect.hpp"
#include "app/iggy3d/creative/Measure.hpp"
#include "app/iggy3d/creative/Select.hpp"
#include "app/iggy3d/creative/Snap.hpp"
#include "app/iggy3d/creative/Tools.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeUiPanelKind : std::uint8_t {
  Tools,
  Status,
  Selection,
  Inspection,
  Measurement,
  Ghost,
  Snap,
};

enum class CreativeUiRowKind : std::uint8_t {
  ActiveTool,
  StatusSummary,
  SelectedTarget,
  InspectedTarget,
  MeasurementState,
  MeasurementStartPoint,
  MeasurementCurrentPoint,
  GhostPreview,
  SnapSettings,
};

using CreativeUiRowFlagMask = std::uint32_t;

inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagNone = 0;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagVisible = 1u << 0;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagEnabled = 1u << 1;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagActive = 1u << 2;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagHasTarget = 1u << 3;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagHasMeasurement =
    1u << 4;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagCompleted = 1u << 5;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagSnapAccepted =
    1u << 6;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagSnapApplied =
    1u << 7;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagSnapChanged =
    1u << 8;
inline constexpr CreativeUiRowFlagMask kCreativeUiRowFlagSettingsValid =
    1u << 9;

struct CreativeUiRow {
  CreativeUiRowKind kind = CreativeUiRowKind::StatusSummary;
  CreativeUiPanelKind panel = CreativeUiPanelKind::Status;
  std::string_view id;
  std::string_view label;
  Tool tool = Tool::Select;
  TargetRef target;
  double primaryX = 0.0;
  double primaryY = 0.0;
  double secondaryX = 0.0;
  double secondaryY = 0.0;
  double value0 = 0.0;
  double value1 = 0.0;
  std::uint64_t data0 = 0;
  std::uint64_t data1 = 0;
  CreativeUiRowFlagMask flags = kCreativeUiRowFlagNone;
};

struct CreativeUiPanel {
  CreativeUiPanelKind kind = CreativeUiPanelKind::Status;
  std::size_t firstRow = 0;
  std::size_t rowCount = 0;
  bool visible = true;
  bool enabled = true;
};

struct CreativeUiModel {
  std::vector<CreativeUiPanel> panels;
  std::vector<CreativeUiRow> rows;
  Tool activeTool = Tool::Select;
  TargetRef selectedTarget;
  TargetRef inspectedTarget;
  bool measurementActive = false;
  bool hasMeasurement = false;
  bool ghostVisible = false;
};

struct CreativeUiBuildRequest {
  CreativeToolState toolState;
  CreativeSelectionState selectionState;
  CreativeInspectionState inspectionState;
  CreativeMeasurementState measurementState;
  CreativeSnapSettings snapSettings;
  CreativeGhostState ghostState;
};

struct CreativeUiBuildReceipt {
  CreativeUiModel model;
  std::size_t panelCount = 0;
  std::size_t rowCount = 0;
  bool accepted = false;
  std::string_view message = "ui_model_not_built";
};

[[nodiscard]] CreativeUiBuildRequest makeDefaultCreativeUiBuildRequest() noexcept;
[[nodiscard]] CreativeUiBuildReceipt buildCreativeUiModel(
    CreativeUiBuildRequest request);

}  // namespace iggy3d::creative
