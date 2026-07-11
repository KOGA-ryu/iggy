#include "app/iggy3d/creative/tools/Volume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
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

cr::CreativeVolumeSelection selection(cr::CreativeGridCoord3 first,
                                      cr::CreativeGridCoord3 second,
                                      double cellSize = 1.0) {
  cr::CreativeVolumeSelection result;
  result.cellSize = cellSize;
  static_cast<void>(cr::advanceCreativeVolumeSelection(result, first));
  static_cast<void>(cr::advanceCreativeVolumeSelection(result, second));
  return result;
}

cr::CreativeVolumeOperationRequest request(
    cr::CreativeVolumeOperationKind operation,
    cr::CreativeVolumeSelection selected,
    cr::CreativeObjectKind kind = cr::CreativeObjectKind::Wall) {
  cr::CreativeVolumeOperationRequest result;
  result.operation = operation;
  result.selection = selected;
  result.objectKind = kind;
  return result;
}

cr::CreativeDocument document(std::string_view name) {
  cr::CreativeDocument result = cr::CreativeDocument::create(std::string{name});
  static_cast<void>(result.assignId(1U));
  return result;
}

bool hasVolumeTag(const cr::CreativeObject& object) {
  return std::find(object.tags.begin(), object.tags.end(),
                   cr::kCreativeVolumeCellTag) != object.tags.end();
}

bool canonicalSelectionAndPreview() {
  cr::CreativeVolumeSelection selected =
      selection({2, 0, 3}, {-1, 2, 1});
  const cr::CreativeGridBounds3 grid = cr::creativeVolumeGridBounds(selected);
  const cr::CreativeBounds world = cr::creativeVolumeWorldBounds(selected);

  cr::CreativeVolumeSelection inProgress;
  static_cast<void>(cr::advanceCreativeVolumeSelection(inProgress, {3, 0, 4}));
  const cr::CreativeVolumeSelection preview =
      cr::previewCreativeVolumeSelection(inProgress, {5, 0, 7});

  bool ok = expect(cr::creativeVolumeSelectionValid(selected),
                   "canonical selection valid") &&
            expect(grid.min.x == -1 && grid.min.y == 0 && grid.min.z == 1,
                   "canonical grid minimum") &&
            expect(grid.max.x == 3 && grid.max.y == 3 && grid.max.z == 4,
                   "canonical grid maximum exclusive") &&
            expect(cr::creativeVolumeCellCount(selected) == 36U,
                   "canonical cell count") &&
            expect(world.min.x == -1.0 && world.max.x == 3.0 &&
                       world.min.y == 0.0 && world.max.y == 3.0,
                   "canonical world bounds") &&
            expect(cr::creativeVolumeSelectionComplete(preview),
                   "cursor preview is complete") &&
            expect(cr::creativeVolumeCellCount(preview) == 12U,
                   "cursor preview cell count");
  ok = expect(cr::resizeCreativeVolumeSelectionHeight(selected, 1),
              "height grows") &&
       expect(cr::creativeVolumeCellCount(selected) == 48U,
              "height growth updates count") &&
       ok;
  return ok;
}

bool fillIsAtomicAndIdempotent() {
  cr::CreativeDocument document = ::document("fill");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {1, 1, 1});
  const cr::CreativeVolumeOperationRequest fill =
      request(cr::CreativeVolumeOperationKind::Fill, selected);

  const cr::CreativeVolumeOperationReceipt first =
      cr::executeCreativeVolumeOperation(document, fill);
  bool ok = expect(first.accepted && first.changed, "fill accepted") &&
            expect(first.createdObjectCount == 8U, "fill creates eight cells") &&
            expect(document.objectCount() == 8U, "fill document count") &&
            expect(std::all_of(document.objects().begin(), document.objects().end(),
                               [](const cr::CreativeObject& object) {
                                 return object.kind == cr::CreativeObjectKind::Wall &&
                                        hasVolumeTag(object);
                               }),
                   "fill objects carry kind and volume tag");

  const std::uint64_t revisionBeforeRepeat = document.revision();
  const cr::CreativeVolumeOperationReceipt repeat =
      cr::executeCreativeVolumeOperation(document, fill);
  ok = expect(repeat.accepted && !repeat.changed,
              "repeat fill accepted as no change") &&
       expect(repeat.skippedOccupiedCellCount == 8U,
              "repeat fill reports occupied cells") &&
       expect(document.objectCount() == 8U,
              "repeat fill creates no duplicates") &&
       expect(document.revision() == revisionBeforeRepeat,
              "repeat fill preserves revision") &&
       ok;
  return ok;
}

bool hollowCreatesOnlyShell() {
  cr::CreativeDocument document = ::document("hollow");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {2, 2, 2});
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Hollow, selected));

  const bool centerMissing = std::none_of(
      document.objects().begin(), document.objects().end(),
      [](const cr::CreativeObject& object) {
        return object.bounds.min.x == 1.0 && object.bounds.min.y == 1.0 &&
               object.bounds.min.z == 1.0;
      });
  bool ok = expect(receipt.accepted && receipt.changed, "hollow accepted") &&
            expect(receipt.plannedCellCount == 26U, "hollow shell count") &&
            expect(receipt.createdObjectCount == 26U,
                   "hollow creates shell") &&
            expect(centerMissing, "hollow center remains empty");

  cr::CreativeDocument solid = ::document("solid to hollow");
  const cr::CreativeVolumeOperationReceipt filled =
      cr::executeCreativeVolumeOperation(
          solid, request(cr::CreativeVolumeOperationKind::Fill, selected));
  const cr::CreativeVolumeOperationReceipt hollowed =
      cr::executeCreativeVolumeOperation(
          solid, request(cr::CreativeVolumeOperationKind::Hollow, selected));
  ok = expect(filled.accepted && filled.createdObjectCount == 27U,
              "solid setup fill") &&
       expect(hollowed.accepted && hollowed.changed,
              "solid converted to hollow") &&
       expect(hollowed.removedObjectCount == 1U,
              "hollow removes kernel-owned interior") &&
       expect(hollowed.createdObjectCount == 0U,
              "hollow reuses existing boundary") &&
       expect(solid.objectCount() == 26U,
              "solid to hollow final count") &&
       ok;
  return ok;
}

bool shapedFillAndHollowUseSharedPlanner() {
  const cr::CreativeVolumeSelection selected =
      selection({0, 0, 0}, {2, 2, 2});
  cr::CreativeVolumeOperationRequest ellipsoid =
      request(cr::CreativeVolumeOperationKind::Fill, selected);
  ellipsoid.shapeKind = cr::CreativeShapeBrushKind::Ellipsoid;

  cr::CreativeDocument document = ::document("ellipsoid");
  const cr::CreativeVolumeOperationReceipt filled =
      cr::executeCreativeVolumeOperation(document, ellipsoid);
  const bool cornersMissing = std::none_of(
      document.objects().begin(), document.objects().end(),
      [](const cr::CreativeObject& object) {
        return (object.bounds.min.x == 0.0 && object.bounds.min.y == 0.0 &&
                object.bounds.min.z == 0.0) ||
               (object.bounds.min.x == 2.0 && object.bounds.min.y == 2.0 &&
                object.bounds.min.z == 2.0);
      });
  bool ok = expect(filled.accepted && filled.changed,
                   "ellipsoid fill accepted") &&
            expect(filled.shapeKind == cr::CreativeShapeBrushKind::Ellipsoid &&
                       filled.shapeAxis == cr::CreativeShapeBrushAxis::Y,
                   "ellipsoid receipt records planner inputs") &&
            expect(filled.shapeCandidateCellCount == 27U &&
                       filled.plannedCellCount == 19U &&
                       filled.createdObjectCount == 19U,
                   "ellipsoid plan and mutation counts agree") &&
            expect(document.objectCount() == 19U && cornersMissing,
                   "ellipsoid document matches shared shape plan");

  ellipsoid.operation = cr::CreativeVolumeOperationKind::Hollow;
  const cr::CreativeVolumeOperationReceipt hollowed =
      cr::executeCreativeVolumeOperation(document, ellipsoid);
  ok = expect(hollowed.accepted && hollowed.changed,
              "ellipsoid hollow accepted") &&
       expect(hollowed.plannedCellCount == 18U &&
                  hollowed.removedObjectCount == 1U &&
                  hollowed.createdObjectCount == 0U,
              "ellipsoid hollow removes only planner interior") &&
       expect(document.objectCount() == 18U,
              "ellipsoid hollow final object count") &&
       ok;

  cr::CreativeDocument lineDocument = ::document("line");
  cr::CreativeVolumeOperationRequest line = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({-2, 1, 0}, {3, 3, 1}));
  line.shapeKind = cr::CreativeShapeBrushKind::Line;
  const cr::CreativeVolumeOperationReceipt lineReceipt =
      cr::executeCreativeVolumeOperation(lineDocument, line);
  ok = expect(lineReceipt.accepted && lineReceipt.createdObjectCount == 6U &&
                  lineReceipt.plannedCellCount == 6U,
              "line fill applies deterministic Bresenham plan") &&
       expect(lineDocument.objectCount() == 6U,
              "line document count matches plan") &&
       ok;

  cr::CreativeVolumeOperationRequest invalid = line;
  invalid.shapeKind = static_cast<cr::CreativeShapeBrushKind>(255U);
  const std::uint64_t revisionBefore = lineDocument.revision();
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(lineDocument, invalid);
  return expect(!rejected.accepted && !rejected.changed &&
                    rejected.status ==
                        cr::CreativeVolumeOperationStatus::InvalidRequest,
                "invalid shape request fails closed") &&
         expect(lineDocument.revision() == revisionBefore &&
                    lineDocument.objectCount() == 6U,
                "invalid shape leaves document unchanged") &&
         ok;
}

bool operationLimitRejectsWithoutMutation() {
  cr::CreativeDocument document = ::document("limit");
  cr::CreativeVolumeOperationRequest fill = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({0, 0, 0}, {2, 2, 2}));
  fill.maxAffectedObjects = 10U;
  const std::uint64_t revisionBefore = document.revision();

  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(document, fill);
  return expect(!receipt.accepted && !receipt.changed,
                "limit rejection not accepted") &&
         expect(receipt.status ==
                    cr::CreativeVolumeOperationStatus::OperationLimitExceeded,
                "limit status") &&
         expect(document.objectCount() == 0U, "limit leaves document empty") &&
         expect(document.revision() == revisionBefore,
                "limit preserves revision");
}

bool partialCreateFailureRollsBack() {
  cr::CreativeDocument document = ::document("id exhaustion");
  cr::CreativeDocumentRestoreRequest restore;
  restore.documentId = document.id();
  restore.name = "id exhaustion";
  restore.units = document.units();
  restore.gridSettings = document.gridSettings();
  restore.snapSettings = document.documentSnapSettings();
  restore.worldBounds = document.worldBounds();
  restore.nextObjectId = std::numeric_limits<cr::CreativeObjectId>::max();
  const cr::CreativeDocumentRestoreReceipt restored =
      document.restoreForLoad(restore);
  if (!restored.accepted) {
    return expect(false, "id exhaustion restore accepted");
  }

  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill,
                  selection({0, 0, 0}, {1, 0, 0})));
  return expect(!receipt.accepted && !receipt.changed,
                "partial create failure rejected") &&
         expect(receipt.status ==
                    cr::CreativeVolumeOperationStatus::CreateRejected,
                "partial create failure status") &&
         expect(document.objectCount() == 0U,
                "partial create failure rolls back first cell") &&
         expect(document.nextObjectId() ==
                    std::numeric_limits<cr::CreativeObjectId>::max(),
                "partial create failure preserves id cursor") &&
         expect(document.revision() == revisionBefore,
                "partial create failure preserves revision");
}

bool replaceTargetsOnlyVolumeCells() {
  cr::CreativeDocument document = ::document("replace");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {1, 0, 1});
  const cr::CreativeVolumeOperationReceipt fill =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill, selected));
  if (!fill.accepted) {
    return expect(false, "replace setup fill accepted");
  }

  cr::CreativeVolumeOperationRequest replace = request(
      cr::CreativeVolumeOperationKind::Replace, selected,
      cr::CreativeObjectKind::Floor);
  replace.hasReplaceKindFilter = true;
  replace.replaceKindFilter = cr::CreativeObjectKind::Wall;
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(document, replace);

  return expect(receipt.accepted && receipt.changed, "replace accepted") &&
         expect(receipt.matchedObjectCount == 4U, "replace matched count") &&
         expect(receipt.removedObjectCount == 4U, "replace removed count") &&
         expect(receipt.createdObjectCount == 4U, "replace created count") &&
         expect(document.objectCount() == 4U, "replace preserves count") &&
         expect(std::all_of(document.objects().begin(), document.objects().end(),
                            [](const cr::CreativeObject& object) {
                              return object.kind == cr::CreativeObjectKind::Floor &&
                                     hasVolumeTag(object);
                            }),
                "replace writes target kind");
}

bool eraseIsContainedAndRollbackSafe() {
  cr::CreativeDocument document = ::document("erase");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {1, 0, 1});
  const cr::CreativeVolumeOperationReceipt fill =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill, selected));
  if (!fill.accepted) {
    return expect(false, "erase setup fill accepted");
  }

  cr::CreativeDocumentCreateRequest largeFloor;
  largeFloor.kind = cr::CreativeObjectKind::Floor;
  largeFloor.name = "large floor";
  largeFloor.bounds = {{-10.0, 0.0, -10.0}, {10.0, 0.25, 10.0}};
  largeFloor.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt floorReceipt =
      document.createObject(largeFloor);
  if (!floorReceipt.accepted) {
    return expect(false, "erase large floor setup accepted");
  }

  cr::CreativeObject* locked = document.findObject(fill.createdObjectIds.front());
  locked->locked = true;
  const std::uint64_t countBeforeReject = document.objectCount();
  const std::uint64_t revisionBeforeReject = document.revision();
  const cr::CreativeVolumeOperationReceipt rejected =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Erase, selected));
  bool ok = expect(!rejected.accepted && !rejected.changed,
                   "locked erase rejected") &&
            expect(document.objectCount() == countBeforeReject,
                   "locked erase rolls back removals") &&
            expect(document.revision() == revisionBeforeReject,
                   "locked erase preserves revision");

  locked = document.findObject(fill.createdObjectIds.front());
  locked->locked = false;
  const cr::CreativeVolumeOperationReceipt erased =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Erase, selected));
  ok = expect(erased.accepted && erased.changed, "erase accepted") &&
       expect(erased.removedObjectCount == 4U, "erase removes contained cells") &&
       expect(document.objectCount() == 1U, "erase leaves spanning floor") &&
       expect(document.containsObject(floorReceipt.objectId),
              "erase preserves non-contained floor") &&
       ok;
  return ok;
}

bool eraseRejectsParentWithExternalChild() {
  cr::CreativeDocument document = ::document("erase parent");
  cr::CreativeDocumentCreateRequest parentRequest;
  parentRequest.kind = cr::CreativeObjectKind::Group;
  parentRequest.name = "parent";
  parentRequest.transform.position = {0.5, 0.5, 0.5};
  parentRequest.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt parent =
      document.createObject(parentRequest);

  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Wall;
  childRequest.name = "child outside";
  childRequest.bounds = {{3.0, 0.0, 0.0}, {4.0, 1.0, 1.0}};
  childRequest.hasBoundsOverride = true;
  const cr::CreativeDocumentCreateReceipt child =
      document.createObject(childRequest);
  if (!parent.accepted || !child.accepted) {
    return expect(false, "erase parent setup accepted");
  }
  document.findObject(child.objectId)->parentId = parent.objectId;

  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Erase,
                  selection({0, 0, 0}, {0, 0, 0})));
  return expect(!receipt.accepted && !receipt.changed,
                "external child erase rejected") &&
         expect(receipt.status ==
                    cr::CreativeVolumeOperationStatus::RemoveRejected,
                "external child erase status") &&
         expect(document.containsObject(parent.objectId) &&
                    document.containsObject(child.objectId),
                "external child erase preserves both objects") &&
         expect(document.revision() == revisionBefore,
                "external child erase preserves revision");
}

bool cloneUsesVolumeWidthOffset() {
  cr::CreativeDocument document = ::document("clone");
  const cr::CreativeVolumeSelection selected = selection({0, 0, 0}, {1, 0, 0});
  const cr::CreativeVolumeOperationReceipt fill =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill, selected));
  if (!fill.accepted) {
    return expect(false, "clone setup fill accepted");
  }

  const cr::CreativeVolumeOperationReceipt cloned =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Clone, selected));
  const bool shiftedCellsPresent =
      std::any_of(document.objects().begin(), document.objects().end(),
                  [](const cr::CreativeObject& object) {
                    return object.bounds.min.x == 2.0;
                  }) &&
      std::any_of(document.objects().begin(), document.objects().end(),
                  [](const cr::CreativeObject& object) {
                    return object.bounds.min.x == 3.0;
                  });

  return expect(cloned.accepted && cloned.changed, "clone accepted") &&
         expect(cloned.matchedObjectCount == 2U, "clone matched count") &&
         expect(cloned.createdObjectCount == 2U, "clone created count") &&
         expect(document.objectCount() == 4U, "clone document count") &&
         expect(shiftedCellsPresent, "clone shifts by volume width");
}

bool unsupportedBrushRejects() {
  cr::CreativeDocument document = ::document("unsupported");
  const cr::CreativeVolumeOperationReceipt receipt =
      cr::executeCreativeVolumeOperation(
          document,
          request(cr::CreativeVolumeOperationKind::Fill,
                  selection({0, 0, 0}, {0, 0, 0}),
                  cr::CreativeObjectKind::SpawnPoint));
  return expect(!receipt.accepted, "point brush rejected") &&
         expect(receipt.status ==
                    cr::CreativeVolumeOperationStatus::UnsupportedObjectKind,
                "point brush status") &&
         expect(document.objectCount() == 0U,
                "point brush leaves document unchanged");
}

bool volumeBrushCycleSkipsUnsupportedKinds() {
  constexpr std::array palette{
      cr::CreativeObjectKind::SpawnPoint,
      cr::CreativeObjectKind::Wall,
      cr::CreativeObjectKind::PatrolRoute,
      cr::CreativeObjectKind::Floor,
  };
  return expect(cr::firstCreativeVolumeBrush(palette) ==
                    cr::CreativeObjectKind::Wall,
                "volume brush first skips point") &&
         expect(cr::nextCreativeVolumeBrush(
                    palette, cr::CreativeObjectKind::Wall) ==
                    cr::CreativeObjectKind::Floor,
                "volume brush next skips path") &&
         expect(cr::nextCreativeVolumeBrush(
                    palette, cr::CreativeObjectKind::Floor) ==
                    cr::CreativeObjectKind::Wall,
                "volume brush wraps") &&
         expect(!cr::creativeVolumeBrushSupported(
                    cr::CreativeObjectKind::SpawnPoint),
                "point is not a volume brush");
}

bool toolSettingsMapAtomicallyToVolumeRequests() {
  cr::CreativeToolSettings settings =
      cr::makeDefaultCreativeToolSettings();
  cr::CreativeVolumeOperationRequest replace = request(
      cr::CreativeVolumeOperationKind::Replace,
      selection({0, 0, 0}, {1, 0, 0}));
  replace.hasReplaceKindFilter = true;
  replace.replaceKindFilter = cr::CreativeObjectKind::Wall;

  bool ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(replace,
                                                                 settings),
                   "replace any settings map") &&
            expect(!replace.hasReplaceKindFilter &&
                       replace.replaceKindFilter ==
                           cr::CreativeObjectKind::Unknown,
                   "replace any clears source filter");

  cr::CreativeVolumeOperationRequest fill = request(
      cr::CreativeVolumeOperationKind::Fill,
      selection({0, 0, 0}, {4, 2, 4}));
  settings.shapeBrushKind = cr::CreativeShapeBrushKind::Cylinder;
  settings.shapeBrushAxis = cr::CreativeShapeBrushAxis::Z;
  ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(fill, settings),
              "shape settings map to fill") &&
       expect(fill.shapeKind == cr::CreativeShapeBrushKind::Cylinder &&
                  fill.shapeAxis == cr::CreativeShapeBrushAxis::Z,
              "fill request owns shape and axis") &&
       ok;

  settings.replaceSourceKind = cr::CreativeObjectKind::Crate;
  ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(replace, settings),
              "replace source settings map") &&
       expect(replace.hasReplaceKindFilter &&
                  replace.replaceKindFilter == cr::CreativeObjectKind::Crate,
              "replace source becomes request filter") &&
       ok;

  cr::CreativeVolumeOperationRequest clone = request(
      cr::CreativeVolumeOperationKind::Clone,
      selection({0, 0, 0}, {0, 0, 0}, 0.5));
  settings.cloneOffsetAxis = cr::CreativeCloneOffsetAxis::Z;
  settings.cloneOffsetDistance =
      cr::CreativeCloneOffsetDistance::FourCells;
  ok = expect(cr::applyCreativeToolSettingsToVolumeRequest(clone, settings),
              "clone settings map") &&
       expect(clone.hasCloneOffset && clone.cloneOffset.x == 0.0 &&
                  clone.cloneOffset.y == 0.0 && clone.cloneOffset.z == 2.0,
              "clone request uses axis cells and selection scale") &&
       ok;

  cr::CreativeVolumeOperationRequest invalidScale = clone;
  invalidScale.selection.cellSize = 0.0;
  invalidScale.hasCloneOffset = false;
  invalidScale.cloneOffset = {3.0, 4.0, 5.0};
  const cr::CreativeVolumeOperationRequest beforeInvalidScale = invalidScale;
  ok = expect(!cr::applyCreativeToolSettingsToVolumeRequest(invalidScale,
                                                              settings),
              "invalid clone scale rejected") &&
       expect(invalidScale.hasCloneOffset ==
                  beforeInvalidScale.hasCloneOffset &&
                  invalidScale.cloneOffset.x ==
                      beforeInvalidScale.cloneOffset.x &&
                  invalidScale.cloneOffset.y ==
                      beforeInvalidScale.cloneOffset.y &&
                  invalidScale.cloneOffset.z ==
                      beforeInvalidScale.cloneOffset.z,
              "invalid clone scale leaves request unchanged") &&
       ok;

  cr::CreativeVolumeOperationRequest unchanged = clone;
  unchanged.hasCloneOffset = false;
  unchanged.cloneOffset = {7.0, 8.0, 9.0};
  const cr::CreativeVolumeOperationRequest before = unchanged;
  settings.cloneOffsetAxis = cr::CreativeCloneOffsetAxis::Count;
  ok = expect(!cr::applyCreativeToolSettingsToVolumeRequest(unchanged,
                                                              settings),
              "invalid settings rejected") &&
       expect(unchanged.hasCloneOffset == before.hasCloneOffset &&
                  unchanged.cloneOffset.x == before.cloneOffset.x &&
                  unchanged.cloneOffset.y == before.cloneOffset.y &&
                  unchanged.cloneOffset.z == before.cloneOffset.z &&
                  unchanged.maxAffectedObjects == before.maxAffectedObjects,
              "invalid settings leave request unchanged") &&
       ok;

  settings = cr::makeDefaultCreativeToolSettings();
  unchanged.operation = cr::CreativeVolumeOperationKind::Count;
  const cr::CreativeVolumeOperationRequest invalidOperation = unchanged;
  ok = expect(!cr::applyCreativeToolSettingsToVolumeRequest(unchanged,
                                                              settings),
              "invalid operation rejected") &&
       expect(unchanged.operation == invalidOperation.operation &&
                  unchanged.cloneOffset.x == invalidOperation.cloneOffset.x,
              "invalid operation leaves request unchanged") &&
       ok;
  return ok;
}

bool facadeSelectionAndHistoryRoundTrip() {
  cr::CreativeAppState appState;
  const cr::CreativeFacadeDocumentInstallReceipt install =
      appState.facade.installDocument(document("facade history"));
  if (!install.accepted) {
    return expect(false, "facade history document installed");
  }

  cr::CreativeDocumentHistoryTransaction transaction =
      cr::beginCreativeHistoryTransaction(appState.facade, "volume_fill");
  const cr::CreativeVolumeOperationReceipt fill =
      appState.facade.applyVolumeOperation(
          request(cr::CreativeVolumeOperationKind::Fill,
                  selection({0, 0, 0}, {1, 0, 0})));
  const cr::CreativeHistoryRecordReceipt recorded =
      cr::commitCreativeHistoryTransaction(
          appState.history, std::move(transaction), appState.facade);
  bool ok = expect(fill.accepted && fill.createdObjectCount == 2U,
                   "facade fill accepted") &&
            expect(cr::selectedTargetCount(appState.facade.selectionState()) ==
                       2U,
                   "facade selects created cells") &&
            expect(recorded.recorded, "volume fill records one undo step") &&
            expect(cr::creativeUndoDepth(appState.history) == 1U,
                   "volume fill undo depth one");

  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeHistoryApplyReceipt redo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Redo);
  ok = expect(undo.accepted && undo.objectCountAfter == 0U,
              "volume fill undo restores empty document") &&
       expect(redo.accepted && redo.objectCountAfter == 2U,
              "volume fill redo restores cells") &&
       ok;
  return ok;
}

bool directCornerSelectionAndExpansion() {
  cr::CreativeVolumeSelection selected;
  bool ok = expect(
                cr::setCreativeVolumeSelectionCorner(
                    selected, cr::CreativeVolumeCorner::First, {2, 3, 4}) ==
                    cr::CreativeVolumeSelectionPhase::FirstCorner,
                "direct first corner starts selection") &&
            expect(selected.firstCell.x == 2 && selected.firstCell.y == 3 &&
                       selected.firstCell.z == 4,
                   "direct first corner stored") &&
            expect(cr::setCreativeVolumeSelectionCorner(
                       selected, cr::CreativeVolumeCorner::Second,
                       {-1, 5, 6}) ==
                       cr::CreativeVolumeSelectionPhase::Complete,
                   "direct second corner completes selection") &&
            expect(cr::creativeVolumeCellCount(selected) == 36U,
                   "direct corners define canonical volume");

  ok = expect(cr::expandCreativeVolumeSelectionToCell(selected, {4, 1, 3}),
              "pick expansion grows selection") &&
       expect(selected.firstCell.x == -1 && selected.firstCell.y == 1 &&
                  selected.firstCell.z == 3 && selected.secondCell.x == 4 &&
                  selected.secondCell.y == 5 && selected.secondCell.z == 6,
              "pick expansion stores canonical inclusive corners") &&
       expect(!cr::expandCreativeVolumeSelectionToCell(selected, {0, 2, 4}),
              "contained pick does not mutate selection") &&
       ok;
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = canonicalSelectionAndPreview() && ok;
  ok = fillIsAtomicAndIdempotent() && ok;
  ok = hollowCreatesOnlyShell() && ok;
  ok = shapedFillAndHollowUseSharedPlanner() && ok;
  ok = operationLimitRejectsWithoutMutation() && ok;
  ok = partialCreateFailureRollsBack() && ok;
  ok = replaceTargetsOnlyVolumeCells() && ok;
  ok = eraseIsContainedAndRollbackSafe() && ok;
  ok = eraseRejectsParentWithExternalChild() && ok;
  ok = cloneUsesVolumeWidthOffset() && ok;
  ok = unsupportedBrushRejects() && ok;
  ok = volumeBrushCycleSkipsUnsupportedKinds() && ok;
  ok = toolSettingsMapAtomicallyToVolumeRequests() && ok;
  ok = facadeSelectionAndHistoryRoundTrip() && ok;
  ok = directCornerSelectionAndExpansion() && ok;
  return ok ? 0 : 1;
}
