#include "app/iggy3d/creative/Facade.hpp"

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

cr::CreativeToolInputPacket pointerInput(cr::CreativeToolInputKind kind,
                                         double x,
                                         double y,
                                         cr::Id targetId = 0) {
  cr::CreativeToolInputPacket input;
  input.kind = kind;
  input.pointer.x = x;
  input.pointer.y = y;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = targetId;
  return input;
}

bool defaultsBuildDefaultUiModel() {
  const cr::Facade facade;
  const cr::CreativeUiBuildReceipt ui = facade.buildUiModel();

  return expect(facade.state().tool == cr::Tool::Select,
                "default state tool") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "default tool state") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "default selection") &&
         expect(facade.inspectionState().inspectedTarget.value == cr::kInvalidId,
                "default inspection") &&
         expect(!facade.measurementState().active,
                "default measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "default measurement empty") &&
         expect(!facade.ghostState().visible, "default ghost hidden") &&
         expect(ui.accepted, "default ui accepted") &&
         expect(ui.panelCount == 7U, "default ui panels") &&
         expect(ui.rowCount == 4U, "default ui rows");
}

bool setActiveToolUpdatesKernelAndOldState() {
  cr::Facade facade;
  const bool changed = facade.setActiveTool(cr::Tool::Inspect);
  const bool same = facade.setActiveTool(cr::Tool::Inspect);

  return expect(changed, "set active changed") &&
         expect(!same, "set active same no change") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "tool state inspect") &&
         expect(facade.state().tool == cr::Tool::Inspect,
                "old state inspect");
}

bool selectPressUpdatesSelectionOnlyAndNotDocument() {
  cr::Facade facade;
  const std::size_t initialObjects = facade.document().objectCount();
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerPress,
                       10.0,
                       20.0,
                       42));

  return expect(receipt.accepted, "select dispatch accepted") &&
         expect(receipt.emittedIntentCount == 1U, "select emitted count") &&
         expect(receipt.selectionChanged, "select changed selection") &&
         expect(!receipt.inspectionChanged, "select inspection unchanged") &&
         expect(!receipt.measurementChanged, "select measurement unchanged") &&
         expect(facade.selectionState().selectedTarget.value == 42U,
                "select state target") &&
         expect(facade.state().selected.value == 42U,
                "old state selected") &&
         expect(facade.state().hovered.value == 42U,
                "old state hovered") &&
         expect(facade.document().objectCount() == initialObjects,
                "select did not mutate document");
}

bool inspectPressUpdatesInspectionOnly() {
  cr::Facade facade;
  const bool toolChanged = facade.setActiveTool(cr::Tool::Inspect);
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerPress,
                       4.0,
                       5.0,
                       77));

  return expect(toolChanged, "inspect tool changed") &&
         expect(receipt.accepted, "inspect dispatch accepted") &&
         expect(receipt.inspectionChanged, "inspect changed inspection") &&
         expect(!receipt.selectionChanged, "inspect selection unchanged") &&
         expect(facade.inspectionState().inspectedTarget.value == 77U,
                "inspection target") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "selection still empty") &&
         expect(facade.document().objectCount() == 0U,
                "inspect did not mutate document");
}

bool measurePressMoveReleaseUpdatesMeasurement() {
  cr::Facade facade;
  const bool toolChanged = facade.setActiveTool(cr::Tool::Measure);
  const cr::CreativeFacadeToolDispatchReceipt begin =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerPress,
                       1.0,
                       2.0,
                       7));
  const cr::CreativeFacadeToolDispatchReceipt update =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerMove,
                       3.0,
                       4.0,
                       8));
  const cr::CreativeFacadeToolDispatchReceipt end =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerRelease,
                       5.0,
                       6.0,
                       9));

  return expect(toolChanged, "measure tool changed") &&
         expect(begin.measurementChanged, "measure begin changed") &&
         expect(update.measurementChanged, "measure update changed") &&
         expect(end.measurementChanged, "measure end changed") &&
         expect(!facade.measurementState().active,
                "measurement inactive after end") &&
         expect(facade.measurementState().hasMeasurement,
                "measurement retained after end") &&
         expect(facade.measurementState().startPoint.x == 1.0,
                "measurement start x") &&
         expect(facade.measurementState().currentPoint.x == 5.0,
                "measurement current x") &&
         expect(facade.measurementState().currentPoint.target.value == 9U,
                "measurement current target") &&
         expect(facade.document().objectCount() == 0U,
                "measure did not mutate document");
}

bool leavingMeasureCancelsActiveMeasurement() {
  cr::Facade facade;
  const bool measureToolChanged = facade.setActiveTool(cr::Tool::Measure);
  const cr::CreativeFacadeToolDispatchReceipt begin =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerPress,
                       1.0,
                       2.0,
                       7));

  const bool selectToolChanged = facade.setActiveTool(cr::Tool::Select);

  return expect(measureToolChanged, "leave measure setup tool changed") &&
         expect(begin.measurementChanged, "leave measure setup began") &&
         expect(selectToolChanged, "leave measure changed to select") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "leave measure active tool select") &&
         expect(facade.state().tool == cr::Tool::Select,
                "leave measure old state select") &&
         expect(!facade.measurementState().active,
                "leave measure measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "leave measure measurement cleared") &&
         expect(facade.measurementState().sampleCount == 0U,
                "leave measure samples cleared");
}

bool sameMeasureToolActivationKeepsActiveMeasurement() {
  cr::Facade facade;
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, 1.0, 2.0, 7)));

  const bool changed = facade.setActiveTool(cr::Tool::Measure);

  return expect(!changed, "same measure no change") &&
         expect(facade.toolState().activeTool == cr::Tool::Measure,
                "same measure active tool") &&
         expect(facade.state().tool == cr::Tool::Measure,
                "same measure old state") &&
         expect(facade.measurementState().active,
                "same measure remains active") &&
         expect(facade.measurementState().hasMeasurement,
                "same measure retains measurement") &&
         expect(facade.measurementState().startPoint.x == 1.0,
                "same measure start preserved");
}

bool leavingMeasurePreservesCompletedMeasurement() {
  cr::Facade facade;
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, 1.0, 2.0, 7)));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerRelease, 5.0, 6.0, 9)));

  const bool changed = facade.setActiveTool(cr::Tool::Select);

  return expect(changed, "completed measure leave changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "completed measure active select") &&
         expect(facade.state().tool == cr::Tool::Select,
                "completed measure old state select") &&
         expect(!facade.measurementState().active,
                "completed measure remains inactive") &&
         expect(facade.measurementState().hasMeasurement,
                "completed measure retained") &&
         expect(facade.measurementState().startPoint.x == 1.0,
                "completed measure start retained") &&
         expect(facade.measurementState().currentPoint.x == 5.0,
                "completed measure current retained") &&
         expect(facade.measurementState().currentPoint.target.value == 9U,
                "completed measure target retained");
}

bool nonMeasureToolSwitchDoesNotTouchMeasurement() {
  cr::Facade facade;

  const bool changed = facade.setActiveTool(cr::Tool::Inspect);

  return expect(changed, "non-measure switch changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "non-measure active inspect") &&
         expect(facade.state().tool == cr::Tool::Inspect,
                "non-measure old state inspect") &&
         expect(!facade.measurementState().active,
                "non-measure measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "non-measure measurement empty") &&
         expect(facade.measurementState().sampleCount == 0U,
                "non-measure samples unchanged");
}

bool pointerMoveUpdatesGhostWithSnap() {
  cr::Facade facade;
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerMove,
                       1.2,
                       2.7,
                       42));

  return expect(receipt.accepted, "ghost dispatch accepted") &&
         expect(receipt.ghostChanged, "ghost changed") &&
         expect(facade.ghostState().visible, "ghost visible") &&
         expect(facade.ghostState().rawPoint.x == 1.2, "ghost raw x") &&
         expect(facade.ghostState().rawPoint.y == 2.7, "ghost raw y") &&
         expect(facade.ghostState().snappedPoint.x == 1.0,
                "ghost snapped x") &&
         expect(facade.ghostState().snappedPoint.y == 3.0,
                "ghost snapped y") &&
         expect(facade.ghostState().target.value == 42U, "ghost target");
}

bool snapSettingsAffectLaterGhostDispatch() {
  cr::Facade facade;
  cr::CreativeSnapSettings snap = cr::makeDefaultCreativeSnapSettings();
  snap.stepX = 0.5;
  snap.stepY = 0.5;
  facade.setSnapSettings(snap);

  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerMove,
                       1.2,
                       2.7,
                       42));

  return expect(receipt.ghostChanged, "snap ghost changed") &&
         expect(facade.snapSettings().stepX == 0.5, "snap setting stored x") &&
         expect(facade.snapSettings().stepY == 0.5, "snap setting stored y") &&
         expect(facade.ghostState().snappedPoint.x == 1.0,
                "snap ghost x") &&
         expect(facade.ghostState().snappedPoint.y == 2.5,
                "snap ghost y");
}

bool unknownInputDoesNotChangeKernels() {
  cr::Facade facade;
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput({});

  return expect(!receipt.accepted, "unknown not accepted") &&
         expect(!receipt.changed, "unknown unchanged") &&
         expect(!receipt.selectionChanged, "unknown selection unchanged") &&
         expect(!receipt.inspectionChanged, "unknown inspection unchanged") &&
         expect(!receipt.measurementChanged, "unknown measurement unchanged") &&
         expect(!receipt.ghostChanged, "unknown ghost unchanged") &&
         expect(receipt.message == "unsupported_input", "unknown message") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "unknown selection state") &&
         expect(facade.inspectionState().inspectedTarget.value == cr::kInvalidId,
                "unknown inspection state") &&
         expect(!facade.measurementState().hasMeasurement,
                "unknown measurement state") &&
         expect(!facade.ghostState().visible, "unknown ghost state");
}

bool roomCommandsStillWorkThroughFacade() {
  cr::Facade facade;
  const cr::CreativeObjectId id = facade.createRoom("Facade Room");
  const bool renamed = facade.renameObject(id, "Facade Room Renamed");
  const bool removed = facade.removeObject(id);

  return expect(id != cr::kInvalidObjectId, "facade room id") &&
         expect(renamed, "facade room renamed") &&
         expect(removed, "facade room removed") &&
         expect(facade.document().objectCount() == 0U,
                "facade room document empty") &&
         expect(facade.stats().commandAttempts == 3U,
                "facade room attempts") &&
         expect(facade.stats().commandSuccesses == 3U,
                "facade room successes");
}

}  // namespace

int main() {
  const bool ok = defaultsBuildDefaultUiModel() &&
                  setActiveToolUpdatesKernelAndOldState() &&
                  selectPressUpdatesSelectionOnlyAndNotDocument() &&
                  inspectPressUpdatesInspectionOnly() &&
                  measurePressMoveReleaseUpdatesMeasurement() &&
                  leavingMeasureCancelsActiveMeasurement() &&
                  sameMeasureToolActivationKeepsActiveMeasurement() &&
                  leavingMeasurePreservesCompletedMeasurement() &&
                  nonMeasureToolSwitchDoesNotTouchMeasurement() &&
                  pointerMoveUpdatesGhostWithSnap() &&
                  snapSettingsAffectLaterGhostDispatch() &&
                  unknownInputDoesNotChangeKernels() &&
                  roomCommandsStillWorkThroughFacade();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
