#include "app/iggy3d/creative/Facade.hpp"

#include <algorithm>
#include <cmath>
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

bool near(double lhs, double rhs, double epsilon = 1.0e-9) {
  return std::fabs(lhs - rhs) <= epsilon;
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

cr::CreativeObjectId createRoom(cr::Facade& facade, std::string_view name) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = std::string{name};
  return facade.createDocumentObject(request).objectId;
}

bool removeObject(cr::Facade& facade, cr::CreativeObjectId objectId) {
  return facade.removeDocumentObject(objectId).objectRemoved;
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

bool defaultsExposeDefaultEditorState() {
  const cr::Facade facade;

  return expect(facade.toolState().activeTool == cr::Tool::Select,
                "default tool state") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "default selection") &&
         expect(!facade.measurementState().active,
                "default measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "default measurement empty") &&
         expect(!facade.ghostState().visible, "default ghost hidden");
}

bool setActiveToolUpdatesCanonicalState() {
  cr::Facade facade;
  const bool changed = facade.setActiveTool(cr::Tool::Move);
  const bool same = facade.setActiveTool(cr::Tool::Move);

  return expect(changed, "set active changed") &&
         expect(!same, "set active same no change") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "tool state move");
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
         expect(!receipt.measurementChanged, "select measurement unchanged") &&
         expect(facade.selectionState().selectedTarget.value == 42U,
                "select state target") &&
         expect(facade.toolState().pointer.target.value == 42U,
                "select pointer target") &&
         expect(facade.document().objectCount() == initialObjects,
                "select did not mutate document");
}

bool movePressSelectsLikeSelect() {
  // Move selects like Select until the drag slice (TV1-F/G) lands.
  cr::Facade facade;
  const bool toolChanged = facade.setActiveTool(cr::Tool::Move);
  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerPress,
                       4.0,
                       5.0,
                       77));

  return expect(toolChanged, "move tool changed") &&
         expect(receipt.accepted, "move dispatch accepted") &&
         expect(receipt.selectionChanged, "move changed selection") &&
         expect(facade.selectionState().selectedTarget.value == 77U,
                "move selection target") &&
         expect(facade.document().objectCount() == 0U,
                "move press did not mutate document");
}

bool navigatePointerInputDoesNotTouchEditorState() {
  cr::Facade facade;
  const bool toolChanged = facade.setActiveTool(cr::Tool::Navigate);
  const cr::CreativeFacadeToolDispatchReceipt press =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerPress,
                       4.0,
                       5.0,
                       77));
  const cr::CreativeFacadeToolDispatchReceipt move =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerMove,
                       6.0,
                       7.0,
                       88));

  return expect(toolChanged, "navigate tool changed") &&
         expect(press.accepted, "navigate press accepted") &&
         expect(press.emittedIntentCount == 0U, "navigate press inert") &&
         expect(!press.changed, "navigate press unchanged") &&
         expect(press.message == "navigate_pointer_inert",
                "navigate press message") &&
         expect(move.emittedIntentCount == 0U, "navigate move inert") &&
         expect(!facade.ghostState().visible, "navigate no ghost") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "navigate selection untouched") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "navigate pointer target untouched") &&
         expect(facade.document().objectCount() == 0U,
                "navigate did not mutate document");
}

bool switchingToNavigateCancelsActiveMeasurement() {
  cr::Facade facade;
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, 1.0, 2.0, 7)));

  const bool changed = facade.setActiveTool(cr::Tool::Navigate);

  return expect(changed, "navigate switch changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Navigate,
                "navigate switch active tool") &&
         expect(!facade.measurementState().active,
                "navigate switch cancels measurement") &&
         expect(!facade.measurementState().hasMeasurement,
                "navigate switch clears measurement") &&
         expect(!facade.toolState().measurementActive,
                "navigate switch clears tool measurement flag");
}

bool measureClickClickUpdatesMeasurement() {
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
          pointerInput(cr::CreativeToolInputKind::PointerPress,
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

bool measurementAnnotationsAreExplicitDocumentMutations() {
  cr::Facade facade;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Measurement Annotations");
  static_cast<void>(document.assignId(93U));
  if (!expect(facade.installDocument(std::move(document)).accepted,
              "measurement annotation document installed")) {
    return false;
  }

  const std::uint64_t revisionBefore = facade.document().revision();
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, 1.0, 2.0, 7)));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, 5.0, 6.0, 9)));
  const bool transientStayedOutsideDocument =
      facade.document().revision() == revisionBefore &&
      facade.document().measurementAnnotationStore().annotations.empty();

  const cr::CreativeFacadeMeasurementAnnotationSaveReceipt saved =
      facade.saveMeasurementAnnotation("Door clearance");
  const cr::CreativeMeasurementAnnotation* annotation =
      cr::findCreativeMeasurementAnnotation(
          facade.document().measurementAnnotationStore(), saved.annotationId);
  const bool savedExactlyOnce =
      saved.accepted && saved.changed &&
      saved.annotationId != cr::kInvalidCreativeMeasurementAnnotationId &&
      annotation != nullptr && annotation->name == "Door clearance" &&
      facade.document().revision() == revisionBefore + 1U &&
      !facade.measurementState().hasMeasurement;

  const cr::CreativeMeasurementAnnotationMutationReceipt removed =
      facade.removeMeasurementAnnotation(saved.annotationId);
  return expect(transientStayedOutsideDocument,
                "transient measurement changes no document state") &&
         expect(savedExactlyOnce,
                "explicit measurement save creates one durable annotation") &&
         expect(removed.accepted && removed.changed &&
                    facade.document()
                        .measurementAnnotationStore()
                        .annotations.empty() &&
                    facade.document().revision() == revisionBefore + 2U,
                "explicit measurement removal changes the document once");
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
      pointerInput(cr::CreativeToolInputKind::PointerPress, 5.0, 6.0, 9)));

  const bool changed = facade.setActiveTool(cr::Tool::Select);

  return expect(changed, "completed measure leave changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "completed measure active select") &&
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

  const bool changed = facade.setActiveTool(cr::Tool::Move);

  return expect(changed, "non-measure switch changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "non-measure active move") &&
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

bool cancelInputHidesGhost() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));

  const cr::CreativeFacadeToolDispatchReceipt cancel =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::Cancel, 0.0, 0.0));

  return expect(cancel.accepted, "ghost cancel accepted") &&
         expect(cancel.ghostChanged, "ghost cancel changed ghost") &&
         expect(!facade.ghostState().visible, "ghost cancel hidden");
}

bool toolSwitchHidesVisibleGhost() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));

  const bool changed = facade.setActiveTool(cr::Tool::Move);

  return expect(changed, "ghost switch changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "ghost switch active move") &&
         expect(!facade.ghostState().visible, "ghost switch hidden");
}

bool switchingToNavigateHidesVisibleGhost() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));

  const bool changed = facade.setActiveTool(cr::Tool::Navigate);

  return expect(changed, "navigate ghost switch changed") &&
         expect(facade.toolState().activeTool == cr::Tool::Navigate,
                "navigate ghost switch active tool") &&
         expect(!facade.ghostState().visible, "navigate ghost switch hidden");
}

bool sameToolActivationKeepsVisibleGhost() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));

  const bool changed = facade.setActiveTool(cr::Tool::Select);

  return expect(!changed, "same tool ghost no change") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "same tool ghost active select") &&
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
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));

  const bool changed = facade.setActiveTool(cr::Tool::Select);

  return expect(changed, "hidden ghost switch changed") &&
         expect(!facade.ghostState().visible, "hidden ghost remains hidden") &&
         expect(facade.selectionState().selectedTarget.value == 11U,
                "hidden ghost selection preserved") &&
         expect(!facade.measurementState().active,
                "hidden ghost measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "hidden ghost measurement empty");
}

bool pointerMoveAfterHideShowsGhostWithNewTool() {
  cr::Facade facade;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove, 1.2, 2.7, 42)));
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));

  const cr::CreativeFacadeToolDispatchReceipt receipt =
      facade.dispatchToolInput(
          pointerInput(cr::CreativeToolInputKind::PointerMove, 5.4, 6.2, 77));

  return expect(receipt.ghostChanged, "ghost reshow changed") &&
         expect(facade.ghostState().visible, "ghost reshow visible") &&
         expect(facade.ghostState().sourceTool == cr::Tool::Move,
                "ghost reshow source move") &&
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
         expect(!receipt.measurementChanged, "unknown measurement unchanged") &&
         expect(!receipt.ghostChanged, "unknown ghost unchanged") &&
         expect(receipt.message == "unsupported_input", "unknown message") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "unknown selection state") &&
         expect(!facade.measurementState().hasMeasurement,
                "unknown measurement state") &&
         expect(!facade.ghostState().visible, "unknown ghost state");
}

bool roomCommandsStillWorkThroughFacade() {
  cr::Facade facade;
  const cr::CreativeObjectId id = createRoom(facade, "Facade Room");
  const cr::CreativeDocumentMutationReceipt renamed =
      facade.mutateObject(id, cr::CreativeMutationKind::Rename,
                          cr::makeRenamePayload("Facade Room Renamed"));
  const bool removed = removeObject(facade, id);

  return expect(id != cr::kInvalidObjectId, "facade room id") &&
         expect(renamed.status == cr::CreativeDocumentMutationStatus::Applied,
                "facade room renamed") &&
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
         expect(receipt.previousLiveRevision == 0U,
                "initial install previous live revision") &&
         expect(receipt.incomingRevision == revisionBefore,
                "initial install incoming revision") &&
         expect(receipt.installedRevision == revisionBefore,
                "initial install preserves incoming revision fact") &&
         expect(receipt.revisionHighWaterBefore == 0U &&
                    receipt.revisionHighWaterAfter == revisionBefore,
                "initial install establishes revision high water") &&
         expect(!receipt.revisionRebased && receipt.initialInstall,
                "initial install reports no rebase") &&
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

bool saveAcknowledgementDrainsOnlyTheExactPublishedDocument() {
  cr::Facade facade;
  cr::CreativeDocument document =
      documentWithRooms("Save Acknowledge", 78U, 1U);
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      facade.installDocument(std::move(document));
  const cr::CreativeDocumentId documentId = facade.document().id();
  const std::uint64_t revision = facade.document().revision();
  const cr::CreativeObjectDirtyFlags dirtyFlags =
      facade.document().dirtyFlags();

  const cr::CreativeFacadeDocumentSaveAcknowledgeReceipt wrongId =
      facade.acknowledgeDocumentSaved(documentId + 1U, revision);
  const cr::CreativeFacadeDocumentSaveAcknowledgeReceipt wrongRevision =
      facade.acknowledgeDocumentSaved(documentId, revision + 1U);
  const bool rejectedAcknowledgementsPreservedDocument =
      facade.document().id() == documentId &&
      facade.document().revision() == revision &&
      facade.document().dirtyFlags() == dirtyFlags;
  const cr::CreativeFacadeDocumentSaveAcknowledgeReceipt acknowledged =
      facade.acknowledgeDocumentSaved(documentId, revision);
  const cr::CreativeFacadeDocumentSaveAcknowledgeReceipt clean =
      facade.acknowledgeDocumentSaved(documentId, revision);

  return expect(installed.accepted && dirtyFlags != 0U,
                "save acknowledgement fixture is dirty") &&
         expect(!wrongId.accepted && !wrongId.changed &&
                    wrongId.reasonCode ==
                        "creative_facade_document_save_id_mismatch" &&
                    facade.document().revision() == revision,
                "save acknowledgement rejects a different document") &&
         expect(!wrongRevision.accepted && !wrongRevision.changed &&
                    wrongRevision.reasonCode ==
                        "creative_facade_document_save_revision_mismatch",
                "save acknowledgement rejects a stale revision") &&
         expect(rejectedAcknowledgementsPreservedDocument,
                "rejected save acknowledgements preserve the live document") &&
         expect(acknowledged.accepted && acknowledged.changed &&
                    acknowledged.dirtyFlagsBefore == dirtyFlags &&
                    acknowledged.dirtyFlagsDrained == dirtyFlags &&
                    acknowledged.dirtyFlagsAfter == 0U &&
                    acknowledged.revisionBefore == revision &&
                    acknowledged.revisionAfter == revision &&
                    facade.document().dirtyFlags() == 0U,
                "save acknowledgement drains dirty domains without mutation") &&
         expect(clean.accepted && !clean.changed &&
                    clean.dirtyFlagsDrained == 0U &&
                    clean.revisionAfter == revision,
                "save acknowledgement is idempotent for a clean document");
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
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
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
         expect(receipt.previousLiveRevision == revisionBefore &&
                    receipt.installedRevision == revisionBefore,
                "install invalid reports unchanged live revision") &&
         expect(receipt.revisionHighWaterBefore ==
                    setup.revisionHighWaterAfter &&
                    receipt.revisionHighWaterAfter ==
                        setup.revisionHighWaterAfter,
                "install invalid leaves revision high water unchanged") &&
         expect(!receipt.revisionRebased && !receipt.initialInstall,
                "install invalid reports no publication") &&
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
         expect(facade.toolState().activeTool == cr::Tool::Move,
                "install invalid keeps active tool") &&
         expect(facade.selectionState().selectedTarget.value ==
                    targetId(selectedId),
                "install invalid keeps selection");
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
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
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
      pointerInput(cr::CreativeToolInputKind::PointerPress,
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
  const std::uint64_t liveRevisionBeforeInstall =
      facade.document().revision();
  const std::uint64_t expectedInstalledRevision =
      std::max({setup.revisionHighWaterAfter,
                liveRevisionBeforeInstall,
                nextRevision}) +
      1U;
  const cr::CreativeFacadeDocumentInstallReceipt receipt =
      facade.installDocument(std::move(nextDocument));

  return expect(setup.accepted, "install clear setup accepted") &&
         expect(receipt.accepted, "install clear accepted") &&
         expect(receipt.selectionCleared, "install clears selection flag") &&
         expect(receipt.measurementCleared, "install clears measurement flag") &&
         expect(receipt.ghostCleared, "install clears ghost flag") &&
         expect(receipt.toolPointerCleared,
                "install clears tool pointer flag") &&
         expect(receipt.nextDocumentId == 91U, "install clear next id") &&
         expect(receipt.nextObjectCount == 1U,
                "install clear next object count") &&
         expect(receipt.nextDirtyFlags == nextDirty,
                "install clear next dirty flags") &&
         expect(receipt.previousLiveRevision == liveRevisionBeforeInstall &&
                    receipt.incomingRevision == nextRevision &&
                    receipt.installedRevision == expectedInstalledRevision,
                "replacement install reports revision lineage") &&
         expect(receipt.revisionHighWaterBefore ==
                    setup.revisionHighWaterAfter &&
                    receipt.revisionHighWaterAfter ==
                        expectedInstalledRevision,
                "replacement install advances revision high water") &&
         expect(receipt.revisionRebased && !receipt.initialInstall,
                "replacement install reports rebase") &&
         expect(facade.document().id() == 91U,
                "install clear document id") &&
         expect(facade.document().revision() == expectedInstalledRevision,
                "install clear rebases revision") &&
         expect(facade.document().dirtyFlags() == nextDirty,
                "install clear preserves dirty") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "install clear active tool default") &&
         expect(facade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "install clear selection empty") &&
         expect(facade.selectionState().candidateTarget.value ==
                    cr::kInvalidId,
                "install clear selection candidate empty") &&
         expect(!facade.measurementState().active,
                "install clear measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "install clear measurement empty") &&
         expect(facade.measurementState().sampleCount == 0U,
                "install clear measurement samples zero") &&
         expect(!facade.ghostState().visible, "install clear ghost hidden") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "install clear tool pointer target") &&
         expect(facade.stats().commandAttempts == 0U,
                "install clear stats reset attempts");
}

bool installingSecondDocumentDoesNotLeakOldSelection() {
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
  const bool selectedBefore =
      facade.selectionState().selectedTarget.value ==
      targetId(firstSelectedId);

  cr::CreativeDocument second = documentWithRooms("Second", 102, 1);
  const cr::CreativeFacadeDocumentInstallReceipt secondInstall =
      facade.installDocument(std::move(second));

  return expect(firstInstall.accepted, "install second setup accepted") &&
         expect(selectedBefore, "install second before selection exists") &&
         expect(secondInstall.accepted, "install second accepted") &&
         expect(secondInstall.selectionCleared,
                "install second selection cleared") &&
         expect(secondInstall.revisionRebased &&
                    !secondInstall.initialInstall &&
                    secondInstall.installedRevision >
                        firstInstall.installedRevision,
                "install second advances live revision") &&
         expect(facade.document().id() == 102U,
                "install second document id") &&
         expect(facade.document().objectCount() == 1U,
                "install second object count") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "install second clears selected target") &&
         expect(facade.selectionState().candidateTarget.value == cr::kInvalidId,
                "install second clears candidate target");
}

bool resetAndFacadeCopiesPreserveRevisionLineage() {
  cr::Facade facade;
  cr::CreativeDocument initial = documentWithRooms("Initial", 120U, 2U);
  const cr::CreativeFacadeDocumentInstallReceipt initialInstall =
      facade.installDocument(std::move(initial));
  static_cast<void>(
      facade.createDocumentObject(cr::CreativeObjectKind::Room));
  const std::uint64_t revisionBeforeReset = facade.document().revision();

  facade.reset();
  cr::Facade copied = facade;
  cr::CreativeDocument copyReplacement =
      documentWithRooms("Copy Replacement", 121U, 1U);
  const std::uint64_t copyIncomingRevision = copyReplacement.revision();
  const cr::CreativeFacadeDocumentInstallReceipt copyInstall =
      copied.installDocument(std::move(copyReplacement));

  cr::Facade moved = std::move(copied);
  cr::CreativeDocument moveReplacement =
      documentWithRooms("Move Replacement", 122U, 0U);
  const cr::CreativeFacadeDocumentInstallReceipt moveInstall =
      moved.installDocument(std::move(moveReplacement));

  return expect(initialInstall.accepted && initialInstall.initialInstall,
                "lineage fixture initial install") &&
         expect(revisionBeforeReset > initialInstall.installedRevision,
                "lineage fixture advances before reset") &&
         expect(facade.document().id() == cr::kInvalidDocumentId &&
                    facade.document().revision() == 0U,
                "reset clears the live document") &&
         expect(copyInstall.accepted && copyInstall.revisionRebased &&
                    !copyInstall.initialInstall &&
                    copyInstall.revisionHighWaterBefore ==
                        revisionBeforeReset &&
                    copyInstall.incomingRevision == copyIncomingRevision &&
                    copyInstall.installedRevision > revisionBeforeReset,
                "copied Facade preserves reset revision lineage") &&
         expect(moveInstall.accepted && moveInstall.revisionRebased &&
                    !moveInstall.initialInstall &&
                    moveInstall.revisionHighWaterBefore ==
                        copyInstall.revisionHighWaterAfter &&
                    moveInstall.installedRevision >
                        copyInstall.installedRevision,
                "moved Facade preserves revision lineage");
}

bool rejectedInstallDoesNotAdvanceRevisionLineage() {
  cr::Facade facade;
  cr::CreativeDocument initial =
      documentWithRooms("Rejected Install Baseline", 123U, 1U);
  const cr::CreativeFacadeDocumentInstallReceipt initialInstall =
      facade.installDocument(std::move(initial));
  static_cast<void>(
      facade.createDocumentObject(cr::CreativeObjectKind::Room));
  const std::uint64_t liveRevisionBeforeReject =
      facade.document().revision();

  cr::CreativeDocument invalid =
      cr::CreativeDocument::create("Missing Document Id");
  const cr::CreativeFacadeDocumentInstallReceipt rejected =
      facade.installDocument(std::move(invalid));

  cr::CreativeDocument replacement =
      documentWithRooms("After Rejection", 124U, 0U);
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      facade.installDocument(std::move(replacement));

  return expect(initialInstall.accepted,
                "rejected lineage fixture initial install") &&
         expect(!rejected.accepted && !rejected.changed &&
                    rejected.installedRevision == liveRevisionBeforeReject &&
                    rejected.revisionHighWaterBefore ==
                        initialInstall.revisionHighWaterAfter &&
                    rejected.revisionHighWaterAfter ==
                        initialInstall.revisionHighWaterAfter,
                "rejected install leaves revision lineage unchanged") &&
         expect(installed.accepted && installed.revisionRebased &&
                    installed.previousLiveRevision ==
                        liveRevisionBeforeReject &&
                    installed.revisionHighWaterBefore ==
                        rejected.revisionHighWaterAfter &&
                    installed.installedRevision ==
                        liveRevisionBeforeReject + 1U,
                "next accepted install advances from unchanged live truth");
}

bool removingSelectedObjectClearsCanonicalState() {
  cr::Facade facade;
  const cr::CreativeObjectId id = createRoom(facade, "Selected Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(id))));
  const std::uint64_t revisionBeforeRemove = facade.document().revision();

  const bool removed = removeObject(facade, id);

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
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "remove selected clears tool pointer target");
}

bool removingHoveredObjectPreservesSelection() {
  cr::Facade facade;
  const cr::CreativeObjectId selectedId = createRoom(facade, "Selected Room");
  const cr::CreativeObjectId hoveredId = createRoom(facade, "Hovered Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(selectedId))));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   3.0,
                   4.0,
                   targetId(hoveredId))));

  const bool removed = removeObject(facade, hoveredId);

  return expect(removed, "remove hovered object succeeds") &&
         expect(facade.findObject(hoveredId) == nullptr,
                "remove hovered object gone") &&
         expect(facade.document().objectCount() == 1U,
                "remove hovered leaves selected object") &&
         expect(facade.selectionState().selectedTarget.value ==
                    targetId(selectedId),
                "remove hovered preserves selection") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "remove hovered clears tool pointer target");
}

bool removingMissingObjectPreservesEditorTargets() {
  cr::Facade facade;
  const cr::CreativeObjectId selectedId = createRoom(facade, "Selected Room");
  const cr::CreativeObjectId hoveredId = createRoom(facade, "Hovered Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(selectedId))));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   3.0,
                   4.0,
                   targetId(hoveredId))));
  const std::uint64_t revisionBeforeRemove = facade.document().revision();

  const bool removed = removeObject(facade, 9999);

  return expect(!removed, "remove missing object fails") &&
         expect(facade.document().objectCount() == 2U,
                "remove missing keeps objects") &&
         expect(facade.document().revision() == revisionBeforeRemove,
                "remove missing revision stable") &&
         expect(facade.selectionState().selectedTarget.value ==
                    targetId(selectedId),
                "remove missing preserves selection") &&
         expect(facade.toolState().pointer.target.value == targetId(hoveredId),
                "remove missing preserves pointer target");
}

bool removingGhostTargetHidesGhost() {
  cr::Facade facade;
  const cr::CreativeObjectId ghostId = createRoom(facade, "Ghost Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   1.0,
                   2.0,
                   targetId(ghostId))));

  const bool removed = removeObject(facade, ghostId);

  return expect(removed, "remove ghost target succeeds") &&
         expect(!facade.ghostState().visible, "remove ghost target hides ghost");
}

bool removingDifferentObjectPreservesGhost() {
  cr::Facade facade;
  const cr::CreativeObjectId removedId = createRoom(facade, "Removed Room");
  const cr::CreativeObjectId ghostId = createRoom(facade, "Ghost Room");
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   1.0,
                   2.0,
                   targetId(ghostId))));

  const bool removed = removeObject(facade, removedId);

  return expect(removed, "remove different ghost object succeeds") &&
         expect(facade.ghostState().visible,
                "remove different object preserves ghost") &&
         expect(facade.ghostState().target.value == targetId(ghostId),
                "remove different object preserves ghost target");
}

bool removingMeasurementTargetClearsMeasurement() {
  cr::Facade facade;
  const cr::CreativeObjectId measuredId = createRoom(facade, "Measured Room");
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

  const bool removed = removeObject(facade, measuredId);

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
  const cr::CreativeObjectId removedId = createRoom(facade, "Removed Room");
  const cr::CreativeObjectId measuredId = createRoom(facade, "Measured Room");
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

  const bool removed = removeObject(facade, removedId);

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

bool linearArrayCreatesCopiesAndSelectsOnlyFinalInstance() {
  cr::Facade facade;
  cr::CreativeDocument document = cr::CreativeDocument::create("Array Test");
  static_cast<void>(document.assignId(91U));
  const cr::CreativeFacadeDocumentInstallReceipt install =
      facade.installDocument(std::move(document));
  if (!expect(install.accepted, "array test document installed")) {
    return false;
  }
  const cr::CreativeObjectId firstId = createRoom(facade, "First Room");
  const cr::CreativeObjectId secondId = createRoom(facade, "Second Room");
  const cr::CreativeObject* firstBefore = facade.findObject(firstId);
  const cr::CreativeObject* secondBefore = facade.findObject(secondId);
  if (!expect(firstBefore != nullptr && secondBefore != nullptr,
              "array sources exist")) {
    return false;
  }
  const double firstMinX = firstBefore->bounds.min.x;
  const double secondMinX = secondBefore->bounds.min.x;

  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(firstId))));
  cr::CreativeToolInputPacket addSecond =
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   3.0,
                   4.0,
                   targetId(secondId));
  addSecond.pointer.modifiers = cr::kCreativeToolModifierShift;
  static_cast<void>(facade.dispatchToolInput(addSecond));

  const cr::Stats statsBefore = facade.stats();
  cr::CreativeLinearArrayRequest request;
  request.direction = cr::CreativeLinearArrayDirection::PositiveX;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  request.spacing = cr::CreativeLinearArraySpacing::OneCell;
  request.cellSize = 1.0;
  const cr::CreativeLinearArrayReceipt receipt =
      facade.createLinearArrayFromSelection(request);

  const std::span<const cr::TargetRef> selected =
      cr::selectedTargetList(facade.selectionState());
  const cr::CreativeObject* finalFirst = facade.findObject(5U);
  const cr::CreativeObject* finalSecond = facade.findObject(6U);
  const cr::Stats& statsAfter = facade.stats();
  return expect(receipt.accepted && receipt.changed,
                std::string{"array facade command accepted: "} +
                    receipt.message) &&
         expect(receipt.generatedObjectCount == 4U &&
                    receipt.generatedObjectIds().size() == 4U,
                "two sources produce two copied instances") &&
         expect(receipt.finalCopyObjectIds().size() == 2U &&
                    receipt.finalCopyObjectIds()[0] == 5U &&
                    receipt.finalCopyObjectIds()[1] == 6U,
                "receipt identifies final generated copy") &&
         expect(selected.size() == 2U && selected[0].value == 5U &&
                    selected[1].value == 6U &&
                    facade.selectionState().selectedTarget.value == 6U,
                "only final copy becomes selected") &&
         expect(finalFirst != nullptr && finalSecond != nullptr &&
                    finalFirst->bounds.min.x == firstMinX + 2.0 &&
                    finalSecond->bounds.min.x == secondMinX + 2.0,
                "final copy uses ordinal-derived two-cell offset") &&
         expect(facade.document().objectCount() == 6U,
                "array preserves originals and creates four objects") &&
         expect(statsAfter.commandAttempts == statsBefore.commandAttempts + 1U &&
                    statsAfter.commandSuccesses ==
                        statsBefore.commandSuccesses + 1U &&
                    statsAfter.commandFailures == statsBefore.commandFailures &&
                    statsAfter.objectsCreated == statsBefore.objectsCreated + 4U &&
                    statsAfter.roomsCreated == statsBefore.roomsCreated + 4U,
                "array records one command and every generated object");
}

bool radialArrayUsesPivotAndSelectsOnlyFinalInstance() {
  cr::Facade facade;
  cr::CreativeDocument document = cr::CreativeDocument::create("Radial Test");
  static_cast<void>(document.assignId(92U));
  if (!expect(facade.installDocument(std::move(document)).accepted,
              "radial test document installed")) {
    return false;
  }
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Room;
  create.name = "Radial Source";
  create.bounds = {{2.0, 0.0, 0.0}, {3.0, 1.0, 1.0}};
  create.hasBoundsOverride = true;
  const cr::CreativeObjectId sourceId = facade.createDocumentObject(create).objectId;
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress, 1.0, 2.0,
                   targetId(sourceId))));

  const cr::Stats statsBefore = facade.stats();
  cr::CreativeRadialArrayRequest request;
  request.pivot = {};
  request.axis = cr::CreativeAxis3::Y;
  request.instanceCount = cr::CreativeRadialArrayInstanceCount::Four;
  request.sweep = cr::CreativeRadialArraySweep::Degrees360;
  const cr::CreativeRadialArrayReceipt receipt =
      facade.createRadialArrayFromSelection(request);
  const std::span<const cr::TargetRef> selected =
      cr::selectedTargetList(facade.selectionState());
  const cr::CreativeObject* final = facade.findObject(4U);
  const cr::Stats& statsAfter = facade.stats();

  return expect(receipt.accepted && receipt.changed &&
                    receipt.generatedObjectCount == 3U,
                "radial facade command accepted") &&
         expect(selected.size() == 1U && selected.front().value == 4U &&
                    facade.selectionState().selectedTarget.value == 4U,
                "radial facade selects final instance only") &&
         expect(final != nullptr && near(final->bounds.min.x, -1.0) &&
                    near(final->bounds.max.x, 0.0) &&
                    near(final->bounds.min.z, 2.0) &&
                    near(final->bounds.max.z, 3.0),
                "radial facade publishes final rotated geometry") &&
         expect(statsAfter.commandAttempts == statsBefore.commandAttempts + 1U &&
                    statsAfter.commandSuccesses ==
                        statsBefore.commandSuccesses + 1U &&
                    statsAfter.commandFailures == statsBefore.commandFailures &&
                    statsAfter.objectsCreated == statsBefore.objectsCreated + 3U &&
                    statsAfter.roomsCreated == statsBefore.roomsCreated + 3U,
                "radial facade records one command and generated objects");
}

}  // namespace

int main() {
  const bool ok = defaultsExposeDefaultEditorState() &&
                  setActiveToolUpdatesCanonicalState() &&
                  selectPressUpdatesSelectionOnlyAndNotDocument() &&
                  movePressSelectsLikeSelect() &&
                  navigatePointerInputDoesNotTouchEditorState() &&
                  switchingToNavigateCancelsActiveMeasurement() &&
                  measureClickClickUpdatesMeasurement() &&
                  measurementAnnotationsAreExplicitDocumentMutations() &&
                  leavingMeasureCancelsActiveMeasurement() &&
                  sameMeasureToolActivationKeepsActiveMeasurement() &&
                  leavingMeasurePreservesCompletedMeasurement() &&
                  nonMeasureToolSwitchDoesNotTouchMeasurement() &&
                  pointerMoveUpdatesGhostWithSnap() &&
                  cancelInputHidesGhost() &&
                  toolSwitchHidesVisibleGhost() &&
                  switchingToNavigateHidesVisibleGhost() &&
                  sameToolActivationKeepsVisibleGhost() &&
                  hiddenGhostToolSwitchPreservesOtherEditorState() &&
                  pointerMoveAfterHideShowsGhostWithNewTool() &&
                  snapSettingsAffectLaterGhostDispatch() &&
                  unknownInputDoesNotChangeKernels() &&
                  roomCommandsStillWorkThroughFacade() &&
                  installingValidDocumentReplacesDocumentAndPreservesContentState() &&
                  saveAcknowledgementDrainsOnlyTheExactPublishedDocument() &&
                  installingInvalidIdDocumentDoesNotMutateExistingFacade() &&
                  installingDocumentClearsTransientEditorState() &&
                  installingSecondDocumentDoesNotLeakOldSelection() &&
                  resetAndFacadeCopiesPreserveRevisionLineage() &&
                  rejectedInstallDoesNotAdvanceRevisionLineage() &&
                  removingSelectedObjectClearsCanonicalState() &&
                  removingHoveredObjectPreservesSelection() &&
                  removingMissingObjectPreservesEditorTargets() &&
                  removingGhostTargetHidesGhost() &&
                  removingDifferentObjectPreservesGhost() &&
                  removingMeasurementTargetClearsMeasurement() &&
                  removingDifferentObjectPreservesMeasurement() &&
                  linearArrayCreatesCopiesAndSelectsOnlyFinalInstance() &&
                  radialArrayUsesPivotAndSelectsOnlyFinalInstance();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
