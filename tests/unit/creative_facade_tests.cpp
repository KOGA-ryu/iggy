#include "app/iggy3d/creative/Facade.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

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

cr::Id targetId(cr::CreativeObjectId objectId) {
  return static_cast<cr::Id>(objectId);
}

const cr::CreativeUiPanel* findPanel(const cr::CreativeUiModel& model,
                                     cr::CreativeUiPanelKind kind) {
  for (const cr::CreativeUiPanel& panel : model.panels) {
    if (panel.kind == kind) {
      return &panel;
    }
  }
  return nullptr;
}

bool hasGhostPreviewRow(const cr::CreativeUiModel& model) {
  for (const cr::CreativeUiRow& row : model.rows) {
    if (row.kind == cr::CreativeUiRowKind::GhostPreview) {
      return true;
    }
  }
  return false;
}

bool hasRowKind(const cr::CreativeUiModel& model, cr::CreativeUiRowKind kind) {
  for (const cr::CreativeUiRow& row : model.rows) {
    if (row.kind == kind) {
      return true;
    }
  }
  return false;
}

cr::CreativeDocument documentWithRooms(std::string_view name,
                                       cr::CreativeDocumentId documentId,
                                       std::size_t roomCount) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create(std::string{name});
  static_cast<void>(document.assignId(documentId));
  for (std::size_t index = 0; index < roomCount; ++index) {
    cr::CreativeDocumentCreateRequest request;
    request.kind = cr::CreativeObjectKind::Room;
    request.name = "Room " + std::to_string(index + 1U);
    static_cast<void>(document.createObject(request));
  }
  return document;
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

bool cancelInputHidesGhostAndUiDropsGhostRow() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));

  const cr::CreativeFacadeToolDispatchReceipt cancel =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::Cancel, 0.0, 0.0));
  const cr::CreativeUiBuildReceipt ui = facade.buildUiModel();
  const cr::CreativeUiPanel* ghostPanel =
      findPanel(ui.model, cr::CreativeUiPanelKind::Ghost);

  return expect(cancel.accepted, "ghost cancel accepted") &&
         expect(cancel.ghostChanged, "ghost cancel changed ghost") &&
         expect(!facade.ghostState().visible, "ghost cancel hidden") &&
         expect(ui.accepted, "ghost cancel ui accepted") &&
         expect(!ui.model.ghostVisible, "ghost cancel ui ghost hidden") &&
         expect(ghostPanel != nullptr, "ghost cancel panel exists") &&
         expect(ghostPanel != nullptr && !ghostPanel->visible,
                "ghost cancel panel hidden") &&
         expect(ghostPanel != nullptr && ghostPanel->rowCount == 0U,
                "ghost cancel panel no rows") &&
         expect(!hasGhostPreviewRow(ui.model), "ghost cancel no ghost row");
}

bool toolSwitchHidesVisibleGhost() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));

  const bool changed = facade.setActiveTool(cr::Tool::Inspect);

  return expect(changed, "ghost switch changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "ghost switch active inspect") &&
         expect(facade.state().tool == cr::Tool::Inspect,
                "ghost switch old state inspect") &&
         expect(!facade.ghostState().visible, "ghost switch hidden");
}

bool sameToolActivationKeepsVisibleGhost() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));

  const bool changed = facade.setActiveTool(cr::Tool::Select);

  return expect(!changed, "same tool ghost no change") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "same tool ghost active select") &&
         expect(facade.state().tool == cr::Tool::Select,
                "same tool ghost old state select") &&
         expect(facade.ghostState().visible, "same tool ghost remains visible") &&
         expect(facade.ghostState().sourceTool == cr::Tool::Select,
                "same tool ghost source preserved") &&
         expect(facade.ghostState().target.value == 42U,
                "same tool ghost target preserved");
}

bool hiddenGhostToolSwitchPreservesOtherEditorState() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, 1.0, 2.0, 11)));
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, 3.0, 4.0, 22)));

  const bool changed = facade.setActiveTool(cr::Tool::Select);

  return expect(changed, "hidden ghost switch changed") &&
         expect(!facade.ghostState().visible, "hidden ghost remains hidden") &&
         expect(facade.selectionState().selectedTarget.value == 11U,
                "hidden ghost selection preserved") &&
         expect(facade.inspectionState().inspectedTarget.value == 22U,
                "hidden ghost inspection preserved") &&
         expect(!facade.measurementState().active,
                "hidden ghost measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "hidden ghost measurement empty");
}

bool pointerMoveAfterHideShowsGhostWithNewTool() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));

  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerMove, 5.4, 6.2, 77));

  return expect(receipt.ghostChanged, "ghost reshow changed") &&
         expect(facade.ghostState().visible, "ghost reshow visible") &&
         expect(facade.ghostState().sourceTool == cr::Tool::Inspect,
                "ghost reshow source inspect") &&
         expect(facade.ghostState().rawPoint.x == 5.4,
                "ghost reshow raw x") &&
         expect(facade.ghostState().target.value == 77U,
                "ghost reshow target");
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

bool installingValidDocumentReplacesDocumentAndPreservesContentState() {
  cr::Facade facade;
  cr::CreativeDocument document = documentWithRooms("Installed", 77, 1);
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeObjectDirtyFlags dirtyBefore = document.dirtyFlags();
  const cr::CreativeObjectId nextObjectIdBefore = document.nextObjectId();

  const cr::CreativeFacadeDocumentInstallReceipt receipt =
      facade.installDocument(std::move(document));

  return expect(receipt.requested, "install requested") &&
         expect(receipt.accepted, "install accepted") &&
         expect(receipt.changed, "install changed") &&
         expect(receipt.hadPreviousDocument, "install previous document") &&
         expect(receipt.previousDocumentId == cr::kInvalidDocumentId,
                "install previous id invalid") &&
         expect(receipt.nextDocumentId == 77U, "install next id") &&
         expect(receipt.previousObjectCount == 0U,
                "install previous object count") &&
         expect(receipt.nextObjectCount == 1U, "install next object count") &&
         expect(receipt.previousDirtyFlags == 0U,
                "install previous dirty flags") &&
         expect(receipt.nextDirtyFlags == dirtyBefore,
                "install next dirty flags") &&
         expect(receipt.activeToolBefore == cr::Tool::Select,
                "install active tool before") &&
         expect(receipt.activeToolAfter == cr::Tool::Select,
                "install active tool after") &&
         expect(receipt.status == "creative_facade_document_installed",
                "install status") &&
         expect(receipt.reasonCode == "creative_facade_document_installed",
                "install reason") &&
         expect(facade.document().id() == 77U,
                "install document id stored") &&
         expect(facade.document().name() == "Installed",
                "install document name stored") &&
         expect(facade.document().objectCount() == 1U,
                "install document object count") &&
         expect(facade.document().revision() == revisionBefore,
                "install preserves revision") &&
         expect(facade.document().dirtyFlags() == dirtyBefore,
                "install preserves dirty flags") &&
         expect(facade.document().nextObjectId() == nextObjectIdBefore,
                "install preserves cursor") &&
         expect(facade.findObject(1) != nullptr,
                "install object findable");
}

bool installingInvalidIdDocumentDoesNotMutateExistingFacade() {
  cr::Facade facade;
  cr::CreativeDocument existing = documentWithRooms("Existing", 88, 1);
  const cr::CreativeFacadeDocumentInstallReceipt setup =
      facade.installDocument(std::move(existing));
  const cr::CreativeObjectId selectedId = facade.document().objects()[0].id;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(selectedId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  const std::uint64_t revisionBefore = facade.document().revision();
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();

  cr::CreativeDocument invalid = cr::CreativeDocument::create("No Id");
  const cr::CreativeFacadeDocumentInstallReceipt receipt =
      facade.installDocument(std::move(invalid));

  return expect(setup.accepted, "install invalid setup accepted") &&
         expect(receipt.requested, "install invalid requested") &&
         expect(!receipt.accepted, "install invalid rejected") &&
         expect(!receipt.changed, "install invalid unchanged") &&
         expect(receipt.previousDocumentId == 88U,
                "install invalid previous id") &&
         expect(receipt.nextDocumentId == cr::kInvalidDocumentId,
                "install invalid next id") &&
         expect(receipt.status == "creative_facade_document_id_missing",
                "install invalid status") &&
         expect(facade.document().id() == 88U,
                "install invalid keeps document id") &&
         expect(facade.document().objectCount() == 1U,
                "install invalid keeps objects") &&
         expect(facade.document().revision() == revisionBefore,
                "install invalid keeps revision") &&
         expect(facade.document().dirtyFlags() == dirtyBefore,
                "install invalid keeps dirty flags") &&
         expect(facade.toolState().activeTool == cr::Tool::Inspect,
                "install invalid keeps active tool") &&
         expect(facade.selectionState().selectedTarget.value ==
                    targetId(selectedId),
                "install invalid keeps selection") &&
         expect(facade.state().selected.value == targetId(selectedId),
                "install invalid keeps old selected");
}

bool installingDocumentClearsTransientEditorState() {
  cr::Facade facade;
  cr::CreativeDocument oldDocument = documentWithRooms("Old", 90, 2);
  const cr::CreativeFacadeDocumentInstallReceipt setup =
      facade.installDocument(std::move(oldDocument));
  const cr::CreativeObjectId firstId = facade.document().objects()[0].id;
  const cr::CreativeObjectId secondId = facade.document().objects()[1].id;

  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(firstId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   3.0,
                   4.0,
                   targetId(secondId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   5.0,
                   6.0,
                   targetId(firstId))));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerRelease,
                   7.0,
                   8.0,
                   targetId(firstId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   9.0,
                   10.0,
                   targetId(secondId))));
  static_cast<void>(
      facade.createDocumentObject(cr::CreativeObjectKind::Room));

  cr::CreativeDocument nextDocument = documentWithRooms("Next", 91, 1);
  const std::uint64_t nextRevision = nextDocument.revision();
  const cr::CreativeObjectDirtyFlags nextDirty = nextDocument.dirtyFlags();
  const cr::CreativeFacadeDocumentInstallReceipt receipt =
      facade.installDocument(std::move(nextDocument));
  const cr::CreativeUiBuildReceipt ui = facade.buildUiModel();

  return expect(setup.accepted, "install clear setup accepted") &&
         expect(receipt.accepted, "install clear accepted") &&
         expect(receipt.selectionCleared, "install clears selection flag") &&
         expect(receipt.inspectionCleared, "install clears inspection flag") &&
         expect(receipt.measurementCleared, "install clears measurement flag") &&
         expect(receipt.ghostCleared, "install clears ghost flag") &&
         expect(receipt.toolPointerCleared,
                "install clears tool pointer flag") &&
         expect(receipt.nextDocumentId == 91U, "install clear next id") &&
         expect(receipt.nextObjectCount == 1U,
                "install clear next object count") &&
         expect(receipt.nextDirtyFlags == nextDirty,
                "install clear next dirty flags") &&
         expect(facade.document().id() == 91U,
                "install clear document id") &&
         expect(facade.document().revision() == nextRevision,
                "install clear preserves revision") &&
         expect(facade.document().dirtyFlags() == nextDirty,
                "install clear preserves dirty") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "install clear active tool default") &&
         expect(facade.state().tool == cr::Tool::Select,
                "install clear old tool default") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "install clear selection empty") &&
         expect(facade.selectionState().candidateTarget.value ==
                    cr::kInvalidId,
                "install clear selection candidate empty") &&
         expect(facade.inspectionState().inspectedTarget.value ==
                    cr::kInvalidId,
                "install clear inspection empty") &&
         expect(facade.inspectionState().candidateTarget.value ==
                    cr::kInvalidId,
                "install clear inspection candidate empty") &&
         expect(!facade.measurementState().active,
                "install clear measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "install clear measurement empty") &&
         expect(facade.measurementState().sampleCount == 0U,
                "install clear measurement samples zero") &&
         expect(!facade.ghostState().visible, "install clear ghost hidden") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "install clear tool pointer target") &&
         expect(facade.state().selected.value == cr::kInvalidId,
                "install clear old selected") &&
         expect(facade.state().hovered.value == cr::kInvalidId,
                "install clear old hovered") &&
         expect(facade.stats().commandAttempts == 0U,
                "install clear stats reset attempts") &&
         expect(ui.accepted, "install clear ui accepted") &&
         expect(ui.rowCount == 4U, "install clear default row count") &&
         expect(!hasRowKind(ui.model, cr::CreativeUiRowKind::SelectedTarget),
                "install clear no selected row") &&
         expect(!hasRowKind(ui.model, cr::CreativeUiRowKind::InspectedTarget),
                "install clear no inspected row") &&
         expect(!hasRowKind(ui.model, cr::CreativeUiRowKind::MeasurementState),
                "install clear no measurement row") &&
         expect(!hasGhostPreviewRow(ui.model), "install clear no ghost row");
}

bool installingSecondDocumentDoesNotLeakOldSelectionRows() {
  cr::Facade facade;
  cr::CreativeDocument first = documentWithRooms("First", 101, 2);
  const cr::CreativeFacadeDocumentInstallReceipt firstInstall =
      facade.installDocument(std::move(first));
  const cr::CreativeObjectId firstSelectedId = facade.document().objects()[0].id;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(firstSelectedId))));
  const cr::CreativeUiBuildReceipt before = facade.buildUiModel();

  cr::CreativeDocument second = documentWithRooms("Second", 102, 1);
  const cr::CreativeFacadeDocumentInstallReceipt secondInstall =
      facade.installDocument(std::move(second));
  const cr::CreativeUiBuildReceipt after = facade.buildUiModel();

  return expect(firstInstall.accepted, "install second setup accepted") &&
         expect(before.accepted, "install second before ui accepted") &&
         expect(hasRowKind(before.model, cr::CreativeUiRowKind::SelectedTarget),
                "install second before selected row") &&
         expect(secondInstall.accepted, "install second accepted") &&
         expect(secondInstall.selectionCleared,
                "install second selection cleared") &&
         expect(facade.document().id() == 102U,
                "install second document id") &&
         expect(facade.document().objectCount() == 1U,
                "install second object count") &&
         expect(after.accepted, "install second after ui accepted") &&
         expect(after.model.objectSummaries.size() == 1U,
                "install second summaries only new document") &&
         expect(!hasRowKind(after.model, cr::CreativeUiRowKind::SelectedTarget),
                "install second no selected row") &&
         expect(!hasRowKind(after.model, cr::CreativeUiRowKind::InspectedTarget),
                "install second no inspected row");
}

bool removingSelectedObjectClearsSelectionAndOldState() {
  cr::Facade facade;
  const cr::CreativeObjectId id = facade.createRoom("Selected Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(id))));
  const std::uint64_t revisionBeforeRemove = facade.document().revision();

  const bool removed = facade.removeObject(id);

  return expect(removed, "remove selected object succeeds") &&
         expect(facade.findObject(id) == nullptr,
                "remove selected object gone") &&
         expect(facade.document().objectCount() == 0U,
                "remove selected object count") &&
         expect(facade.document().revision() == revisionBeforeRemove + 1U,
                "remove selected revision increments") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "remove selected clears selection") &&
         expect(facade.selectionState().candidateTarget.value == cr::kInvalidId,
                "remove selected candidate empty") &&
         expect(facade.state().selected.value == cr::kInvalidId,
                "remove selected clears old selected") &&
         expect(facade.state().hovered.value == cr::kInvalidId,
                "remove selected clears old hovered") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "remove selected clears tool pointer target");
}

bool removingInspectedObjectClearsInspectionOnly() {
  cr::Facade facade;
  const cr::CreativeObjectId selectedId = facade.createRoom("Selected Room");
  const cr::CreativeObjectId inspectedId = facade.createRoom("Inspected Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(selectedId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   3.0,
                   4.0,
                   targetId(inspectedId))));

  const bool removed = facade.removeObject(inspectedId);

  return expect(removed, "remove inspected object succeeds") &&
         expect(facade.findObject(inspectedId) == nullptr,
                "remove inspected object gone") &&
         expect(facade.document().objectCount() == 1U,
                "remove inspected leaves selected object") &&
         expect(facade.selectionState().selectedTarget.value ==
                    targetId(selectedId),
                "remove inspected preserves selection") &&
         expect(facade.state().selected.value == targetId(selectedId),
                "remove inspected preserves old selected") &&
         expect(facade.inspectionState().inspectedTarget.value ==
                    cr::kInvalidId,
                "remove inspected clears inspection") &&
         expect(facade.inspectionState().candidateTarget.value ==
                    cr::kInvalidId,
                "remove inspected candidate empty");
}

bool removingMissingObjectPreservesEditorTargets() {
  cr::Facade facade;
  const cr::CreativeObjectId selectedId = facade.createRoom("Selected Room");
  const cr::CreativeObjectId inspectedId = facade.createRoom("Inspected Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(selectedId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   3.0,
                   4.0,
                   targetId(inspectedId))));
  const std::uint64_t revisionBeforeRemove = facade.document().revision();

  const bool removed = facade.removeObject(9999);

  return expect(!removed, "remove missing object fails") &&
         expect(facade.document().objectCount() == 2U,
                "remove missing keeps objects") &&
         expect(facade.document().revision() == revisionBeforeRemove,
                "remove missing revision stable") &&
         expect(facade.selectionState().selectedTarget.value ==
                    targetId(selectedId),
                "remove missing preserves selection") &&
         expect(facade.inspectionState().inspectedTarget.value ==
                    targetId(inspectedId),
                "remove missing preserves inspection") &&
         expect(facade.state().selected.value == targetId(selectedId),
                "remove missing preserves old selected") &&
         expect(facade.state().hovered.value == targetId(inspectedId),
                "remove missing preserves hovered");
}

bool removingGhostTargetHidesGhost() {
  cr::Facade facade;
  const cr::CreativeObjectId ghostId = facade.createRoom("Ghost Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   1.0,
                   2.0,
                   targetId(ghostId))));

  const bool removed = facade.removeObject(ghostId);

  return expect(removed, "remove ghost target succeeds") &&
         expect(!facade.ghostState().visible, "remove ghost target hides ghost");
}

bool removingDifferentObjectPreservesGhost() {
  cr::Facade facade;
  const cr::CreativeObjectId removedId = facade.createRoom("Removed Room");
  const cr::CreativeObjectId ghostId = facade.createRoom("Ghost Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   1.0,
                   2.0,
                   targetId(ghostId))));

  const bool removed = facade.removeObject(removedId);

  return expect(removed, "remove different ghost object succeeds") &&
         expect(facade.ghostState().visible,
                "remove different object preserves ghost") &&
         expect(facade.ghostState().target.value == targetId(ghostId),
                "remove different object preserves ghost target");
}

bool removingMeasurementTargetClearsMeasurement() {
  cr::Facade facade;
  const cr::CreativeObjectId measuredId = facade.createRoom("Measured Room");
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(measuredId))));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   3.0,
                   4.0,
                   targetId(measuredId))));

  const bool removed = facade.removeObject(measuredId);

  return expect(removed, "remove measurement target succeeds") &&
         expect(!facade.measurementState().active,
                "remove measurement target inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "remove measurement target cleared") &&
         expect(!facade.toolState().measurementActive,
                "remove measurement target clears tool measurement flag") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "remove measurement target clears tool pointer");
}

bool removingDifferentObjectPreservesMeasurement() {
  cr::Facade facade;
  const cr::CreativeObjectId removedId = facade.createRoom("Removed Room");
  const cr::CreativeObjectId measuredId = facade.createRoom("Measured Room");
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(measuredId))));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   3.0,
                   4.0,
                   targetId(measuredId))));

  const bool removed = facade.removeObject(removedId);

  return expect(removed, "remove different measurement object succeeds") &&
         expect(facade.measurementState().active,
                "remove different object keeps measurement active") &&
         expect(facade.measurementState().hasMeasurement,
                "remove different object keeps measurement") &&
         expect(facade.toolState().measurementActive,
                "remove different object keeps tool measurement flag") &&
         expect(facade.measurementState().startPoint.target.value ==
                    targetId(measuredId),
                "remove different object keeps start target") &&
         expect(facade.measurementState().currentPoint.target.value ==
                    targetId(measuredId),
                "remove different object keeps current target");
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
                  cancelInputHidesGhostAndUiDropsGhostRow() &&
                  toolSwitchHidesVisibleGhost() &&
                  sameToolActivationKeepsVisibleGhost() &&
                  hiddenGhostToolSwitchPreservesOtherEditorState() &&
                  pointerMoveAfterHideShowsGhostWithNewTool() &&
                  snapSettingsAffectLaterGhostDispatch() &&
                  unknownInputDoesNotChangeKernels() &&
                  roomCommandsStillWorkThroughFacade() &&
                  installingValidDocumentReplacesDocumentAndPreservesContentState() &&
                  installingInvalidIdDocumentDoesNotMutateExistingFacade() &&
                  installingDocumentClearsTransientEditorState() &&
                  installingSecondDocumentDoesNotLeakOldSelectionRows() &&
                  removingSelectedObjectClearsSelectionAndOldState() &&
                  removingInspectedObjectClearsInspectionOnly() &&
                  removingMissingObjectPreservesEditorTargets() &&
                  removingGhostTargetHidesGhost() &&
                  removingDifferentObjectPreservesGhost() &&
                  removingMeasurementTargetClearsMeasurement() &&
                  removingDifferentObjectPreservesMeasurement();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
