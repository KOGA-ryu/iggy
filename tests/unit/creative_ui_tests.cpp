#include "app/iggy3d/creative/ui/Ui.hpp"

#include <cstddef>
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

cr::CreativeUiObjectSummary objectSummary(cr::Id target,
                                          cr::CreativeObjectKind kind,
                                          bool visible) {
  cr::CreativeUiObjectSummary summary;
  summary.target.value = target;
  summary.objectKind = kind;
  summary.exists = true;
  summary.visible = visible;
  return summary;
}

const cr::CreativeUiRow* rowOfKind(const cr::CreativeUiModel& model,
                                   cr::CreativeUiRowKind kind) {
  for (const cr::CreativeUiRow& row : model.rows) {
    if (row.kind == kind) {
      return &row;
    }
  }
  return nullptr;
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
         // Inspector (Selection) always present: 1 resting row with no
         // selection (TL-6) => 9 rows by default.
         expect(receipt.rowCount == 9U, "default row count") &&
         expect(model.panels.size() == 7U, "default panels size") &&
         expect(model.rows.size() == 9U, "default rows size") &&
         expect(panel(model, cr::CreativeUiPanelKind::Tools).firstRow == 0U,
                "tools first row") &&
         expect(panel(model, cr::CreativeUiPanelKind::Tools).rowCount == 4U,
                "tools row count") &&
         expect(panel(model, cr::CreativeUiPanelKind::Create).firstRow == 4U,
                "create first row") &&
         expect(panel(model, cr::CreativeUiPanelKind::Create).rowCount == 2U,
                "create row count") &&
         expect(panel(model, cr::CreativeUiPanelKind::Status).firstRow == 6U,
                "status first row") &&
         expect(panel(model, cr::CreativeUiPanelKind::Status).rowCount == 1U,
                "status row count") &&
         expect(panel(model, cr::CreativeUiPanelKind::Selection).firstRow == 7U,
                "selection first row") &&
         expect(panel(model, cr::CreativeUiPanelKind::Selection).rowCount == 1U,
                "selection resting row") &&
         expect(model.rows[7].kind == cr::CreativeUiRowKind::InspectorEmpty,
                "selection resting row kind") &&
         expect(panel(model, cr::CreativeUiPanelKind::Measurement).rowCount == 0U,
                "measurement empty") &&
         expect(panel(model, cr::CreativeUiPanelKind::Ghost).rowCount == 0U,
                "ghost empty") &&
         expect(panel(model, cr::CreativeUiPanelKind::Snap).firstRow == 8U,
                "snap first row") &&
         expect(panel(model, cr::CreativeUiPanelKind::Snap).rowCount == 1U,
                "snap row count") &&
         expect(model.rows[0].kind == cr::CreativeUiRowKind::ToolButton,
                "default select tool row") &&
         expect(model.rows[0].id == "tool_select", "default select id") &&
         expect(model.rows[0].label == "Select", "default select label") &&
         expect(model.rows[0].tool == cr::Tool::Select,
                "default select tool payload") &&
         expect(hasFlag(model.rows[0].flags, cr::kCreativeUiRowFlagActive),
                "default select active") &&
         expect(model.rows[1].id == "tool_move", "default move id") &&
         expect(model.rows[1].label == "Move", "default move label") &&
         expect(model.rows[1].tool == cr::Tool::Move,
                "default move tool payload") &&
         expect(!hasFlag(model.rows[1].flags, cr::kCreativeUiRowFlagActive),
                "default move inactive") &&
         expect(model.rows[2].id == "tool_measure", "default measure id") &&
         expect(model.rows[2].label == "Measure", "default measure label") &&
         expect(model.rows[3].id == "tool_navigate", "default navigate id") &&
         expect(model.rows[3].label == "Navigate",
                "default navigate label") &&
         expect(model.rows[4].kind == cr::CreativeUiRowKind::CreateObject,
                "default create room row") &&
         expect(model.rows[4].id == "create_room",
                "default create room id") &&
         expect(model.rows[4].label == "Create Room",
                "default create room label") &&
         expect(model.rows[4].objectKind == cr::CreativeObjectKind::Room,
                "default create room payload") &&
         expect(model.rows[5].kind == cr::CreativeUiRowKind::CreateObject,
                "default create crate row") &&
         expect(model.rows[5].id == "create_crate",
                "default create crate id") &&
         expect(model.rows[5].label == "Create Crate",
                "default create crate label") &&
         expect(model.rows[5].objectKind == cr::CreativeObjectKind::Crate,
                "default create crate payload") &&
         expect(model.rows[6].kind == cr::CreativeUiRowKind::StatusSummary,
                "default status row") &&
         expect(model.rows[8].kind == cr::CreativeUiRowKind::SnapSettings,
                "default snap row");
}

std::size_t activeToolRowCount(const cr::CreativeUiModel& model) {
  std::size_t count = 0;
  for (const cr::CreativeUiRow& row : model.rows) {
    if (row.kind == cr::CreativeUiRowKind::ToolButton &&
        hasFlag(row.flags, cr::kCreativeUiRowFlagActive)) {
      ++count;
    }
  }
  return count;
}

bool toolPaletteMarksExactlyOneActiveRowPerTool() {
  const cr::Tool tools[] = {cr::Tool::Select,
                            cr::Tool::Move,
                            cr::Tool::Measure,
                            cr::Tool::Navigate};

  bool ok = true;
  for (const cr::Tool tool : tools) {
    cr::CreativeUiBuildRequest request =
        cr::makeDefaultCreativeUiBuildRequest();
    request.toolState.activeTool = tool;
    const cr::CreativeUiBuildReceipt receipt =
        cr::buildCreativeUiModel(request);
    const cr::CreativeUiModel& model = receipt.model;

    ok = expect(activeToolRowCount(model) == 1U,
                "exactly one active tool row") &&
         ok;
    ok = expect(model.activeTool == tool, "summary active tool") && ok;
    for (std::size_t index = 0; index < 4U; ++index) {
      const cr::CreativeUiRow& row = model.rows[index];
      const bool isActive =
          hasFlag(row.flags, cr::kCreativeUiRowFlagActive);
      ok = expect(isActive == (row.tool == tool),
                  "active flag tracks tool state") &&
           ok;
      ok = expect(hasFlag(row.flags, cr::kCreativeUiRowFlagVisible) &&
                      hasFlag(row.flags, cr::kCreativeUiRowFlagEnabled),
                  "tool rows visible and enabled") &&
           ok;
    }
  }
  return ok;
}

bool selectedTargetRowAppearsOnlyWhenNonzero() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  const cr::CreativeUiBuildReceipt empty = cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& emptyRow =
      firstPanelRow(empty.model, cr::CreativeUiPanelKind::Selection);
  request.selectionState.selectedTarget.value = 42;
  const cr::CreativeUiBuildReceipt selected =
      cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& row =
      firstPanelRow(selected.model, cr::CreativeUiPanelKind::Selection);

  // No selection: the inspector is not empty; it holds one resting row.
  // With a selection: the inspector expands to the full display + toggle rows,
  // led by the DISPLAY-ONLY SelectedTarget row.
  return expect(panel(empty.model, cr::CreativeUiPanelKind::Selection).rowCount == 1U,
                "selection resting present") &&
         expect(emptyRow.kind == cr::CreativeUiRowKind::InspectorEmpty,
                "selection resting kind") &&
         expect(emptyRow.target.value == cr::kInvalidId,
                "selection resting no target") &&
         expect(panel(selected.model, cr::CreativeUiPanelKind::Selection).rowCount == 9U,
                "selection inspector present") &&
         expect(row.kind == cr::CreativeUiRowKind::SelectedTarget,
                "selection row kind") &&
         expect(row.target.value == 42U, "selection target") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagHasTarget),
                "selection target flag");
}

bool selectedVisibleObjectSummaryMarksRowVisible() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = 42;
  request.objectSummaries.push_back(
      objectSummary(42, cr::CreativeObjectKind::Room, true));

  const cr::CreativeUiBuildReceipt receipt = cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& row =
      firstPanelRow(receipt.model, cr::CreativeUiPanelKind::Selection);

  return expect(row.target.value == 42U, "selected visible target") &&
         expect(row.objectKind == cr::CreativeObjectKind::Room,
                "selected visible kind") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagObjectKnown),
                "selected visible known") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagObjectVisible),
                "selected visible flag") &&
         expect(receipt.model.objectSummaries.size() == 1U,
                "selected visible summary copied");
}

bool selectedHiddenObjectSummaryKeepsRowAndMarksInvisible() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = 42;
  request.objectSummaries.push_back(
      objectSummary(42, cr::CreativeObjectKind::Room, false));

  const cr::CreativeUiBuildReceipt receipt = cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& row =
      firstPanelRow(receipt.model, cr::CreativeUiPanelKind::Selection);

  return expect(panel(receipt.model,
                      cr::CreativeUiPanelKind::Selection).rowCount == 9U,
                "selected hidden inspector rows") &&
         expect(row.target.value == 42U, "selected hidden target") &&
         expect(row.objectKind == cr::CreativeObjectKind::Room,
                "selected hidden kind") &&
         expect(hasFlag(row.flags, cr::kCreativeUiRowFlagObjectKnown),
                "selected hidden known") &&
         expect(!hasFlag(row.flags, cr::kCreativeUiRowFlagObjectVisible),
                "selected hidden visible false");
}

bool selectedMissingObjectSummaryKeepsRowUnknown() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = 42;

  const cr::CreativeUiBuildReceipt receipt = cr::buildCreativeUiModel(request);
  const cr::CreativeUiRow& row =
      firstPanelRow(receipt.model, cr::CreativeUiPanelKind::Selection);

  return expect(panel(receipt.model,
                      cr::CreativeUiPanelKind::Selection).rowCount == 9U,
                "selected missing inspector rows") &&
         expect(row.target.value == 42U, "selected missing target") &&
         expect(row.objectKind == cr::CreativeObjectKind::Unknown,
                "selected missing kind unknown") &&
         expect(!hasFlag(row.flags, cr::kCreativeUiRowFlagObjectKnown),
                "selected missing known false") &&
         expect(!hasFlag(row.flags, cr::kCreativeUiRowFlagObjectVisible),
                "selected missing visible false");
}

bool inspectorRowsCarrySelectedObjectFacts() {
  cr::CreativeUiBuildRequest request = cr::makeDefaultCreativeUiBuildRequest();
  request.selectionState.selectedTarget.value = 42;
  cr::CreativeUiObjectSummary summary =
      objectSummary(42, cr::CreativeObjectKind::Crate, false);
  summary.locked = true;
  summary.name = "Crate A";
  summary.objectId = 42;
  summary.layerId = 3;
  summary.bounds.min = {1.0, 2.0, 3.0};
  summary.bounds.max = {5.0, 6.0, 7.0};
  summary.position = {1.0, 2.0, 3.0};
  request.objectSummaries.push_back(summary);

  const cr::CreativeUiBuildReceipt receipt = cr::buildCreativeUiModel(request);
  const cr::CreativeUiModel& model = receipt.model;
  const cr::CreativeUiRow* kindRow =
      rowOfKind(model, cr::CreativeUiRowKind::InspectorKind);
  const cr::CreativeUiRow* idRow =
      rowOfKind(model, cr::CreativeUiRowKind::InspectorId);
  const cr::CreativeUiRow* nameRow =
      rowOfKind(model, cr::CreativeUiRowKind::InspectorName);
  const cr::CreativeUiRow* visibleRow =
      rowOfKind(model, cr::CreativeUiRowKind::InspectorVisible);
  const cr::CreativeUiRow* lockedRow =
      rowOfKind(model, cr::CreativeUiRowKind::InspectorLocked);
  const cr::CreativeUiRow* boundsRow =
      rowOfKind(model, cr::CreativeUiRowKind::InspectorBounds);
  const cr::CreativeUiRow* positionRow =
      rowOfKind(model, cr::CreativeUiRowKind::InspectorPosition);
  const cr::CreativeUiRow* layerRow =
      rowOfKind(model, cr::CreativeUiRowKind::InspectorLayer);

  return expect(kindRow != nullptr &&
                    kindRow->objectKind == cr::CreativeObjectKind::Crate,
                "inspector kind row") &&
         expect(idRow != nullptr && idRow->data0 == 42U,
                "inspector id row") &&
         expect(nameRow != nullptr && nameRow->name == "Crate A",
                "inspector name row") &&
         expect(visibleRow != nullptr &&
                    !hasFlag(visibleRow->flags,
                             cr::kCreativeUiRowFlagObjectVisible) &&
                    hasFlag(visibleRow->flags,
                            cr::kCreativeUiRowFlagObjectKnown),
                "inspector visible row") &&
         expect(lockedRow != nullptr &&
                    hasFlag(lockedRow->flags,
                            cr::kCreativeUiRowFlagObjectLocked),
                "inspector locked row") &&
         expect(boundsRow != nullptr && boundsRow->primaryX == 1.0 &&
                    boundsRow->primaryY == 2.0 && boundsRow->primaryZ == 3.0 &&
                    boundsRow->secondaryX == 5.0 &&
                    boundsRow->secondaryY == 6.0 &&
                    boundsRow->secondaryZ == 7.0,
                "inspector bounds row") &&
         expect(positionRow != nullptr && positionRow->primaryX == 1.0 &&
                    positionRow->primaryY == 2.0 &&
                    positionRow->primaryZ == 3.0,
                "inspector position row") &&
         expect(layerRow != nullptr && layerRow->data1 == 3U,
                "inspector layer row");
}

bool inspectorRestingRowHasNoTargetOrObject() {
  const cr::CreativeUiBuildReceipt receipt = buildDefault();
  const cr::CreativeUiRow* resting =
      rowOfKind(receipt.model, cr::CreativeUiRowKind::InspectorEmpty);

  return expect(resting != nullptr, "resting row present") &&
         expect(resting->panel == cr::CreativeUiPanelKind::Selection,
                "resting row in selection panel") &&
         expect(resting->target.value == cr::kInvalidId,
                "resting row no target") &&
         expect(!hasFlag(resting->flags, cr::kCreativeUiRowFlagHasTarget),
                "resting row no target flag") &&
         expect(rowOfKind(receipt.model,
                          cr::CreativeUiRowKind::SelectedTarget) == nullptr,
                "resting has no selected row");
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
                  toolPaletteMarksExactlyOneActiveRowPerTool() &&
                  selectedTargetRowAppearsOnlyWhenNonzero() &&
                  selectedVisibleObjectSummaryMarksRowVisible() &&
                  selectedHiddenObjectSummaryKeepsRowAndMarksInvisible() &&
                  selectedMissingObjectSummaryKeepsRowUnknown() &&
                  inspectorRowsCarrySelectedObjectFacts() &&
                  inspectorRestingRowHasNoTargetOrObject() &&
                  measurementRowsPreserveStateAndPoints() &&
                  ghostRowAppearsOnlyWhenVisible() &&
                  snapSettingsRowReflectsModeAxesStepsAndOrigin() &&
                  panelsReferenceRowsByIndexAndCount() &&
                  repeatedBuildProducesSameRows();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
