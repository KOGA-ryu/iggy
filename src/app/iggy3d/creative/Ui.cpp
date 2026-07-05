#include "app/iggy3d/creative/Ui.hpp"

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
                        const CreativeUiBuildRequest& request) noexcept {
  const CreativeUiObjectSummary* summary =
      findObjectSummary(request.objectSummaries, row.target);
  if (summary == nullptr) {
    return;
  }

  row.objectKind = summary->objectKind;
  row.flags |= kCreativeUiRowFlagObjectKnown;
  if (summary->visible) {
    row.flags |= kCreativeUiRowFlagObjectVisible;
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

void appendToolsPanel(CreativeUiModel& model,
                      const CreativeUiBuildRequest& request) {
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Tools, true, true);

  CreativeUiRow row;
  row.kind = CreativeUiRowKind::ActiveTool;
  row.panel = CreativeUiPanelKind::Tools;
  row.id = "active_tool";
  row.label = "Active Tool";
  row.tool = request.toolState.activeTool;
  row.flags = enabledVisibleFlags() | kCreativeUiRowFlagActive;
  appendRow(model, row);

  CreativeUiRow createRoomRow;
  createRoomRow.kind = CreativeUiRowKind::CreateRoom;
  createRoomRow.panel = CreativeUiPanelKind::Tools;
  createRoomRow.id = "create_room";
  createRoomRow.label = "Create Room";
  createRoomRow.flags = enabledVisibleFlags();
  appendRow(model, createRoomRow);

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

void appendSelectionPanel(CreativeUiModel& model,
                          const CreativeUiBuildRequest& request) {
  const bool visible = hasTarget(request.selectionState.selectedTarget);
  const std::size_t panelIndex =
      beginPanel(model, CreativeUiPanelKind::Selection, visible, true);

  if (visible) {
    CreativeUiRow row;
    row.kind = CreativeUiRowKind::SelectedTarget;
    row.panel = CreativeUiPanelKind::Selection;
    row.id = "selected_target";
    row.label = "Selected Target";
    row.target = request.selectionState.selectedTarget;
    row.flags = enabledVisibleFlags() | targetFlag(row.target);
    applyObjectSummary(row, request);
    appendRow(model, row);
  }

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

  receipt.model.panels.reserve(6);
  receipt.model.rows.reserve(9);

  appendToolsPanel(receipt.model, request);
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
