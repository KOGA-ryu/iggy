#include "app/iggy3d/creative/ui/Ui.hpp"

#include <array>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool hasTarget(TargetRef target) noexcept {
  return target.value != kInvalidId;
}

[[nodiscard]] CreativeUiRowFlagMask enabledVisibleFlags() noexcept {
  return kCreativeUiRowFlagVisible | kCreativeUiRowFlagEnabled;
}

[[nodiscard]] CreativeUiRowFlagMask targetFlag(TargetRef target) noexcept {
  return hasTarget(target) ? kCreativeUiRowFlagHasTarget
                           : kCreativeUiRowFlagNone;
}

[[nodiscard]] const CreativeUiObjectSummary* findObjectSummary(
    const std::vector<CreativeUiObjectSummary>& summaries,
    TargetRef target) noexcept {
  if (!hasTarget(target)) {
    return nullptr;
  }

  for (const CreativeUiObjectSummary& summary : summaries) {
    if (summary.exists && summary.target.value == target.value) {
      return &summary;
    }
  }

  return nullptr;
}

void applyObjectSummary(CreativeUiRow& row,
                        const CreativeUiObjectSummary* summary) noexcept {
  if (summary == nullptr) {
    return;
  }

  row.objectKind = summary->objectKind;
  row.flags |= kCreativeUiRowFlagObjectKnown;
  if (summary->visible) {
    row.flags |= kCreativeUiRowFlagObjectVisible;
  }
  if (summary->locked) {
    row.flags |= kCreativeUiRowFlagObjectLocked;
  }
}

std::size_t beginPanel(CreativeUiModel& model,
                       CreativeUiPanelKind kind,
                       bool visible,
                       bool enabled) {
  const std::size_t firstRow = model.rows.size();
  model.panels.push_back(CreativeUiPanel{kind, firstRow, 0, visible, enabled});
  return model.panels.size() - 1;
}

void finishPanel(CreativeUiModel& model, std::size_t panelIndex) {
  CreativeUiPanel& panel = model.panels[panelIndex];
  panel.rowCount = model.rows.size() - panel.firstRow;
}

void appendRow(CreativeUiModel& model, CreativeUiRow row) {
  model.rows.push_back(row);
}

struct CreativeUiToolRowSpec {
  Tool tool = Tool::Select;
  std::string_view id;
  std::string_view label;
};

inline constexpr std::array<CreativeUiToolRowSpec, 4> kCreativeUiToolRows = {{
    {Tool::Select, "tool_select", "Select"},
    {Tool::Move, "tool_move", "Move"},
    {Tool::Measure, "tool_measure", "Measure"},
    {Tool::Navigate, "tool_navigate", "Navigate"},
}};

struct CreativeUiCreateRowSpec {
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  std::string_view id;
  std::string_view label;
};

inline constexpr std::array<CreativeUiCreateRowSpec, 2> kCreativeUiCreateRows =
    {{
        {CreativeObjectKind::Room, "create_room", "Create Room"},
        {CreativeObjectKind::Crate, "create_crate", "Create Crate"},
    }};

void appendToolsPanel(CreativeUiModel& model,
                      const CreativeUiBuildRequest& request) {
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Tools, true, true);

  for (const CreativeUiToolRowSpec& spec : kCreativeUiToolRows) {
    CreativeUiRow row;
    row.kind = CreativeUiRowKind::ToolButton;
    row.panel = CreativeUiPanelKind::Tools;
    row.id = spec.id;
    row.label = spec.label;
    row.tool = spec.tool;
    row.flags = enabledVisibleFlags();
    if (request.toolState.activeTool == spec.tool) {
      row.flags |= kCreativeUiRowFlagActive;
    }
    appendRow(model, row);
  }

  finishPanel(model, panelIndex);
}

void appendCreatePanel(CreativeUiModel& model) {
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Create, true, true);

  for (const CreativeUiCreateRowSpec& spec : kCreativeUiCreateRows) {
    CreativeUiRow row;
    row.kind = CreativeUiRowKind::CreateObject;
    row.panel = CreativeUiPanelKind::Create;
    row.id = spec.id;
    row.label = spec.label;
    row.objectKind = spec.objectKind;
    row.flags = enabledVisibleFlags();
    appendRow(model, row);
  }

  finishPanel(model, panelIndex);
}

void appendStatusPanel(CreativeUiModel& model,
                       const CreativeUiBuildRequest& request) {
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Status, true, true);

  CreativeUiRow row;
  row.kind = CreativeUiRowKind::StatusSummary;
  row.panel = CreativeUiPanelKind::Status;
  row.id = "creative_status";
  row.label = "Creative Status";
  row.tool = request.toolState.activeTool;
  row.flags = enabledVisibleFlags();
  row.data0 = model.panels.size();
  row.data1 = model.rows.size();
  if (hasTarget(request.selectionState.selectedTarget)) {
    row.flags |= kCreativeUiRowFlagHasTarget;
  }
  if (request.measurementState.active) {
    row.flags |= kCreativeUiRowFlagActive;
  }
  if (request.measurementState.hasMeasurement) {
    row.flags |= kCreativeUiRowFlagHasMeasurement;
  }
  if (request.ghostState.visible) {
    row.flags |= kCreativeUiRowFlagVisible;
  }
  appendRow(model, row);

  finishPanel(model, panelIndex);
}

[[nodiscard]] CreativeUiRow makeInspectorRow(
    CreativeUiRowKind kind,
    std::string_view id,
    std::string_view label,
    TargetRef target) {
  CreativeUiRow row;
  row.kind = kind;
  row.panel = CreativeUiPanelKind::Selection;
  row.id = id;
  row.label = label;
  row.target = target;
  row.flags = enabledVisibleFlags() | targetFlag(target);
  return row;
}

void appendSelectionPanel(CreativeUiModel& model,
                          const CreativeUiBuildRequest& request) {
  // The Selection panel IS the inspector (TD-4). It is always visible: with a
  // selection it shows display facts + the visible/locked toggle rows; with no
  // selection it shows an explicit resting row (TL-6) rather than an empty
  // panel.
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Selection, true, true);

  const TargetRef target = request.selectionState.selectedTarget;
  if (!hasTarget(target)) {
    CreativeUiRow row = makeInspectorRow(CreativeUiRowKind::InspectorEmpty,
                                         "inspector_empty",
                                         "Inspector Empty",
                                         {});
    row.flags = enabledVisibleFlags();
    appendRow(model, row);
    finishPanel(model, panelIndex);
    return;
  }

  const CreativeUiObjectSummary* summary =
      findObjectSummary(request.objectSummaries, target);

  // Selected-target display row (DISPLAY-ONLY now; the command-table entry was
  // removed in TV1-E so clicking it does nothing — TD-4).
  CreativeUiRow selectedRow = makeInspectorRow(CreativeUiRowKind::SelectedTarget,
                                               "selected_target",
                                               "Selected Target",
                                               target);
  applyObjectSummary(selectedRow, summary);
  appendRow(model, selectedRow);

  // Kind (display).
  CreativeUiRow kindRow = makeInspectorRow(CreativeUiRowKind::InspectorKind,
                                           "inspector_kind",
                                           "Inspector Kind",
                                           target);
  applyObjectSummary(kindRow, summary);
  appendRow(model, kindRow);

  // Id (display).
  CreativeUiRow idRow = makeInspectorRow(CreativeUiRowKind::InspectorId,
                                         "inspector_id",
                                         "Inspector Id",
                                         target);
  applyObjectSummary(idRow, summary);
  idRow.data0 = summary != nullptr ? summary->objectId : 0;
  appendRow(model, idRow);

  // Name (display).
  CreativeUiRow nameRow = makeInspectorRow(CreativeUiRowKind::InspectorName,
                                           "inspector_name",
                                           "Inspector Name",
                                           target);
  applyObjectSummary(nameRow, summary);
  nameRow.name = summary != nullptr ? summary->name : std::string_view{};
  appendRow(model, nameRow);

  // Visible (COMMAND -> ToggleSelectedObjectVisibility, TD-4).
  CreativeUiRow visibleRow = makeInspectorRow(CreativeUiRowKind::InspectorVisible,
                                              "inspector_visible",
                                              "Inspector Visible",
                                              target);
  applyObjectSummary(visibleRow, summary);
  appendRow(model, visibleRow);

  // Locked (COMMAND -> ToggleSelectedObjectLocked, TD-4).
  CreativeUiRow lockedRow = makeInspectorRow(CreativeUiRowKind::InspectorLocked,
                                             "inspector_locked",
                                             "Inspector Locked",
                                             target);
  applyObjectSummary(lockedRow, summary);
  appendRow(model, lockedRow);

  // Bounds min/max/size (display, TD-9).
  CreativeUiRow boundsRow = makeInspectorRow(CreativeUiRowKind::InspectorBounds,
                                             "inspector_bounds",
                                             "Inspector Bounds",
                                             target);
  applyObjectSummary(boundsRow, summary);
  if (summary != nullptr) {
    boundsRow.primaryX = summary->bounds.min.x;
    boundsRow.primaryY = summary->bounds.min.y;
    boundsRow.primaryZ = summary->bounds.min.z;
    boundsRow.secondaryX = summary->bounds.max.x;
    boundsRow.secondaryY = summary->bounds.max.y;
    boundsRow.secondaryZ = summary->bounds.max.z;
  }
  appendRow(model, boundsRow);

  // Position (display, TD-9).
  CreativeUiRow positionRow = makeInspectorRow(
      CreativeUiRowKind::InspectorPosition,
      "inspector_position",
      "Inspector Position",
      target);
  applyObjectSummary(positionRow, summary);
  if (summary != nullptr) {
    positionRow.primaryX = summary->position.x;
    positionRow.primaryY = summary->position.y;
    positionRow.primaryZ = summary->position.z;
  }
  appendRow(model, positionRow);

  // Layer (display).
  CreativeUiRow layerRow = makeInspectorRow(CreativeUiRowKind::InspectorLayer,
                                            "inspector_layer",
                                            "Inspector Layer",
                                            target);
  applyObjectSummary(layerRow, summary);
  layerRow.data1 = summary != nullptr ? summary->layerId : 0;
  appendRow(model, layerRow);

  finishPanel(model, panelIndex);
}

void appendMeasurementPanel(CreativeUiModel& model,
                            const CreativeUiBuildRequest& request) {
  const bool visible = request.measurementState.hasMeasurement;
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Measurement, visible, true);

  if (visible) {
    CreativeUiRow stateRow;
    stateRow.kind = CreativeUiRowKind::MeasurementState;
    stateRow.panel = CreativeUiPanelKind::Measurement;
    stateRow.id = "measurement_state";
    stateRow.label = "Measurement State";
    stateRow.flags = enabledVisibleFlags() | kCreativeUiRowFlagHasMeasurement;
    if (request.measurementState.active) {
      stateRow.flags |= kCreativeUiRowFlagActive;
    } else {
      stateRow.flags |= kCreativeUiRowFlagCompleted;
    }
    stateRow.data0 = request.measurementState.sampleCount;
    appendRow(model, stateRow);

    CreativeUiRow startRow;
    startRow.kind = CreativeUiRowKind::MeasurementStartPoint;
    startRow.panel = CreativeUiPanelKind::Measurement;
    startRow.id = "measurement_start";
    startRow.label = "Measurement Start";
    startRow.target = request.measurementState.startPoint.target;
    startRow.primaryX = request.measurementState.startPoint.x;
    startRow.primaryY = request.measurementState.startPoint.y;
    startRow.flags = enabledVisibleFlags() | targetFlag(startRow.target);
    appendRow(model, startRow);

    CreativeUiRow currentRow;
    currentRow.kind = CreativeUiRowKind::MeasurementCurrentPoint;
    currentRow.panel = CreativeUiPanelKind::Measurement;
    currentRow.id = "measurement_current";
    currentRow.label = "Measurement Current";
    currentRow.target = request.measurementState.currentPoint.target;
    currentRow.primaryX = request.measurementState.currentPoint.x;
    currentRow.primaryY = request.measurementState.currentPoint.y;
    currentRow.flags = enabledVisibleFlags() | targetFlag(currentRow.target);
    appendRow(model, currentRow);
  }

  finishPanel(model, panelIndex);
}

void appendGhostPanel(CreativeUiModel& model,
                      const CreativeUiBuildRequest& request) {
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Ghost, request.ghostState.visible, true);

  if (request.ghostState.visible) {
    CreativeUiRow row;
    row.kind = CreativeUiRowKind::GhostPreview;
    row.panel = CreativeUiPanelKind::Ghost;
    row.id = "ghost_preview";
    row.label = "Ghost Preview";
    row.tool = request.ghostState.sourceTool;
    row.target = request.ghostState.target;
    row.primaryX = request.ghostState.rawPoint.x;
    row.primaryY = request.ghostState.rawPoint.y;
    row.secondaryX = request.ghostState.snappedPoint.x;
    row.secondaryY = request.ghostState.snappedPoint.y;
    row.data0 = request.ghostState.updateCount;
    row.flags = enabledVisibleFlags() | targetFlag(row.target);
    if (request.ghostState.snapAccepted) {
      row.flags |= kCreativeUiRowFlagSnapAccepted;
    }
    if (request.ghostState.snapApplied) {
      row.flags |= kCreativeUiRowFlagSnapApplied;
    }
    if (request.ghostState.snapChanged) {
      row.flags |= kCreativeUiRowFlagSnapChanged;
    }
    appendRow(model, row);
  }

  finishPanel(model, panelIndex);
}

void appendSnapPanel(CreativeUiModel& model,
                     const CreativeUiBuildRequest& request) {
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Snap, true, true);

  CreativeUiRow row;
  row.kind = CreativeUiRowKind::SnapSettings;
  row.panel = CreativeUiPanelKind::Snap;
  row.id = "snap_settings";
  row.label = "Snap Settings";
  row.primaryX = request.snapSettings.stepX;
  row.primaryY = request.snapSettings.stepY;
  row.secondaryX = request.snapSettings.originX;
  row.secondaryY = request.snapSettings.originY;
  row.data0 = static_cast<std::uint64_t>(request.snapSettings.mode);
  row.data1 = request.snapSettings.axes;
  row.flags = enabledVisibleFlags();
  if (isValidSnapSettings(request.snapSettings)) {
    row.flags |= kCreativeUiRowFlagSettingsValid;
  }
  appendRow(model, row);

  finishPanel(model, panelIndex);
}

}  // namespace

CreativeUiBuildRequest makeDefaultCreativeUiBuildRequest() noexcept {
  return {};
}

CreativeUiBuildReceipt buildCreativeUiModel(CreativeUiBuildRequest request) {
  CreativeUiBuildReceipt receipt;
  receipt.accepted = true;
  receipt.message = "ui_model_built";

  receipt.model.activeTool = request.toolState.activeTool;
  receipt.model.selectedTarget = request.selectionState.selectedTarget;
  receipt.model.objectSummaries = request.objectSummaries;
  receipt.model.measurementActive = request.measurementState.active;
  receipt.model.hasMeasurement = request.measurementState.hasMeasurement;
  receipt.model.ghostVisible = request.ghostState.visible;

  receipt.model.panels.reserve(7);
  receipt.model.rows.reserve(21);

  appendToolsPanel(receipt.model, request);
  appendCreatePanel(receipt.model);
  appendStatusPanel(receipt.model, request);
  appendSelectionPanel(receipt.model, request);
  appendMeasurementPanel(receipt.model, request);
  appendGhostPanel(receipt.model, request);
  appendSnapPanel(receipt.model, request);

  receipt.panelCount = receipt.model.panels.size();
  receipt.rowCount = receipt.model.rows.size();
  return receipt;
}

}  // namespace iggy3d::creative
