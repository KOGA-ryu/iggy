#include "app/iggy3d/creative/Ui.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool hasFlag(cr::CreativeUiRowFlagMask flags,
             cr::CreativeUiRowFlagMask flag) {
  return (flags & flag) != 0;
}

cr::CreativeUiBuildReceipt buildDefault() {
  return cr::buildCreativeUiModel(cr::makeDefaultCreativeUiBuildRequest());
}

const cr::CreativeUiPanel& panel(const cr::CreativeUiModel& model,
                                 cr::CreativeUiPanelKind kind) {
  return model.panels[static_cast<std::size_t>(kind)];
}

const cr::CreativeUiRow& firstPanelRow(const cr::CreativeUiModel& model,
                                       cr::CreativeUiPanelKind kind) {
  const cr::CreativeUiPanel& uiPanel = panel(model, kind);
  return model.rows[uiPanel.firstRow];
}

bool defaultModelDeterministic() {
  const cr::CreativeUiBuildReceipt receipt = buildDefault();
  const cr::CreativeUiModel& model = receipt.model;

  return expect(receipt.accepted, "default accepted") &&
         expect(receipt.panelCount == 7U, "default panel count") &&
         expect(receipt.rowCount == 3U, "default row count") &&
         expect(model.panels.size() == 7U, "default panels size") &&
         expect(model.rows.size() == 3U, "default rows size") &&
         expect(panel(model, cr::CreativeUiPanelKind::Tools).firstRow == 0U,
                "tools first row") &&
         expect(panel(model, cr::CreativeUiPanelKind::Tools).rowCount == 1U,
                "tools row count") &&
         expect(panel(model, cr::CreativeUiPanelKind::Status).firstRow == 1U,
                "status first row") &&
         expect(panel(model, cr::CreativeUiPanelKind::Status).rowCount == 1U,
                "status row count") &&
         expect(panel(model, cr::CreativeUiPanelKind::Selection).rowCount == 0U,
                "selection empty") &&
         expect(panel(model, cr::CreativeUiPanelKind::Inspection).rowCount == 0U,
                "inspection empty") &&
         expect(panel(model, cr::CreativeUiPanelKind::Measurement).rowCount == 0U,
                "measurement empty") &&
         expect(panel(model, cr::CreativeUiPanelKind::Ghost).rowCount == 0U,
                "ghost empty") &&
         expect(panel(model, cr::CreativeUiPanelKind::Snap).firstRow == 2U,
                "snap first row") &&
         expect(panel(model, cr::CreativeUiPanelKind::Snap).rowCount == 1U,
                "snap row count") &&
         expect(model.rows[0].kind == cr::CreativeUiRowKind::ActiveTool,
                "default active row") &&
         expect(model.rows[1].kind == cr::CreativeUiRowKind::StatusSummary,
                "default status row") &&
         expect(model.rows[2].kind == cr::CreativeUiRowKind::SnapSettings,
                "default snap row");
}

bool activeToolRowReflectsToolChanges() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.toolState.activeTool = cr::Tool::Inspect;
  const cr::CreativeUiBuildReceipt inspectReceipt =
      cr::buildCreativeUiModel(request);
  request.toolState.activeTool = cr::Tool::Measure;
  const cr::CreativeUiBuildReceipt measureReceipt =
      cr::buildCreativeUiModel(request);

  return expect(inspectReceipt.model.rows[0].tool == cr::Tool::Inspect,
                "inspect active tool") &&
         expect(measureReceipt.model.rows[0].tool == cr::Tool::Measure,
                "measure active tool") &&
         expect(measureReceipt.model.activeTool == cr::Tool::Measure,
                "summary active tool");
}

bool selectedTargetRowAppearsOnlyWhenNonzero() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  const cr::CreativeUiBuildReceipt empty = cr::buildCreativeUiModel(request);
  request.selectionState.selectedTarget.value = 42;
  const cr::CreativeUiBuildReceipt selected =
      cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& row =
      firstPanelRow(selected.model, cr::CreativeUiPanelKind::Selection);

  return expect(panel(empty.model, cr::CreativeUiPanelKind::Selection).rowCount == 0U,
                "selection absent") &&
         expect(panel(selected.model, cr::CreativeUiPanelKind::Selection).rowCount == 1U,
                "selection present") &&
         expect(row.kind == cr::CreativeUiRowKind::SelectedTarget,
                "selection row kind") &&
         expect(row.target.value == 42U, "selection target") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagHasTarget),
                "selection target flag");
}

bool inspectedTargetRowAppearsOnlyWhenNonzero() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  const cr::CreativeUiBuildReceipt empty = cr::buildCreativeUiModel(request);
  request.inspectionState.inspectedTarget.value = 84;
  const cr::CreativeUiBuildReceipt inspected =
      cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& row =
      firstPanelRow(inspected.model, cr::CreativeUiPanelKind::Inspection);

  return expect(panel(empty.model, cr::CreativeUiPanelKind::Inspection).rowCount == 0U,
                "inspection absent") &&
         expect(panel(inspected.model, cr::CreativeUiPanelKind::Inspection).rowCount == 1U,
                "inspection present") &&
         expect(row.kind == cr::CreativeUiRowKind::InspectedTarget,
                "inspection row kind") &&
         expect(row.target.value == 84U, "inspection target") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagHasTarget),
                "inspection target flag");
}

bool measurementRowsPreserveStateAndPoints() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.measurementState.active = true;
  request.measurementState.hasMeasurement = true;
  request.measurementState.startPoint = {1.0, 2.0, cr::TargetRef{7}};
  request.measurementState.currentPoint = {3.0, 4.0, cr::TargetRef{9}};
  request.measurementState.sampleCount = 2;

  const cr::CreativeUiBuildReceipt active = cr::buildCreativeUiModel(request);
  const cr::CreativeUiPanel& uiPanel =
      panel(active.model, cr::CreativeUiPanelKind::Measurement);
  const cr::CreativeUiRow& stateRow = active.model.rows[uiPanel.firstRow];
  const cr::CreativeUiRow& startRow = active.model.rows[uiPanel.firstRow + 1U];
  const cr::CreativeUiRow& currentRow = active.model.rows[uiPanel.firstRow + 2U];

  request.measurementState.active = false;
  const cr::CreativeUiBuildReceipt completed =
      cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& completedState =
      firstPanelRow(completed.model, cr::CreativeUiPanelKind::Measurement);

  return expect(uiPanel.rowCount == 3U, "measurement row count") &&
         expect(stateRow.kind == cr::CreativeUiRowKind::MeasurementState,
                "measurement state row") &&
         expect(hasFlag(stateRow.flags, cr::kCreativeUiRowFlagActive),
                "measurement active flag") &&
         expect(hasFlag(stateRow.flags, cr::kCreativeUiRowFlagHasMeasurement),
                "measurement has flag") &&
         expect(stateRow.data0 == 2U, "measurement sample count") &&
         expect(startRow.kind == cr::CreativeUiRowKind::MeasurementStartPoint,
                "measurement start kind") &&
         expect(startRow.primaryX == 1.0, "measurement start x") &&
         expect(startRow.primaryY == 2.0, "measurement start y") &&
         expect(startRow.target.value == 7U, "measurement start target") &&
         expect(currentRow.kind == cr::CreativeUiRowKind::MeasurementCurrentPoint,
                "measurement current kind") &&
         expect(currentRow.primaryX == 3.0, "measurement current x") &&
         expect(currentRow.primaryY == 4.0, "measurement current y") &&
         expect(currentRow.target.value == 9U, "measurement current target") &&
         expect(hasFlag(completedState.flags, cr::kCreativeUiRowFlagCompleted),
                "measurement completed flag");
}

bool ghostRowAppearsOnlyWhenVisible() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  const cr::CreativeUiBuildReceipt hidden = cr::buildCreativeUiModel(request);
  request.ghostState.visible = true;
  request.ghostState.sourceTool = cr::Tool::Measure;
  request.ghostState.rawPoint = {1.2, 2.7};
  request.ghostState.snappedPoint = {1.0, 3.0};
  request.ghostState.target.value = 42;
  request.ghostState.snapAccepted = true;
  request.ghostState.snapApplied = true;
  request.ghostState.snapChanged = true;
  request.ghostState.updateCount = 3;

  const cr::CreativeUiBuildReceipt visible = cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& row =
      firstPanelRow(visible.model, cr::CreativeUiPanelKind::Ghost);

  return expect(panel(hidden.model, cr::CreativeUiPanelKind::Ghost).rowCount == 0U,
                "ghost absent") &&
         expect(panel(visible.model, cr::CreativeUiPanelKind::Ghost).rowCount == 1U,
                "ghost present") &&
         expect(row.kind == cr::CreativeUiRowKind::GhostPreview,
                "ghost row kind") &&
         expect(row.tool == cr::Tool::Measure, "ghost source tool") &&
         expect(row.primaryX == 1.2, "ghost raw x") &&
         expect(row.primaryY == 2.7, "ghost raw y") &&
         expect(row.secondaryX == 1.0, "ghost snapped x") &&
         expect(row.secondaryY == 3.0, "ghost snapped y") &&
         expect(row.target.value == 42U, "ghost target") &&
         expect(row.data0 == 3U, "ghost update count") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagSnapAccepted),
                "ghost snap accepted") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagSnapApplied),
                "ghost snap applied") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagSnapChanged),
                "ghost snap changed");
}

bool snapSettingsRowReflectsModeAxesStepsAndOrigin() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.snapSettings.mode = cr::CreativeSnapMode::Disabled;
  request.snapSettings.axes = cr::kCreativeSnapAxisX;
  request.snapSettings.stepX = 0.5;
  request.snapSettings.stepY = 2.0;
  request.snapSettings.originX = 10.0;
  request.snapSettings.originY = 20.0;

  const cr::CreativeUiBuildReceipt receipt = cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& row =
      firstPanelRow(receipt.model, cr::CreativeUiPanelKind::Snap);

  return expect(row.kind == cr::CreativeUiRowKind::SnapSettings,
                "snap row kind") &&
         expect(row.data0 == static_cast<std::uint64_t>(cr::CreativeSnapMode::Disabled),
                "snap mode") &&
         expect(row.data1 == cr::kCreativeSnapAxisX, "snap axes") &&
         expect(row.primaryX == 0.5, "snap step x") &&
         expect(row.primaryY == 2.0, "snap step y") &&
         expect(row.secondaryX == 10.0, "snap origin x") &&
         expect(row.secondaryY == 20.0, "snap origin y") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagSettingsValid),
                "snap settings valid");
}

bool panelsReferenceRowsByIndexAndCount() {
  const cr::CreativeUiBuildReceipt receipt = buildDefault();
  const cr::CreativeUiModel& model = receipt.model;
  bool ok = true;

  for (const cr::CreativeUiPanel& uiPanel : model.panels) {
    ok = expect(uiPanel.firstRow + uiPanel.rowCount <= model.rows.size(),
                "panel row range") &&
         ok;
  }

  return ok;
}

bool repeatedBuildProducesSameRows() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.toolState.activeTool = cr::Tool::Measure;
  request.selectionState.selectedTarget.value = 42;
  request.snapSettings.stepX = 0.5;

  const cr::CreativeUiBuildReceipt first = cr::buildCreativeUiModel(request);
  const cr::CreativeUiBuildReceipt second = cr::buildCreativeUiModel(request);

  return expect(first.panelCount == second.panelCount,
                "repeat panel count") &&
         expect(first.rowCount == second.rowCount, "repeat row count") &&
         expect(first.model.rows[0].tool == second.model.rows[0].tool,
                "repeat active tool") &&
         expect(first.model.rows[2].target.value ==
                    second.model.rows[2].target.value,
                "repeat selected target") &&
         expect(first.model.rows.back().primaryX ==
                    second.model.rows.back().primaryX,
                "repeat snap step");
}

}  // namespace

int main() {
  const bool ok = defaultModelDeterministic() &&
                  activeToolRowReflectsToolChanges() &&
                  selectedTargetRowAppearsOnlyWhenNonzero() &&
                  inspectedTargetRowAppearsOnlyWhenNonzero() &&
                  measurementRowsPreserveStateAndPoints() &&
                  ghostRowAppearsOnlyWhenVisible() &&
                  snapSettingsRowReflectsModeAxesStepsAndOrigin() &&
                  panelsReferenceRowsByIndexAndCount() &&
                  repeatedBuildProducesSameRows();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
