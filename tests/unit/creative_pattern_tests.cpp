#include "app/iggy3d/creative/tools/Pattern.hpp"
#include "app/iggy3d/creative/tools/AssetScatter.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>

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

cr::CreativeDocument document(std::string_view name) {
  cr::CreativeDocument output = cr::CreativeDocument::create(std::string{name});
  static_cast<void>(output.assignId(1U));
  return output;
}

cr::CreativeObjectId createGroup(
    cr::CreativeDocument& document,
    std::string_view name,
    cr::CreativeVec3 position,
    std::optional<cr::CreativeObjectId> parentId = std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Group;
  request.name = std::string{name};
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.parentId = parentId;
  return document.createObject(request).objectId;
}

cr::CreativeObjectId createRoom(cr::CreativeDocument& document,
                                std::string_view name,
                                cr::CreativeBounds bounds) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Room;
  request.name = std::string{name};
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  return document.createObject(request).objectId;
}

bool planUsesOrdinalOffsetsWithoutAccumulation() {
  cr::CreativeLinearArrayPlanRequest request;
  request.sourceObjectCount = 2U;
  request.direction = cr::CreativeLinearArrayDirection::NegativeZ;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Four;
  request.spacing = cr::CreativeLinearArraySpacing::TwoCells;
  request.cellSize = 0.5;
  const cr::CreativeLinearArrayPlanReceipt plan =
      cr::planCreativeLinearArray(request);

  bool ok = expect(plan.accepted, "array plan accepted") &&
            expect(plan.status == cr::CreativeLinearArrayStatus::Planned,
                   "array plan status") &&
            expect(plan.copyCount == 4U && plan.generatedObjectCount == 8U,
                   "array plan counts") &&
            expect(plan.instanceCount == 4U,
                   "array plan fixed instance count") &&
            expect(plan.instances[0].ordinal == 1U &&
                       plan.instances[0].offset.z == -1.0 &&
                       plan.instances[3].ordinal == 4U &&
                       plan.instances[3].offset.z == -4.0,
                   "array plan signed spacing offsets") &&
            expect(plan.instances[0].offset.x == 0.0 &&
                       plan.instances[0].offset.y == 0.0,
                   "array plan affects one axis");

  request.sourceObjectCount = 1U;
  request.direction = cr::CreativeLinearArrayDirection::PositiveX;
  request.copyCount = cr::CreativeLinearArrayCopyCount::ThirtyTwo;
  request.spacing = cr::CreativeLinearArraySpacing::OneCell;
  request.cellSize = 0.1;
  const cr::CreativeLinearArrayPlanReceipt longPlan =
      cr::planCreativeLinearArray(request);
  ok = expect(longPlan.accepted && longPlan.instanceCount == 32U,
              "long array plan accepted") &&
       expect(longPlan.instances[31].offset.x == 32.0 * 0.1,
              "last offset derives from ordinal") &&
       expect(cr::creativeLinearArrayCopyCountValue(
                  cr::CreativeLinearArrayCopyCount::ThirtyTwo) == 32U &&
                  cr::creativeLinearArraySpacingCells(
                      cr::CreativeLinearArraySpacing::EightCells) == 8U,
              "array option values stable") &&
       ok;
  return ok;
}

bool planRejectsInvalidAndOversizedRequests() {
  cr::CreativeLinearArrayPlanRequest request;
  const cr::CreativeLinearArrayPlanReceipt empty =
      cr::planCreativeLinearArray(request);

  request.sourceObjectCount = 1U;
  request.cellSize = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeLinearArrayPlanReceipt invalid =
      cr::planCreativeLinearArray(request);

  request.sourceObjectCount = 20U;
  request.cellSize = 1.0;
  request.copyCount = cr::CreativeLinearArrayCopyCount::ThirtyTwo;
  const cr::CreativeLinearArrayPlanReceipt oversized =
      cr::planCreativeLinearArray(request);

  request.sourceObjectCount = 1U;
  request.copyCount = cr::CreativeLinearArrayCopyCount::One;
  request.maxGeneratedObjects =
      cr::kCreativeLinearArrayGeneratedObjectCapacity + 1U;
  const cr::CreativeLinearArrayPlanReceipt raisedHardLimit =
      cr::planCreativeLinearArray(request);

  return expect(!empty.accepted &&
                    empty.status ==
                        cr::CreativeLinearArrayStatus::EmptySelection,
                "empty plan rejected") &&
         expect(!invalid.accepted &&
                    invalid.status ==
                        cr::CreativeLinearArrayStatus::InvalidRequest,
                "nonfinite plan rejected") &&
         expect(!oversized.accepted &&
                    oversized.status ==
                        cr::CreativeLinearArrayStatus::OperationLimitExceeded,
                "oversized plan rejected") &&
         expect(!raisedHardLimit.accepted &&
                    raisedHardLimit.status ==
                        cr::CreativeLinearArrayStatus::InvalidRequest,
                "hard limit cannot be raised by caller");
}

bool radialPlanPinsClosedAndPartialSweepLaws() {
  cr::CreativeRadialArrayPlanRequest request;
  request.sourceObjectCount = 2U;
  request.pivot = {4.0, 1.0, -2.0};
  request.axis = cr::CreativeAxis3::Y;
  request.instanceCount = cr::CreativeRadialArrayInstanceCount::Four;
  request.sweep = cr::CreativeRadialArraySweep::Degrees360;
  const cr::CreativeRadialArrayPlanReceipt closed =
      cr::planCreativeRadialArray(request);

  bool ok = expect(closed.accepted &&
                       closed.status == cr::CreativeRadialArrayStatus::Planned,
                   "closed radial plan accepted") &&
            expect(closed.totalInstanceCount == 4U &&
                       closed.generatedCopyCount == 3U &&
                       closed.generatedObjectCount == 6U &&
                       closed.instanceCount == 3U,
                   "radial counts include original exactly once") &&
            expect(near(closed.instances[0].angleRadians,
                        std::numbers::pi * 0.5) &&
                       near(closed.instances[2].angleRadians,
                            std::numbers::pi * 1.5) &&
                       closed.instances[2].angleRadians <
                           std::numbers::pi * 2.0,
                   "closed ring omits duplicate 360 endpoint");

  request.sweep = cr::CreativeRadialArraySweep::Degrees180;
  const cr::CreativeRadialArrayPlanReceipt partial =
      cr::planCreativeRadialArray(request);
  ok = expect(partial.accepted && partial.instanceCount == 3U,
              "partial radial plan accepted") &&
       expect(near(partial.instances[0].angleRadians,
                   std::numbers::pi / 3.0) &&
                  near(partial.instances[2].angleRadians, std::numbers::pi),
              "partial sweep spaces copies and includes endpoint") &&
       expect(cr::creativeRadialArrayInstanceCountValue(
                  cr::CreativeRadialArrayInstanceCount::ThirtyTwo) == 32U &&
                  cr::creativeRadialArraySweepDegrees(
                      cr::CreativeRadialArraySweep::Degrees90) == 90.0,
              "radial option values remain explicit") &&
       ok;
  return ok;
}

bool radialPlanRejectsInvalidAndOversizedRequests() {
  cr::CreativeRadialArrayPlanRequest request;
  const cr::CreativeRadialArrayPlanReceipt empty =
      cr::planCreativeRadialArray(request);
  request.sourceObjectCount = 1U;
  request.pivot.x = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeRadialArrayPlanReceipt nonfinite =
      cr::planCreativeRadialArray(request);
  request.pivot = {};
  request.axis = static_cast<cr::CreativeAxis3>(255U);
  const cr::CreativeRadialArrayPlanReceipt invalidAxis =
      cr::planCreativeRadialArray(request);
  request.axis = cr::CreativeAxis3::Y;
  request.sourceObjectCount = 17U;
  request.instanceCount = cr::CreativeRadialArrayInstanceCount::ThirtyTwo;
  const cr::CreativeRadialArrayPlanReceipt oversized =
      cr::planCreativeRadialArray(request);
  request.sourceObjectCount = 1U;
  request.instanceCount = cr::CreativeRadialArrayInstanceCount::Two;
  request.maxGeneratedObjects =
      cr::kCreativeRadialArrayGeneratedObjectCapacity + 1U;
  const cr::CreativeRadialArrayPlanReceipt raisedHardLimit =
      cr::planCreativeRadialArray(request);

  return expect(!empty.accepted &&
                    empty.status ==
                        cr::CreativeRadialArrayStatus::EmptySelection,
                "empty radial plan rejected") &&
         expect(!nonfinite.accepted &&
                    nonfinite.status ==
                        cr::CreativeRadialArrayStatus::InvalidRequest,
                "nonfinite radial pivot rejected") &&
         expect(!invalidAxis.accepted &&
                    invalidAxis.status ==
                        cr::CreativeRadialArrayStatus::InvalidRequest,
                "invalid radial axis rejected") &&
         expect(!oversized.accepted &&
                    oversized.status ==
                        cr::CreativeRadialArrayStatus::OperationLimitExceeded,
                "oversized radial plan rejected") &&
         expect(!raisedHardLimit.accepted &&
                    raisedHardLimit.status ==
                        cr::CreativeRadialArrayStatus::InvalidRequest,
                "radial hard limit cannot be raised");
}

bool axisAngleClipboardPasteRotatesRigidly() {
  cr::CreativeDocument document = ::document("axis angle paste");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {0.0, 2.0, 0.0});
  cr::CreativeClipboard clipboard;
  const std::array selected{source};
  if (!cr::copyDocumentObjectsToClipboard(document, selected, clipboard)
           .accepted) {
    return expect(false, "axis-angle source copied");
  }
  cr::CreativeClipboardPasteRequest request;
  request.offset = {};
  request.hasTransformAnchor = true;
  request.transformAnchor = {};
  request.hasAxisAngleRotation = true;
  request.rotationAxis = cr::CreativeAxis3::X;
  request.rotationRadians = std::numbers::pi * 0.5;
  const cr::CreativeClipboardPasteReceipt receipt =
      cr::pasteCreativeClipboardAtomically(document, clipboard, request);
  const cr::CreativeObject* pasted = receipt.pastedObjectIds.empty()
                                         ? nullptr
                                         : document.findObject(
                                               receipt.pastedObjectIds.front());
  const bool xRotationMatches =
      receipt.accepted && pasted != nullptr &&
      near(pasted->transform.position.x, 0.0) &&
      near(pasted->transform.position.y, 0.0) &&
      near(pasted->transform.position.z, 2.0) &&
      near(pasted->transform.rotationEulerRadians.x,
           std::numbers::pi * 0.5) &&
      near(pasted->transform.rotationEulerRadians.y, 0.0) &&
      near(pasted->transform.rotationEulerRadians.z, 0.0);
  request.rotationAxis = cr::CreativeAxis3::Z;
  const cr::CreativeClipboardPasteReceipt zReceipt =
      cr::pasteCreativeClipboardAtomically(document, clipboard, request);
  const cr::CreativeObject* zPasted = zReceipt.pastedObjectIds.empty()
                                          ? nullptr
                                          : document.findObject(
                                                zReceipt.pastedObjectIds.front());
  const bool zRotationMatches =
      zReceipt.accepted && zPasted != nullptr &&
      near(zPasted->transform.position.x, -2.0) &&
      near(zPasted->transform.position.y, 0.0) &&
      near(zPasted->transform.position.z, 0.0) &&
      near(zPasted->transform.rotationEulerRadians.z,
           std::numbers::pi * 0.5);
  request.mirrorX = true;
  const cr::CreativeClipboardPasteReceipt mirroredRotation =
      cr::pasteCreativeClipboardAtomically(document, clipboard, request);
  const std::size_t countBeforeInvalid = document.objectCount();
  const std::uint64_t revisionBeforeInvalid = document.revision();
  request.quarterTurns = 1U;
  const cr::CreativeClipboardPasteReceipt mixedRotation =
      cr::pasteCreativeClipboardAtomically(document, clipboard, request);
  return expect(xRotationMatches,
                "axis-angle paste revolves and orients around X") &&
         expect(zRotationMatches,
                "axis-angle paste supports Z-axis rigid rotation") &&
         expect(mirroredRotation.accepted,
                "axis-angle paste composes with reflection") &&
         expect(!mixedRotation.accepted &&
                    document.objectCount() == countBeforeInvalid &&
                    document.revision() == revisionBeforeInvalid,
                "mixed legacy and axis-angle rotation fails atomically");
}

bool batchPasteRemapsEachCopyIndependently() {
  cr::CreativeDocument document = ::document("batch remap");
  const cr::CreativeObjectId parent =
      createGroup(document, "Parent", {0.0, 0.0, 0.0});
  const cr::CreativeObjectId child =
      createGroup(document, "Child", {0.0, 1.0, 0.0}, parent);
  cr::CreativeClipboard clipboard;
  const std::array selected{child, parent};
  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(document, selected, clipboard);
  if (!copied.accepted) {
    return expect(false, "batch remap source copied");
  }

  std::array<cr::CreativeClipboardPasteRequest, 2> requests{};
  requests[0].offset = {2.0, 0.0, 0.0};
  requests[0].externalParentPolicy =
      cr::CreativeClipboardExternalParentPolicy::PreserveIfPresent;
  requests[1].offset = {0.0, 0.0, -3.0};
  requests[1].externalParentPolicy =
      cr::CreativeClipboardExternalParentPolicy::PreserveIfPresent;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeClipboardBatchPasteReceipt receipt =
      cr::pasteCreativeClipboardBatchAtomically(document, clipboard, requests);

  const cr::CreativeObject* firstParent = document.findObject(3U);
  const cr::CreativeObject* firstChild = document.findObject(4U);
  const cr::CreativeObject* secondParent = document.findObject(5U);
  const cr::CreativeObject* secondChild = document.findObject(6U);
  return expect(receipt.accepted && receipt.changed,
                "batch paste accepted") &&
         expect(receipt.requestedPasteCount == 2U &&
                    receipt.pastedPasteCount == 2U &&
                    receipt.pastedObjectCount == 4U,
                "batch paste counts") &&
         expect(receipt.pastedObjectIds ==
                    std::vector<cr::CreativeObjectId>{3U, 4U, 5U, 6U},
                "batch paste output is copy-major parent-first") &&
         expect(firstParent != nullptr && firstChild != nullptr &&
                    firstChild->parentId == firstParent->id &&
                    firstParent->transform.position.x == 2.0 &&
                    firstChild->transform.position.x == 2.0,
                "first copy remaps parent and offset") &&
         expect(secondParent != nullptr && secondChild != nullptr &&
                    secondChild->parentId == secondParent->id &&
                    secondParent->transform.position.z == -3.0 &&
                    secondChild->transform.position.z == -3.0,
                "second copy has independent parent remap") &&
         expect(document.revision() == revisionBefore + 1U &&
                    receipt.revisionBefore == revisionBefore &&
                    receipt.revisionAfter == revisionBefore + 1U,
                "batch publishes all generated revisions together");
}

bool copyDerivesDeterministicPlacementAnchor() {
  cr::CreativeDocument document = ::document("clipboard anchor");
  const cr::CreativeObjectId group =
      createGroup(document, "Group", {3.0, 5.0, -2.0});
  const cr::CreativeObjectId room = createRoom(
      document, "Room", {{-1.0, 7.0, 4.0}, {1.0, 9.0, 6.0}});
  const std::array selected{room, group};
  cr::CreativeClipboard clipboard;

  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(document, selected, clipboard);
  bool ok = expect(copied.accepted && clipboard.hasPlacementAnchor,
                   "clipboard copy records placement anchor") &&
            expect(clipboard.placementAnchor.x == 1.0 &&
                       clipboard.placementAnchor.y == 5.0 &&
                       clipboard.placementAnchor.z == 2.0,
                   "clipboard anchor is lower center of selection extent") &&
            expect(clipboard.objects.size() == 2U,
                   "clipboard anchor does not alter copied objects");
  cr::clearCreativeClipboard(clipboard);
  ok = expect(cr::creativeClipboardEmpty(clipboard) &&
                  !clipboard.hasPlacementAnchor,
              "clearing clipboard clears placement anchor") &&
       ok;
  return ok;
}

bool singlePasteRetainsTheExistingReceiptContract() {
  cr::CreativeDocument document = ::document("single paste");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {1.0, 2.0, 3.0});
  cr::CreativeClipboard clipboard;
  const std::array selected{source};
  if (!cr::copyDocumentObjectsToClipboard(document, selected, clipboard)
           .accepted) {
    return expect(false, "single paste source copied");
  }

  cr::CreativeClipboardPasteRequest request;
  request.offset = {4.0, 0.0, 0.0};
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeClipboardPasteReceipt receipt =
      cr::pasteCreativeClipboardAtomically(document, clipboard, request);
  const cr::CreativeObject* pasted = receipt.pastedObjectIds.empty()
                                         ? nullptr
                                         : document.findObject(
                                               receipt.pastedObjectIds[0]);
  return expect(receipt.accepted && receipt.changed &&
                    receipt.status == cr::CreativeClipboardStatus::Pasted,
                "single paste remains accepted") &&
         expect(receipt.requestedObjectCount == 1U &&
                    receipt.pastedObjectCount == 1U &&
                    receipt.idRemaps.size() == 1U &&
                    receipt.pastedObjectIds.size() == 1U,
                "single paste receipt shape is preserved") &&
         expect(pasted != nullptr && pasted->transform.position.x == 5.0,
                "single paste still applies its offset") &&
         expect(document.revision() == revisionBefore + 1U &&
                    receipt.reasonCode == "creative_clipboard_pasted",
                "single paste revision and reason remain stable");
}

bool transformedPasteMatchesTheSharedPlacementPlan() {
  cr::CreativeDocument document = ::document("transformed paste");
  const cr::CreativeObjectId source = createRoom(
      document, "Source", {{0.0, 0.0, 0.0}, {2.0, 1.0, 1.0}});
  cr::CreativeClipboard clipboard;
  const std::array selected{source};
  if (!cr::copyDocumentObjectsToClipboard(document, selected, clipboard)
           .accepted) {
    return expect(false, "transformed paste source copied");
  }

  cr::CreativeClipboardPasteRequest request;
  request.offset = {5.0, 0.0, 7.0};
  request.scaleFactor = {2.0, 0.5, 1.5};
  request.quarterTurns = 1U;
  request.mirrorX = true;
  cr::CreativeSelectionPlacementRequest planRequest;
  planRequest.mode = cr::CreativeSelectionPlacementMode::Copy;
  planRequest.sourceAnchor = clipboard.placementAnchor;
  planRequest.targetAnchor = {clipboard.placementAnchor.x + request.offset.x,
                              clipboard.placementAnchor.y + request.offset.y,
                              clipboard.placementAnchor.z + request.offset.z};
  planRequest.scaleFactor = request.scaleFactor;
  planRequest.quarterTurns = request.quarterTurns;
  planRequest.mirrorX = request.mirrorX;
  const cr::CreativeSelectionPlacementPlan plan =
      cr::planCreativeSelectionPlacement(clipboard.objects, planRequest);
  const cr::CreativeClipboardPasteReceipt receipt =
      cr::pasteCreativeClipboardAtomically(document, clipboard, request);
  const cr::CreativeObject* pasted = receipt.pastedObjectIds.empty()
                                         ? nullptr
                                         : document.findObject(
                                               receipt.pastedObjectIds.front());
  cr::CreativeClipboardPasteRequest invalidScale = request;
  invalidScale.scaleFactor.x = 0.0;
  const std::uint64_t revisionBeforeRejectedScale = document.revision();
  const cr::CreativeClipboardPasteReceipt rejectedScale =
      cr::pasteCreativeClipboardAtomically(document, clipboard, invalidScale);

  return expect(plan.accepted && receipt.accepted && pasted != nullptr,
                "transformed paste accepted") &&
         expect(pasted != nullptr && plan.objects.size() == 1U &&
                    pasted->bounds.min.x == plan.objects[0].bounds.min.x &&
                    pasted->bounds.min.z == plan.objects[0].bounds.min.z &&
                    pasted->bounds.max.x == plan.objects[0].bounds.max.x &&
                    pasted->bounds.max.z == plan.objects[0].bounds.max.z,
                "paste commit consumes exact planned bounds") &&
         expect(pasted != nullptr && pasted->name == "Source Copy",
                "transformed paste retains clipboard naming contract") &&
         expect(!rejectedScale.accepted && !rejectedScale.changed &&
                    rejectedScale.reasonCode ==
                        "creative_clipboard_scale_invalid" &&
                    document.revision() == revisionBeforeRejectedScale,
                "non-positive clipboard scale fails before mutation");
}

bool lateBatchFailureRollsBackEarlierStagedCopy() {
  cr::CreativeDocument document = ::document("batch rollback");
  const double huge = std::numeric_limits<double>::max() * 0.75;
  const cr::CreativeObjectId source =
      createGroup(document, "Huge", {huge, 0.0, 0.0});
  cr::CreativeClipboard clipboard;
  const std::array selected{source};
  if (!cr::copyDocumentObjectsToClipboard(document, selected, clipboard)
           .accepted) {
    return expect(false, "batch rollback source copied");
  }

  std::array<cr::CreativeClipboardPasteRequest, 2> requests{};
  requests[0].offset = {0.0, 0.0, 0.0};
  requests[1].offset = {std::numeric_limits<double>::max(), 0.0, 0.0};
  const std::uint64_t revisionBefore = document.revision();
  const std::uint64_t countBefore = document.objectCount();
  const cr::CreativeClipboardBatchPasteReceipt receipt =
      cr::pasteCreativeClipboardBatchAtomically(document, clipboard, requests);

  return expect(!receipt.accepted && !receipt.changed,
                "late batch failure rejected") &&
         expect(receipt.status == cr::CreativeClipboardStatus::InvalidRequest &&
                    receipt.failedPasteIndex == 1U,
                "late batch failure identifies second paste") &&
         expect(receipt.pastedPasteCount == 0U &&
                    receipt.pastedObjectIds.empty() &&
                    receipt.idRemaps.empty(),
                "late batch failure publishes no output facts") &&
         expect(document.objectCount() == countBefore &&
                    document.revision() == revisionBefore,
                "late batch failure restores original document");
}

bool arrayExecutionCreatesCopiesAndIdentifiesFinalGroup() {
  cr::CreativeDocument document = ::document("array execution");
  const cr::CreativeObjectId parent =
      createGroup(document, "Parent", {0.0, 0.0, 0.0});
  const cr::CreativeObjectId child =
      createGroup(document, "Child", {0.0, 1.0, 0.0}, parent);
  const std::array selected{parent, child};
  cr::CreativeLinearArrayRequest request;
  request.direction = cr::CreativeLinearArrayDirection::PositiveX;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  request.spacing = cr::CreativeLinearArraySpacing::TwoCells;
  request.cellSize = 0.5;

  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeLinearArrayReceipt receipt =
      cr::createCreativeLinearArrayAtomically(document, selected, request);
  const std::span<const cr::CreativeObjectId> finalCopy =
      receipt.finalCopyObjectIds();
  const cr::CreativeObject* finalParent =
      finalCopy.empty() ? nullptr : document.findObject(finalCopy[0]);
  const cr::CreativeObject* finalChild =
      finalCopy.size() < 2U ? nullptr : document.findObject(finalCopy[1]);
  const cr::CreativePatternRecipe* recipe =
      cr::findCreativePatternRecipe(document.patternRecipeStore(),
                                    receipt.patternRecipeId);

  return expect(receipt.accepted && receipt.changed &&
                    receipt.status == cr::CreativeLinearArrayStatus::Applied,
                "array execution accepted") &&
         expect(receipt.plan.status == cr::CreativeLinearArrayStatus::Planned &&
                    receipt.generatedObjectCount == 4U &&
                    receipt.generatedObjectIds().size() == 4U,
                "array execution plan and generated counts") &&
         expect(finalCopy.size() == 2U && finalCopy[0] == 5U &&
                    finalCopy[1] == 6U,
                "array execution identifies final copy only") &&
         expect(finalParent != nullptr && finalChild != nullptr &&
                    finalParent->transform.position.x == 2.0 &&
                    finalChild->transform.position.x == 2.0 &&
                    finalChild->parentId == finalParent->id,
                "array execution final copy uses ordinal offset and remap") &&
         expect(document.objectCount() == 6U &&
                    document.revision() == revisionBefore + 1U,
                "array execution records copies and one relationship") &&
         expect(receipt.revisionBefore == revisionBefore &&
                    receipt.revisionAfter == revisionBefore + 1U &&
                    receipt.pasteReceipt.revisionBefore == revisionBefore &&
                    receipt.pasteReceipt.revisionAfter ==
                        revisionBefore + 1U &&
                    receipt.patternMutationReceipt.revisionBefore ==
                        revisionBefore &&
                    receipt.patternMutationReceipt.revisionAfter ==
                        revisionBefore + 1U,
                "array nested receipts use one live revision range") &&
         expect(recipe != nullptr &&
                    recipe->kind == cr::CreativePatternRecipeKind::LinearArray &&
                    recipe->sourceObjectIds ==
                        std::vector<cr::CreativeObjectId>{1U, 2U} &&
                    recipe->generatedObjectIds ==
                        std::vector<cr::CreativeObjectId>{3U, 4U, 5U, 6U} &&
                    recipe->linear == request,
                "array execution retains source ids outputs and parameters");
}

bool arrayExecutionRejectsMissingSourceWithoutMutation() {
  cr::CreativeDocument document = ::document("array missing");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {0.0, 0.0, 0.0});
  const std::array selected{source, cr::CreativeObjectId{999U}};
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeLinearArrayReceipt receipt =
      cr::createCreativeLinearArrayAtomically(document, selected, {});
  return expect(!receipt.accepted && !receipt.changed,
                "missing array source rejected") &&
         expect(receipt.status == cr::CreativeLinearArrayStatus::MissingObject &&
                    receipt.failedObjectId == 999U,
                "missing array source reported") &&
         expect(document.objectCount() == 1U &&
                    document.revision() == revisionBefore,
                "missing array source preserves document");
}

bool radialExecutionRotatesGroupAndRemapsParents() {
  cr::CreativeDocument document = ::document("radial execution");
  const cr::CreativeObjectId parent =
      createGroup(document, "Parent", {2.0, 0.0, 0.0});
  const cr::CreativeObjectId child =
      createGroup(document, "Child", {2.0, 1.0, 0.0}, parent);
  const std::array selected{parent, child};
  cr::CreativeRadialArrayRequest request;
  request.pivot = {};
  request.axis = cr::CreativeAxis3::Y;
  request.instanceCount = cr::CreativeRadialArrayInstanceCount::Four;
  request.sweep = cr::CreativeRadialArraySweep::Degrees360;

  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeRadialArrayReceipt receipt =
      cr::createCreativeRadialArrayAtomically(document, selected, request);
  const std::span<const cr::CreativeObjectId> finalCopy =
      receipt.finalCopyObjectIds();
  const cr::CreativeObject* finalParent =
      finalCopy.empty() ? nullptr : document.findObject(finalCopy[0]);
  const cr::CreativeObject* finalChild =
      finalCopy.size() < 2U ? nullptr : document.findObject(finalCopy[1]);
  const cr::CreativePatternRecipe* recipe =
      cr::findCreativePatternRecipe(document.patternRecipeStore(),
                                    receipt.patternRecipeId);

  return expect(receipt.accepted && receipt.changed &&
                    receipt.status == cr::CreativeRadialArrayStatus::Applied,
                "radial execution accepted") &&
         expect(receipt.generatedObjectCount == 6U &&
                    receipt.generatedObjectIds().size() == 6U &&
                    finalCopy.size() == 2U && finalCopy[0] == 7U &&
                    finalCopy[1] == 8U,
                "radial execution identifies final generated group") &&
         expect(finalParent != nullptr && finalChild != nullptr &&
                    near(finalParent->transform.position.x, 0.0) &&
                    near(finalParent->transform.position.z, 2.0) &&
                    near(finalParent->transform.rotationEulerRadians.y,
                         -std::numbers::pi * 0.5) &&
                    near(finalChild->transform.position.x, 0.0) &&
                    near(finalChild->transform.position.y, 1.0) &&
                    near(finalChild->transform.position.z, 2.0) &&
                    finalChild->parentId == finalParent->id,
                "radial execution rotates rigid group and remaps parent") &&
         expect(document.objectCount() == 8U &&
                    document.revision() == revisionBefore + 1U,
                "radial execution publishes copies and relationship") &&
         expect(receipt.revisionBefore == revisionBefore &&
                    receipt.revisionAfter == revisionBefore + 1U &&
                    receipt.pasteReceipt.revisionBefore == revisionBefore &&
                    receipt.pasteReceipt.revisionAfter ==
                        revisionBefore + 1U &&
                    receipt.patternMutationReceipt.revisionBefore ==
                        revisionBefore &&
                    receipt.patternMutationReceipt.revisionAfter ==
                        revisionBefore + 1U,
                "radial nested receipts use one live revision range") &&
         expect(recipe != nullptr &&
                    recipe->kind == cr::CreativePatternRecipeKind::RadialArray &&
                    recipe->sourceObjectIds ==
                        std::vector<cr::CreativeObjectId>{1U, 2U} &&
                    recipe->generatedObjectIds.size() == 6U &&
                    recipe->radial == request,
                "radial execution retains source ids outputs and parameters");
}

bool radialExecutionRejectsDegeneratePivotWithoutMutation() {
  cr::CreativeDocument document = ::document("radial degenerate");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {0.0, 0.0, 0.0});
  const std::array selected{source};
  cr::CreativeRadialArrayRequest request;
  request.pivot = {};
  request.axis = cr::CreativeAxis3::Y;
  request.instanceCount = cr::CreativeRadialArrayInstanceCount::Four;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeRadialArrayReceipt receipt =
      cr::createCreativeRadialArrayAtomically(document, selected, request);
  return expect(!receipt.accepted && !receipt.changed &&
                    receipt.status ==
                        cr::CreativeRadialArrayStatus::DegenerateRadius,
                "degenerate radial pivot rejected explicitly") &&
         expect(document.objectCount() == 1U &&
                    document.revision() == revisionBefore &&
                    receipt.generatedObjectIds().empty(),
                "degenerate radial request leaves document unchanged");
}

bool patternRecipeRejectsMissingReferencesAtomically() {
  cr::CreativeDocument document = ::document("invalid pattern refs");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {0.0, 0.0, 0.0});
  cr::CreativePatternRecipeMutationRequest request;
  request.kind = cr::CreativePatternRecipeMutationKind::Add;
  request.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  request.recipe.sourceObjectIds = {source};
  request.recipe.generatedObjectIds = {999U};
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativePatternRecipeMutationReceipt receipt =
      document.applyPatternRecipeMutation(request);
  return expect(!receipt.accepted && !receipt.changed &&
                    receipt.status ==
                        cr::CreativePatternRecipeMutationStatus::InvalidRequest,
                "pattern recipe rejects missing output") &&
         expect(document.patternRecipeStore().recipes.empty() &&
                    document.revision() == revisionBefore,
                "invalid pattern recipe leaves document unchanged");
}

bool deletingPatternMemberDetachesRelationship() {
  cr::CreativeDocument document = ::document("pattern member delete");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {0.0, 0.0, 0.0});
  cr::CreativeLinearArrayRequest request;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  const cr::CreativeLinearArrayReceipt array =
      cr::createCreativeLinearArrayAtomically(
          document, std::span{&source, 1U}, request);
  if (!expect(array.accepted && array.generatedObjectIds().size() == 2U,
              "pattern member delete setup")) {
    return false;
  }
  const cr::CreativeObjectId removedId = array.generatedObjectIds().front();
  const cr::CreativeObjectId survivorId = array.generatedObjectIds().back();
  const cr::CreativeDocumentRemoveReceipt removed =
      document.removeDocumentObject(removedId);
  return expect(removed.accepted && removed.objectRemoved &&
                    removed.detachedPatternRecipeCount == 1U,
                "deleting a generated member detaches its recipe") &&
         expect(document.patternRecipeStore().recipes.empty() &&
                    document.findObject(source) != nullptr &&
                    document.findObject(survivorId) != nullptr,
                "detachment preserves source and surviving output") &&
         expect(document.isValid(),
                "member deletion leaves valid independent geometry");
}

bool semanticClipboardPreservesEditablePatternAcrossDocuments() {
  cr::CreativeDocument sourceDocument = ::document("semantic clipboard source");
  const cr::CreativeObjectId source =
      createGroup(sourceDocument, "Source", {0.0, 0.0, 0.0});
  cr::CreativeLinearArrayRequest request;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  request.spacing = cr::CreativeLinearArraySpacing::TwoCells;
  const cr::CreativeLinearArrayReceipt created =
      cr::createCreativeLinearArrayAtomically(
          sourceDocument, std::span{&source, 1U}, request);
  if (!expect(created.accepted && created.generatedObjectIds().size() == 2U,
              "semantic clipboard setup")) {
    return false;
  }

  const cr::CreativeObjectId selectedGenerated =
      created.generatedObjectIds().front();
  cr::CreativeClipboard clipboard;
  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(
          sourceDocument, std::span{&selectedGenerated, 1U}, clipboard);
  bool ok = expect(copied.accepted && copied.requestedObjectCount == 1U &&
                       copied.copiedObjectCount == 3U &&
                       copied.copiedPatternRecipeCount == 1U &&
                       clipboard.patternRecipes.size() == 1U,
                   "generated-member copy expands to its semantic pattern") &&
            expect(clipboard.patternRecipes.front().sourceObjectIds ==
                           std::vector<cr::CreativeObjectId>{source} &&
                       clipboard.patternRecipes.front().generatedObjectIds ==
                           std::vector<cr::CreativeObjectId>{
                               created.generatedObjectIds().begin(),
                               created.generatedObjectIds().end()},
                   "semantic clipboard retains source and generated ownership");

  cr::CreativeDocument targetDocument = ::document("semantic clipboard target");
  const cr::CreativeObjectId existingSource =
      createGroup(targetDocument, "Existing", {20.0, 0.0, 0.0});
  cr::CreativeLinearArrayRequest existingRequest;
  existingRequest.copyCount = cr::CreativeLinearArrayCopyCount::One;
  const cr::CreativeLinearArrayReceipt existing =
      cr::createCreativeLinearArrayAtomically(
          targetDocument, std::span{&existingSource, 1U}, existingRequest);
  cr::CreativeClipboardPasteRequest pasteRequest;
  pasteRequest.offset = {10.0, 0.0, 0.0};
  const cr::CreativeClipboardPasteReceipt pasted =
      cr::pasteCreativeClipboardAtomically(targetDocument, clipboard,
                                           pasteRequest);
  if (!expect(existing.accepted && pasted.accepted && pasted.changed &&
                  pasted.pastedObjectCount == 3U &&
                  pasted.pastedPatternRecipeCount == 1U &&
                  pasted.patternRecipeIdRemaps.size() == 1U,
              "cross-document paste remaps the semantic pattern")) {
    return false;
  }
  const cr::CreativePatternRecipeId pastedRecipeId =
      pasted.patternRecipeIdRemaps.front().pastedRecipeId;
  const cr::CreativePatternRecipe* pastedRecipe =
      cr::findCreativePatternRecipe(targetDocument.patternRecipeStore(),
                                    pastedRecipeId);
  const auto remappedObjectId = [&pasted](cr::CreativeObjectId objectId) {
    const auto found = std::find_if(
        pasted.idRemaps.begin(), pasted.idRemaps.end(),
        [objectId](const cr::CreativeClipboardIdRemap& remap) {
          return remap.sourceObjectId == objectId;
        });
    return found == pasted.idRemaps.end() ? cr::kInvalidObjectId
                                         : found->pastedObjectId;
  };
  ok = expect(pastedRecipe != nullptr &&
                  pastedRecipeId != existing.patternRecipeId &&
                  pastedRecipe->sourceObjectIds ==
                      std::vector<cr::CreativeObjectId>{
                          remappedObjectId(source)} &&
                  pastedRecipe->generatedObjectIds ==
                      std::vector<cr::CreativeObjectId>{
                          remappedObjectId(created.generatedObjectIds()[0]),
                          remappedObjectId(created.generatedObjectIds()[1])},
              "pasted recipe receives fresh stable ids and object remaps") &&
       ok;

  cr::CreativeLinearArrayRequest updatedRequest = request;
  updatedRequest.direction = cr::CreativeLinearArrayDirection::NegativeZ;
  updatedRequest.copyCount = cr::CreativeLinearArrayCopyCount::Four;
  const std::vector<cr::CreativeObjectId> originalGenerated{
      created.generatedObjectIds().begin(), created.generatedObjectIds().end()};
  const cr::CreativeLinearArrayReceipt updated =
      cr::updateCreativeLinearArrayRecipeAtomically(
          targetDocument, pastedRecipeId, updatedRequest);
  return expect(updated.accepted && updated.changed &&
                    cr::findCreativePatternRecipe(
                        targetDocument.patternRecipeStore(),
                        existing.patternRecipeId) != nullptr,
                "pasted pattern remains independently editable") &&
         expect(std::all_of(
                    originalGenerated.begin(), originalGenerated.end(),
                    [&sourceDocument](cr::CreativeObjectId objectId) {
                      return sourceDocument.findObject(objectId) != nullptr;
                    }),
                "editing the pasted pattern leaves source geometry untouched") &&
         ok;
}

bool semanticClipboardTranslatesAssetScatterRecipeGeometry() {
  cr::CreativeDocument sourceDocument = ::document("scatter clipboard source");
  const cr::CreativeObjectId generated = createRoom(
      sourceDocument, "Scatter Rock", {{1.0, 0.0, 2.0}, {2.0, 1.0, 3.0}});
  cr::CreativePatternRecipeMutationRequest add;
  add.kind = cr::CreativePatternRecipeMutationKind::Add;
  add.recipe.kind = cr::CreativePatternRecipeKind::AssetScatter;
  add.recipe.generatedObjectIds = {generated};
  add.recipe.scatter.objectKind = cr::CreativeObjectKind::Rock;
  add.recipe.scatter.assetId = "environment/rock_a";
  add.recipe.scatter.assetSourceBounds = {{-0.5, 0.0, -0.5},
                                          {0.5, 1.0, 0.5}};
  add.recipe.scatter.paintCenters = {{1.5, 0.0, 2.5}, {5.0, 0.0, 6.0}};
  add.recipe.scatter.exclusions = {{{3.0, 0.0, 4.0}, 0.75}};
  const cr::CreativePatternRecipeMutationReceipt added =
      sourceDocument.applyPatternRecipeMutation(add);

  cr::CreativeClipboard clipboard;
  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(
          sourceDocument, std::span{&generated, 1U}, clipboard);
  cr::CreativeDocument targetDocument = ::document("scatter clipboard target");
  cr::CreativeClipboardPasteRequest paste;
  paste.offset = {10.0, 2.0, -3.0};
  const cr::CreativeClipboardPasteReceipt pasted =
      cr::pasteCreativeClipboardAtomically(targetDocument, clipboard, paste);
  const cr::CreativePatternRecipe* pastedRecipe =
      pasted.patternRecipeIdRemaps.empty()
          ? nullptr
          : cr::findCreativePatternRecipe(
                targetDocument.patternRecipeStore(),
                pasted.patternRecipeIdRemaps.front().pastedRecipeId);

  return expect(added.accepted && copied.accepted &&
                    copied.copiedPatternRecipeCount == 1U,
                "scatter clipboard expands generated member to recipe") &&
         expect(pasted.accepted && pasted.pastedObjectCount == 1U &&
                    pasted.pastedPatternRecipeCount == 1U &&
                    pastedRecipe != nullptr,
                "scatter clipboard pastes editable relationship") &&
         expect(pastedRecipe->sourceObjectIds.empty() &&
                    pastedRecipe->generatedObjectIds.size() == 1U &&
                    pastedRecipe->scatter.paintCenters.size() == 2U &&
                    pastedRecipe->scatter.exclusions.size() == 1U,
                "scatter clipboard preserves generated-only ownership") &&
         expect(cr::creativeVec3ExactlyEqual(
                    pastedRecipe->scatter.paintCenters[0],
                    {11.5, 2.0, -0.5}) &&
                    cr::creativeVec3ExactlyEqual(
                        pastedRecipe->scatter.paintCenters[1],
                        {15.0, 2.0, 3.0}) &&
                    cr::creativeVec3ExactlyEqual(
                        pastedRecipe->scatter.exclusions[0].center,
                        {13.0, 2.0, 1.0}) &&
                    pastedRecipe->scatter.exclusions[0].radiusMeters == 0.75,
                "scatter clipboard translates centers but not radii");
}

bool semanticClipboardKeepsIndependentSourcesIndependent() {
  cr::CreativeDocument document = ::document("semantic source only");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {0.0, 0.0, 0.0});
  cr::CreativeLinearArrayRequest request;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  const cr::CreativeLinearArrayReceipt created =
      cr::createCreativeLinearArrayAtomically(
          document, std::span{&source, 1U}, request);
  cr::CreativeClipboard clipboard;
  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(
          document, std::span{&source, 1U}, clipboard);
  return expect(created.accepted && copied.accepted &&
                    copied.copiedObjectCount == 1U &&
                    copied.copiedPatternRecipeCount == 0U &&
                    clipboard.objects.size() == 1U &&
                    clipboard.patternRecipes.empty(),
                "copying a pattern source alone stays an independent copy");
}

bool semanticClipboardRejectsInvalidAndUnrepresentableRecipesAtomically() {
  cr::CreativeDocument target = ::document("semantic clipboard rejection");
  cr::CreativeClipboard invalid;
  cr::CreativeObject object;
  object.id = 1U;
  object.kind = cr::CreativeObjectKind::Group;
  object.name = "Source";
  invalid.objects.push_back(object);
  cr::CreativePatternRecipe recipe;
  recipe.id = 1U;
  recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  recipe.sourceObjectIds = {1U};
  recipe.generatedObjectIds = {999U};
  invalid.patternRecipes.push_back(recipe);
  const std::uint64_t revisionBefore = target.revision();
  const cr::CreativeClipboardPasteReceipt dangling =
      cr::pasteCreativeClipboardAtomically(target, invalid);

  cr::CreativeDocument sourceDocument = ::document("transform rejection source");
  const cr::CreativeObjectId source =
      createGroup(sourceDocument, "Source", {0.0, 0.0, 0.0});
  cr::CreativeLinearArrayRequest arrayRequest;
  arrayRequest.copyCount = cr::CreativeLinearArrayCopyCount::One;
  const cr::CreativeLinearArrayReceipt array =
      cr::createCreativeLinearArrayAtomically(
          sourceDocument, std::span{&source, 1U}, arrayRequest);
  cr::CreativeClipboard semantic;
  const cr::CreativeObjectId generated = array.generatedObjectIds().front();
  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(
          sourceDocument, std::span{&generated, 1U}, semantic);
  cr::CreativeClipboardPasteRequest scale;
  scale.scaleFactor = {2.0, 2.0, 2.0};
  const cr::CreativeClipboardPasteReceipt unrepresentable =
      cr::pasteCreativeClipboardAtomically(target, semantic, scale);

  return expect(!dangling.accepted && !dangling.changed &&
                    dangling.status == cr::CreativeClipboardStatus::InvalidClipboard &&
                    dangling.reasonCode ==
                        "creative_clipboard_pattern_references_invalid" &&
                    target.revision() == revisionBefore &&
                    target.objectCount() == 0U,
                "dangling clipboard recipe rejects without mutation") &&
         expect(array.accepted && copied.accepted &&
                    !unrepresentable.accepted && !unrepresentable.changed &&
                    unrepresentable.reasonCode ==
                        "creative_clipboard_pattern_transform_unsupported" &&
                    unrepresentable.pastedObjectIds.empty() &&
                    unrepresentable.patternRecipeIdRemaps.empty() &&
                    target.revision() == revisionBefore,
                "unrepresentable recipe transform fails closed before mutation");
}

bool semanticCutAndDuplicatePreserveWholePatternAtomically() {
  cr::CreativeDocument duplicatedDocument = ::document("semantic duplicate");
  const cr::CreativeObjectId source =
      createGroup(duplicatedDocument, "Source", {0.0, 0.0, 0.0});
  cr::CreativeLinearArrayRequest request;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  const cr::CreativeLinearArrayReceipt created =
      cr::createCreativeLinearArrayAtomically(
          duplicatedDocument, std::span{&source, 1U}, request);
  const cr::CreativeObjectId generated = created.generatedObjectIds().front();
  const cr::CreativeDuplicateCommandReceipt duplicated =
      cr::duplicateDocumentObjectsAtomically(
          duplicatedDocument, std::span{&generated, 1U});
  bool ok = expect(duplicated.accepted && duplicated.changed &&
                       duplicated.duplicatedObjectCount == 3U &&
                       duplicated.duplicatedPatternRecipeCount == 1U &&
                       duplicated.duplicatedSelectionObjectIds.size() == 1U &&
                       duplicatedDocument.patternRecipeStore().recipes.size() ==
                           2U,
                   "duplicate of generated member creates an editable pattern copy");

  cr::CreativeDocument cutDocument = ::document("semantic cut");
  const cr::CreativeObjectId cutSource =
      createGroup(cutDocument, "Source", {0.0, 0.0, 0.0});
  const cr::CreativeLinearArrayReceipt cutArray =
      cr::createCreativeLinearArrayAtomically(
          cutDocument, std::span{&cutSource, 1U}, request);
  const cr::CreativeObjectId cutGenerated =
      cutArray.generatedObjectIds().front();
  cr::CreativeClipboard cutClipboard;
  const std::uint64_t cutRevisionBefore = cutDocument.revision();
  const cr::CreativeClipboardCutReceipt cut =
      cr::cutDocumentObjectsAtomically(
          cutDocument, std::span{&cutGenerated, 1U}, cutClipboard);
  ok = expect(cut.accepted && cut.changed && cut.cutObjectCount == 3U &&
                  cut.cutPatternRecipeCount == 1U &&
                  cutClipboard.patternRecipes.size() == 1U &&
                  cutDocument.objectCount() == 0U &&
                  cutDocument.patternRecipeStore().recipes.empty(),
              "cut of generated member removes the whole semantic pattern") &&
       expect(
           cut.revisionBefore == cutRevisionBefore &&
               cut.revisionAfter == cutRevisionBefore + 1U &&
               std::all_of(
                   cut.removeReceipts.begin(), cut.removeReceipts.end(),
                   [cutRevisionBefore](
                       const cr::CreativeDocumentRemoveReceipt& item) {
                     return item.revisionBefore == cutRevisionBefore &&
                            item.revisionAfter == cutRevisionBefore + 1U;
                   }),
           "cut nested removes use one live revision range") &&
       ok;

  cr::CreativeDocument dependent = ::document("semantic cut dependency");
  const cr::CreativeObjectId sharedSource =
      createGroup(dependent, "Shared", {0.0, 0.0, 0.0});
  const cr::CreativeLinearArrayReceipt first =
      cr::createCreativeLinearArrayAtomically(
          dependent, std::span{&sharedSource, 1U}, request);
  cr::CreativeLinearArrayRequest secondRequest = request;
  secondRequest.direction = cr::CreativeLinearArrayDirection::PositiveZ;
  const cr::CreativeLinearArrayReceipt second =
      cr::createCreativeLinearArrayAtomically(
          dependent, std::span{&sharedSource, 1U}, secondRequest);
  const std::uint64_t dependentRevision = dependent.revision();
  const std::size_t dependentObjects = dependent.objectCount();
  cr::CreativeClipboard rejectedClipboard;
  const cr::CreativeObjectId firstGenerated =
      first.generatedObjectIds().front();
  const cr::CreativeClipboardCutReceipt rejected =
      cr::cutDocumentObjectsAtomically(
          dependent, std::span{&firstGenerated, 1U}, rejectedClipboard);
  return expect(first.accepted && second.accepted && !rejected.accepted &&
                    !rejected.changed &&
                    rejected.reasonCode ==
                        "creative_clipboard_cut_external_reference" &&
                    dependent.revision() == dependentRevision &&
                    dependent.objectCount() == dependentObjects &&
                    rejectedClipboard.objects.empty(),
                "cut rejects external recipe dependencies atomically") &&
         ok;
}

bool semanticClipboardRemapsInternalLinksAndRejectsCrossingLinks() {
  cr::CreativeDocument sourceDocument =
      ::document("semantic clipboard links");
  cr::CreativeDocumentCreateRequest sourceRequest;
  sourceRequest.kind = cr::CreativeObjectKind::Switch;
  sourceRequest.name = "Source";
  cr::CreativeDocumentCreateRequest targetRequest;
  targetRequest.kind = cr::CreativeObjectKind::Door;
  targetRequest.name = "Target";
  cr::CreativeDocumentCreateRequest externalRequest = targetRequest;
  externalRequest.name = "External";
  const cr::CreativeObjectId source =
      sourceDocument.createObject(sourceRequest).objectId;
  const cr::CreativeObjectId target =
      sourceDocument.createObject(targetRequest).objectId;
  const cr::CreativeObjectId external =
      sourceDocument.createObject(externalRequest).objectId;
  const cr::CreativeLogicLinkMutationReceipt internalLink =
      sourceDocument.setLogicLink(
          {source, target, cr::CreativeLogicLinkAction::Toggle});
  const cr::CreativeLogicLinkMutationReceipt crossingLink =
      sourceDocument.setLogicLink(
          {source, external, cr::CreativeLogicLinkAction::Toggle});

  const std::array selected{source, target};
  cr::CreativeClipboard clipboard;
  const cr::CreativeClipboardCopyReceipt copied =
      cr::copyDocumentObjectsToClipboard(sourceDocument, selected, clipboard);
  cr::CreativeDocument pastedDocument = ::document("pasted semantic links");
  const cr::CreativeClipboardPasteReceipt pasted =
      cr::pasteCreativeClipboardAtomically(pastedDocument, clipboard);
  const auto remappedId = [&pasted](cr::CreativeObjectId original) {
    const auto found = std::find_if(
        pasted.idRemaps.begin(), pasted.idRemaps.end(),
        [original](const cr::CreativeClipboardIdRemap& remap) {
          return remap.sourceObjectId == original;
        });
    return found == pasted.idRemaps.end() ? cr::kInvalidObjectId
                                         : found->pastedObjectId;
  };
  const cr::CreativeObjectId pastedSource = remappedId(source);
  const cr::CreativeObjectId pastedTarget = remappedId(target);
  const bool linkRemapped =
      pastedDocument.logicLinks().size() == 1U &&
      pastedDocument.logicLinks().front().sourceObjectId == pastedSource &&
      pastedDocument.logicLinks().front().targetObjectId == pastedTarget &&
      pastedDocument.logicLinks().front().action ==
          cr::CreativeLogicLinkAction::Toggle;

  const std::uint64_t revisionBefore = sourceDocument.revision();
  const std::size_t objectCountBefore = sourceDocument.objectCount();
  const std::size_t linkCountBefore = sourceDocument.logicLinks().size();
  cr::CreativeClipboard rejectedClipboard;
  const cr::CreativeClipboardCutReceipt rejectedCut =
      cr::cutDocumentObjectsAtomically(sourceDocument, selected,
                                       rejectedClipboard);
  const cr::CreativeSemanticDeleteReceipt rejectedDelete =
      cr::deleteDocumentObjectsSemanticallyAtomically(sourceDocument,
                                                       selected);

  return expect(internalLink.accepted,
                "internal logic-link fixture is valid") &&
         expect(crossingLink.accepted,
                "crossing logic-link fixture is valid") &&
         expect(copied.accepted,
                "logic-linked semantic closure copies successfully") &&
         expect(copied.copiedLogicLinkCount == 1U &&
                    clipboard.logicLinks.size() == 1U,
                "copy keeps only links whose endpoints are in the closure") &&
         expect(pasted.accepted && pasted.pastedLogicLinkCount == 1U &&
                    linkRemapped,
                "cross-document paste remaps both internal link endpoints") &&
         expect(!rejectedCut.accepted && !rejectedCut.changed &&
                    rejectedCut.reasonCode ==
                        "creative_clipboard_cut_external_reference" &&
                    rejectedClipboard.objects.empty(),
                "cut rejects a link that crosses the semantic closure") &&
         expect(!rejectedDelete.accepted && !rejectedDelete.changed &&
                    rejectedDelete.status ==
                        cr::CreativeSemanticDeleteStatus::ExternalReference &&
                    sourceDocument.revision() == revisionBefore &&
                    sourceDocument.objectCount() == objectCountBefore &&
                    sourceDocument.logicLinks().size() == linkCountBefore,
                "delete rejects crossing links without partial mutation");
}

bool semanticDeleteRemovesPatternSourceAndOutputsAsOneClosure() {
  cr::CreativeDocument document = ::document("semantic delete");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {0.0, 0.0, 0.0});
  cr::CreativeLinearArrayRequest request;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  const cr::CreativeLinearArrayReceipt created =
      cr::createCreativeLinearArrayAtomically(
          document, std::span{&source, 1U}, request);
  const cr::CreativeObjectId generated = created.generatedObjectIds().front();
  const cr::CreativeSemanticDeleteReceipt deleted =
      cr::deleteDocumentObjectsSemanticallyAtomically(
          document, std::span{&generated, 1U});

  cr::CreativeDocument dependent = ::document("semantic delete dependency");
  const cr::CreativeObjectId sharedSource =
      createGroup(dependent, "Shared", {0.0, 0.0, 0.0});
  const cr::CreativeLinearArrayReceipt first =
      cr::createCreativeLinearArrayAtomically(
          dependent, std::span{&sharedSource, 1U}, request);
  cr::CreativeLinearArrayRequest secondRequest = request;
  secondRequest.direction = cr::CreativeLinearArrayDirection::PositiveZ;
  const cr::CreativeLinearArrayReceipt second =
      cr::createCreativeLinearArrayAtomically(
          dependent, std::span{&sharedSource, 1U}, secondRequest);
  const std::uint64_t revisionBefore = dependent.revision();
  const std::size_t objectCountBefore = dependent.objectCount();
  const cr::CreativeObjectId dependentGenerated =
      first.generatedObjectIds().front();
  const cr::CreativeSemanticDeleteReceipt rejected =
      cr::deleteDocumentObjectsSemanticallyAtomically(
          dependent, std::span{&dependentGenerated, 1U});

  return expect(created.accepted && deleted.accepted && deleted.changed &&
                    deleted.status == cr::CreativeSemanticDeleteStatus::Deleted &&
                    deleted.removedObjectCount == 3U &&
                    deleted.removedPatternRecipeCount == 1U &&
                    deleted.removedObjectIds.size() == 3U &&
                    document.objectCount() == 0U &&
                    document.patternRecipeStore().recipes.empty(),
                "delete of generated output removes its editable source closure") &&
         expect(first.accepted && second.accepted && !rejected.accepted &&
                    !rejected.changed &&
                    rejected.status ==
                        cr::CreativeSemanticDeleteStatus::ExternalReference &&
                    dependent.revision() == revisionBefore &&
                    dependent.objectCount() == objectCountBefore &&
                    dependent.patternRecipeStore().recipes.size() == 2U,
                "semantic delete rejects dependent recipes without partial mutation");
}

bool linearArrayUpdateKeepsIdentityAndReplacesOnlyOwnedOutputs() {
  cr::CreativeDocument document = ::document("linear array update");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {0.0, 0.0, 0.0});
  const cr::CreativeObjectId unrelated = createRoom(
      document, "Unrelated", {{20.0, 0.0, 20.0}, {24.0, 3.0, 24.0}});
  cr::CreativeLinearArrayRequest initial;
  initial.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  const cr::CreativeLinearArrayReceipt created =
      cr::createCreativeLinearArrayAtomically(
          document, std::span{&source, 1U}, initial);
  if (!expect(created.accepted && created.generatedObjectIds().size() == 2U,
              "linear update setup")) {
    return false;
  }
  const std::vector<cr::CreativeObjectId> oldGenerated{
      created.generatedObjectIds().begin(), created.generatedObjectIds().end()};

  cr::CreativeLinearArrayRequest updatedRequest;
  updatedRequest.direction = cr::CreativeLinearArrayDirection::NegativeZ;
  updatedRequest.copyCount = cr::CreativeLinearArrayCopyCount::Four;
  updatedRequest.spacing = cr::CreativeLinearArraySpacing::TwoCells;
  updatedRequest.cellSize = 0.5;
  const std::uint64_t revisionBeforeUpdate = document.revision();
  const cr::CreativeLinearArrayReceipt updated =
      cr::updateCreativeLinearArrayRecipeAtomically(
          document, created.patternRecipeId, updatedRequest);
  const cr::CreativePatternRecipe* recipe =
      cr::findCreativePatternRecipe(document.patternRecipeStore(),
                                    created.patternRecipeId);
  const cr::CreativeObject* finalCopy =
      updated.finalCopyObjectIds().empty()
          ? nullptr
          : document.findObject(updated.finalCopyObjectIds().front());

  return expect(updated.accepted && updated.changed &&
                    updated.updatedExistingRecipe &&
                    updated.patternRecipeId == created.patternRecipeId,
                "linear update keeps editable recipe identity") &&
         expect(updated.replacedGeneratedObjectCount == 2U &&
                    updated.generatedObjectIds().size() == 4U,
                "linear update reports old and replacement output counts") &&
         expect(updated.revisionBefore == revisionBeforeUpdate &&
                    updated.revisionAfter == revisionBeforeUpdate + 1U &&
                    updated.pasteReceipt.revisionBefore ==
                        revisionBeforeUpdate &&
                    updated.pasteReceipt.revisionAfter ==
                        revisionBeforeUpdate + 1U &&
                    updated.patternMutationReceipt.revisionBefore ==
                        revisionBeforeUpdate &&
                    updated.patternMutationReceipt.revisionAfter ==
                        revisionBeforeUpdate + 1U,
                "linear update nested receipts use one live revision range") &&
         expect(document.findObject(oldGenerated[0]) == nullptr &&
                    document.findObject(oldGenerated[1]) == nullptr &&
                    document.findObject(source) != nullptr &&
                    document.findObject(unrelated) != nullptr,
                "linear update removes only prior owned outputs") &&
         expect(recipe != nullptr && recipe->linear == updatedRequest &&
                    recipe->generatedObjectIds ==
                        std::vector<cr::CreativeObjectId>{
                            updated.generatedObjectIds().begin(),
                            updated.generatedObjectIds().end()},
                "linear update stores exact replacement parameters and ids") &&
         expect(finalCopy != nullptr &&
                    near(finalCopy->transform.position.z, -4.0),
                "linear update geometry consumes replacement plan") &&
         expect(document.objectCount() == 6U && document.isValid(),
                "linear update leaves local document topology valid");
}

bool radialArrayUpdateKeepsIdentityAndRemovesOldRing() {
  cr::CreativeDocument document = ::document("radial array update");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {2.0, 0.0, 0.0});
  cr::CreativeRadialArrayRequest initial;
  initial.pivot = {};
  initial.axis = cr::CreativeAxis3::Y;
  initial.instanceCount = cr::CreativeRadialArrayInstanceCount::Four;
  const cr::CreativeRadialArrayReceipt created =
      cr::createCreativeRadialArrayAtomically(
          document, std::span{&source, 1U}, initial);
  if (!expect(created.accepted && created.generatedObjectIds().size() == 3U,
              "radial update setup")) {
    return false;
  }
  const std::vector<cr::CreativeObjectId> oldGenerated{
      created.generatedObjectIds().begin(), created.generatedObjectIds().end()};

  cr::CreativeRadialArrayRequest updatedRequest = initial;
  updatedRequest.instanceCount =
      cr::CreativeRadialArrayInstanceCount::Two;
  updatedRequest.sweep = cr::CreativeRadialArraySweep::Degrees180;
  const std::uint64_t revisionBeforeUpdate = document.revision();
  const cr::CreativeRadialArrayReceipt updated =
      cr::updateCreativeRadialArrayRecipeAtomically(
          document, created.patternRecipeId, updatedRequest);
  const cr::CreativePatternRecipe* recipe =
      cr::findCreativePatternRecipe(document.patternRecipeStore(),
                                    created.patternRecipeId);
  const cr::CreativeObject* replacement =
      updated.generatedObjectIds().empty()
          ? nullptr
          : document.findObject(updated.generatedObjectIds().front());
  return expect(updated.accepted && updated.updatedExistingRecipe &&
                    updated.replacedGeneratedObjectCount == 3U &&
                    updated.generatedObjectIds().size() == 1U,
                "radial update replaces the prior ring") &&
         expect(updated.revisionBefore == revisionBeforeUpdate &&
                    updated.revisionAfter == revisionBeforeUpdate + 1U &&
                    updated.pasteReceipt.revisionBefore ==
                        revisionBeforeUpdate &&
                    updated.pasteReceipt.revisionAfter ==
                        revisionBeforeUpdate + 1U &&
                    updated.patternMutationReceipt.revisionBefore ==
                        revisionBeforeUpdate &&
                    updated.patternMutationReceipt.revisionAfter ==
                        revisionBeforeUpdate + 1U,
                "radial update nested receipts use one live revision range") &&
         expect(std::all_of(oldGenerated.begin(), oldGenerated.end(),
                            [&document](cr::CreativeObjectId objectId) {
                              return document.findObject(objectId) == nullptr;
                            }),
                "radial update removes every old owned output") &&
         expect(recipe != nullptr && recipe->id == created.patternRecipeId &&
                    recipe->radial == updatedRequest &&
                    recipe->generatedObjectIds.size() == 1U,
                "radial update keeps identity and exact parameters") &&
         expect(replacement != nullptr &&
                    near(replacement->transform.position.x, -2.0) &&
                    near(replacement->transform.position.z, 0.0),
                "radial update emits the new partial-sweep endpoint") &&
         expect(document.objectCount() == 2U && document.isValid(),
                "radial update leaves source plus replacement only");
}

bool patternUpdateRejectsExternalAndRecipeDependencies() {
  cr::CreativeDocument externalChildDocument =
      ::document("pattern external child");
  const cr::CreativeObjectId source =
      createGroup(externalChildDocument, "Source", {});
  cr::CreativeLinearArrayRequest oneCopy;
  oneCopy.copyCount = cr::CreativeLinearArrayCopyCount::One;
  const cr::CreativeLinearArrayReceipt created =
      cr::createCreativeLinearArrayAtomically(
          externalChildDocument, std::span{&source, 1U}, oneCopy);
  const cr::CreativeObjectId generated =
      created.generatedObjectIds().empty()
          ? cr::kInvalidObjectId
          : created.generatedObjectIds().front();
  const cr::CreativeObjectId externalChild =
      createGroup(externalChildDocument, "Attached", {}, generated);
  const std::uint64_t externalRevision = externalChildDocument.revision();
  const cr::CreativeLinearArrayReceipt externalRejected =
      cr::updateCreativeLinearArrayRecipeAtomically(
          externalChildDocument, created.patternRecipeId, {});

  cr::CreativeDocument dependentDocument =
      ::document("pattern dependent recipe");
  const cr::CreativeObjectId dependentSource =
      createGroup(dependentDocument, "Source", {});
  const cr::CreativeLinearArrayReceipt upstream =
      cr::createCreativeLinearArrayAtomically(
          dependentDocument, std::span{&dependentSource, 1U}, oneCopy);
  const cr::CreativeObjectId upstreamOutput =
      upstream.generatedObjectIds().front();
  const cr::CreativeLinearArrayReceipt downstream =
      cr::createCreativeLinearArrayAtomically(
          dependentDocument, std::span{&upstreamOutput, 1U}, oneCopy);
  const std::uint64_t dependentRevision = dependentDocument.revision();
  const cr::CreativeLinearArrayReceipt dependentRejected =
      cr::updateCreativeLinearArrayRecipeAtomically(
          dependentDocument, upstream.patternRecipeId, {});

  return expect(!externalRejected.accepted &&
                    externalRejected.status ==
                        cr::CreativeLinearArrayStatus::
                            RecipeDependencyConflict &&
                    externalRejected.failedObjectId == externalChild &&
                    externalChildDocument.revision() == externalRevision,
                "array update refuses an externally attached child") &&
         expect(externalChildDocument.findObject(generated) != nullptr &&
                    externalChildDocument.findObject(externalChild) != nullptr,
                "external-child rejection publishes no partial removal") &&
         expect(downstream.accepted && !dependentRejected.accepted &&
                    dependentRejected.status ==
                        cr::CreativeLinearArrayStatus::
                            RecipeDependencyConflict &&
                    dependentRejected.failedObjectId == upstreamOutput &&
                    dependentDocument.revision() == dependentRevision,
                "array update refuses to invalidate a downstream recipe") &&
         expect(dependentDocument.patternRecipeStore().recipes.size() == 2U &&
                    dependentDocument.isValid(),
                "dependent-recipe rejection preserves both relationships");
}

bool detachingPatternKeepsBakedGeometryIndependent() {
  cr::CreativeDocument document = ::document("pattern detach");
  const cr::CreativeObjectId source =
      createGroup(document, "Source", {});
  cr::CreativeLinearArrayRequest request;
  request.copyCount = cr::CreativeLinearArrayCopyCount::Two;
  const cr::CreativeLinearArrayReceipt created =
      cr::createCreativeLinearArrayAtomically(
          document, std::span{&source, 1U}, request);
  const std::vector<cr::CreativeObjectId> outputs{
      created.generatedObjectIds().begin(), created.generatedObjectIds().end()};
  const cr::CreativePatternRecipeMutationReceipt detached =
      cr::detachCreativePatternRecipe(document, created.patternRecipeId);
  const std::uint64_t detachedRevision = document.revision();
  const cr::CreativeLinearArrayReceipt updateRejected =
      cr::updateCreativeLinearArrayRecipeAtomically(
          document, created.patternRecipeId, request);
  return expect(detached.accepted && detached.changed &&
                    detached.status ==
                        cr::CreativePatternRecipeMutationStatus::Applied,
                "detach removes only editable relation") &&
         expect(document.patternRecipeStore().recipes.empty() &&
                    document.findObject(source) != nullptr &&
                    document.findObject(outputs[0]) != nullptr &&
                    document.findObject(outputs[1]) != nullptr,
                "detach preserves source and every baked output") &&
         expect(!updateRejected.accepted &&
                    updateRejected.status ==
                        cr::CreativeLinearArrayStatus::RecipeNotFound &&
                    document.revision() == detachedRevision,
                "detached geometry no longer accepts relationship updates") &&
         expect(document.isValid(),
                "detached geometry remains a valid independent document");
}

cr::CreativePatternRecipe validAssetScatterRecipe() {
  cr::CreativePatternRecipe recipe;
  recipe.id = 1U;
  recipe.kind = cr::CreativePatternRecipeKind::AssetScatter;
  recipe.generatedObjectIds = {10U, 11U};
  recipe.scatter.objectKind = cr::CreativeObjectKind::Rock;
  recipe.scatter.assetId = "environment/rock_a";
  recipe.scatter.assetContentHash = 42U;
  recipe.scatter.assetMaterialVariant = "mossy";
  recipe.scatter.assetSourceBounds = {{-0.5, 0.0, -0.5},
                                      {0.5, 1.0, 0.5}};
  recipe.scatter.paintCenters = {{2.0, 0.0, 3.0}};
  recipe.scatter.exclusions = {{{2.5, 0.0, 3.5}, 0.75}};
  recipe.scatter.mask = cr::CreativeAssetScatterRecipeMask::Circle;
  recipe.scatter.yaw = cr::CreativeAssetScatterRecipeYaw::QuarterTurns;
  recipe.scatter.baseYawRadians = std::numbers::pi * 0.5;
  recipe.scatter.radiusMeters = 6.0;
  recipe.scatter.spacingMeters = 1.5;
  recipe.scatter.densityFraction = 0.8;
  recipe.scatter.scaleVariation = 0.2;
  recipe.scatter.maximumSlopeRadians = std::numbers::pi / 6.0;
  recipe.scatter.projectToTerrainSurface = true;
  recipe.scatter.avoidCollisions = false;
  recipe.scatter.seed = 99U;
  recipe.scatter.maxGeneratedObjects = 128U;
  return recipe;
}

cr::CreativeDocumentCreateRequest scatterCreateRequest(
    const cr::CreativeAssetScatterRecipe& recipe,
    std::string_view name,
    cr::CreativeVec3 position) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = recipe.objectKind;
  request.name = std::string{name};
  request.assetId = recipe.assetId;
  request.assetContentHash = recipe.assetContentHash;
  request.assetMaterialVariant = recipe.assetMaterialVariant;
  request.bounds = recipe.assetSourceBounds;
  request.hasBoundsOverride = true;
  request.transform.position = position;
  request.hasTransformOverride = true;
  return request;
}

bool assetScatterRecipeMutationIsAtomicEditableAndBakeable() {
  cr::CreativeDocument document = ::document("scatter mutation");
  const cr::CreativeObjectId unrelated =
      createGroup(document, "Unrelated", {-20.0, 0.0, 0.0});
  cr::CreativeAssetScatterRecipe recipe = validAssetScatterRecipe().scatter;
  const std::array createRequests{
      scatterCreateRequest(recipe, "Rock 1", {2.0, 0.0, 3.0}),
      scatterCreateRequest(recipe, "Rock 2", {5.0, 0.0, 6.0}),
  };

  cr::CreativeDocumentCreateRequest mismatched = createRequests.front();
  mismatched.assetId = "environment/not_the_recipe_asset";
  const std::uint64_t beforeInvalid = document.revision();
  const cr::CreativeAssetScatterRecipeMutationReceipt invalid =
      cr::createCreativeAssetScatterRecipeAtomically(
          document, std::span{&mismatched, 1U}, {}, recipe);
  const cr::CreativeAssetScatterRecipeMutationReceipt created =
      cr::createCreativeAssetScatterRecipeAtomically(
          document, createRequests, {}, recipe);
  if (!expect(!invalid.accepted &&
                  invalid.status == cr::CreativeAssetScatterRecipeMutationStatus::
                                        InvalidRequest &&
                  document.findObject(unrelated) != nullptr,
              "scatter rejects recipe-output identity drift atomically") ||
      !expect(created.accepted && created.changed &&
                  created.generatedObjectIds.size() == 2U &&
                  created.patternRecipeId !=
                      cr::kInvalidCreativePatternRecipeId,
              "scatter creates geometry and one recipe atomically")) {
    return false;
  }
  const std::vector<cr::CreativeObjectId> oldOutputs =
      created.generatedObjectIds;
  const cr::CreativePatternRecipe* createdRecipe =
      cr::findCreativePatternRecipe(document.patternRecipeStore(),
                                    created.patternRecipeId);
  bool ok = expect(beforeInvalid == invalid.revisionAfter &&
                       createdRecipe != nullptr &&
                       createdRecipe->sourceObjectIds.empty() &&
                       createdRecipe->generatedObjectIds == oldOutputs &&
                       createdRecipe->scatter == recipe,
                   "created scatter retains exact editable provenance");

  cr::CreativeAssetScatterRecipe changedRecipe = recipe;
  changedRecipe.seed += 1U;
  changedRecipe.paintCenters.push_back({9.0, 0.0, 9.0});
  const cr::CreativeDocumentCreateRequest replacementRequest =
      scatterCreateRequest(changedRecipe, "Regenerated Rock",
                           {9.0, 0.0, 9.0});
  const cr::CreativeAssetScatterRecipeMutationReceipt updated =
      cr::updateCreativeAssetScatterRecipeAtomically(
          document, created.patternRecipeId,
          std::span{&replacementRequest, 1U}, changedRecipe);
  const cr::CreativePatternRecipe* updatedRecipe =
      cr::findCreativePatternRecipe(document.patternRecipeStore(),
                                    created.patternRecipeId);
  ok = expect(updated.accepted && updated.changed &&
                  updated.updatedExistingRecipe &&
                  updated.replacedGeneratedObjectCount == 2U &&
                  updated.generatedObjectIds.size() == 1U &&
                  updatedRecipe != nullptr &&
                  updatedRecipe->id == created.patternRecipeId &&
                  updatedRecipe->scatter == changedRecipe,
              "scatter regenerate keeps recipe identity and exact settings") &&
       expect(document.findObject(oldOutputs[0]) == nullptr &&
                  document.findObject(oldOutputs[1]) == nullptr &&
                  document.findObject(unrelated) != nullptr &&
                  document.objectCount() == 2U,
              "scatter regenerate replaces only owned outputs") &&
       ok;

  const cr::CreativeObjectId regenerated = updated.generatedObjectIds.front();
  cr::CreativeLinearArrayRequest downstreamRequest;
  downstreamRequest.copyCount = cr::CreativeLinearArrayCopyCount::One;
  const cr::CreativeLinearArrayReceipt downstream =
      cr::createCreativeLinearArrayAtomically(
          document, std::span{&regenerated, 1U}, downstreamRequest);
  const std::uint64_t beforeConflict = document.revision();
  const cr::CreativeAssetScatterRecipeMutationReceipt conflict =
      cr::updateCreativeAssetScatterRecipeAtomically(
          document, created.patternRecipeId,
          std::span{&replacementRequest, 1U}, changedRecipe);
  const cr::CreativePatternRecipeMutationReceipt baked =
      cr::detachCreativePatternRecipe(document, created.patternRecipeId);
  return expect(downstream.accepted && !conflict.accepted &&
                    conflict.status ==
                        cr::CreativeAssetScatterRecipeMutationStatus::
                            RecipeDependencyConflict &&
                    conflict.failedObjectId == regenerated &&
                    document.findObject(regenerated) != nullptr &&
                    beforeConflict == conflict.revisionAfter,
                "scatter regenerate refuses downstream recipe dependency") &&
         expect(baked.accepted && baked.changed &&
                    cr::findCreativePatternRecipe(
                        document.patternRecipeStore(),
                        created.patternRecipeId) == nullptr &&
                    document.findObject(regenerated) != nullptr &&
                    cr::findCreativePatternRecipe(
                        document.patternRecipeStore(),
                        downstream.patternRecipeId) != nullptr,
                "scatter bake detaches recipe and keeps instances") &&
         ok;
}

bool assetScatterRecipePinsEditableContract() {
  const cr::CreativePatternRecipe baseline = validAssetScatterRecipe();
  bool ok = expect(cr::validateCreativePatternRecipe(baseline),
                   "generated-only circle scatter recipe is valid") &&
            expect(baseline == validAssetScatterRecipe(),
                   "scatter recipe exact equality is stable") &&
            expect(cr::toString(cr::CreativePatternRecipeKind::AssetScatter) ==
                       "AssetScatter" &&
                       cr::toString(baseline.scatter.mask) == "Circle" &&
                       cr::toString(baseline.scatter.yaw) == "QuarterTurns",
                   "scatter recipe enum text is stable");

  cr::CreativePatternRecipe selection = baseline;
  selection.scatter.mask = cr::CreativeAssetScatterRecipeMask::Selection;
  ok = expect(!cr::validateCreativePatternRecipe(selection),
              "selection scatter requires source objects") &&
       ok;
  selection.sourceObjectIds = {7U};
  ok = expect(cr::validateCreativePatternRecipe(selection),
              "selection scatter accepts a source filter") &&
       ok;

  cr::CreativePatternRecipe box = baseline;
  box.scatter.mask = cr::CreativeAssetScatterRecipeMask::Box;
  box.scatter.paintCenters.push_back({8.0, 0.0, 9.0});
  ok = expect(cr::validateCreativePatternRecipe(box),
              "box scatter accepts multiple paint centers") &&
       ok;

  cr::CreativePatternRecipe invalid = baseline;
  invalid.scatter.paintCenters.push_back(invalid.scatter.paintCenters.front());
  ok = expect(!cr::validateCreativePatternRecipe(invalid),
              "duplicate scatter paint centers are rejected") &&
       ok;
  invalid = baseline;
  invalid.scatter.exclusions.front().radiusMeters = 0.0;
  ok = expect(!cr::validateCreativePatternRecipe(invalid),
              "nonpositive scatter exclusions are rejected") &&
       ok;
  invalid = baseline;
  invalid.scatter.densityFraction =
      std::numeric_limits<double>::quiet_NaN();
  ok = expect(!cr::validateCreativePatternRecipe(invalid),
              "nonfinite scatter parameters are rejected") &&
       ok;
  invalid = baseline;
  invalid.scatter.maxGeneratedObjects =
      cr::kCreativeAssetScatterGeneratedObjectCapacity + 1U;
  return expect(!cr::validateCreativePatternRecipe(invalid),
                "scatter generated capacity cannot be raised") &&
         ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = planUsesOrdinalOffsetsWithoutAccumulation() && ok;
  ok = planRejectsInvalidAndOversizedRequests() && ok;
  ok = radialPlanPinsClosedAndPartialSweepLaws() && ok;
  ok = radialPlanRejectsInvalidAndOversizedRequests() && ok;
  ok = batchPasteRemapsEachCopyIndependently() && ok;
  ok = copyDerivesDeterministicPlacementAnchor() && ok;
  ok = singlePasteRetainsTheExistingReceiptContract() && ok;
  ok = transformedPasteMatchesTheSharedPlacementPlan() && ok;
  ok = axisAngleClipboardPasteRotatesRigidly() && ok;
  ok = lateBatchFailureRollsBackEarlierStagedCopy() && ok;
  ok = arrayExecutionCreatesCopiesAndIdentifiesFinalGroup() && ok;
  ok = arrayExecutionRejectsMissingSourceWithoutMutation() && ok;
  ok = radialExecutionRotatesGroupAndRemapsParents() && ok;
  ok = radialExecutionRejectsDegeneratePivotWithoutMutation() && ok;
  ok = patternRecipeRejectsMissingReferencesAtomically() && ok;
  ok = deletingPatternMemberDetachesRelationship() && ok;
  ok = semanticClipboardPreservesEditablePatternAcrossDocuments() && ok;
  ok = semanticClipboardTranslatesAssetScatterRecipeGeometry() && ok;
  ok = semanticClipboardKeepsIndependentSourcesIndependent() && ok;
  ok = semanticClipboardRejectsInvalidAndUnrepresentableRecipesAtomically() &&
       ok;
  ok = semanticCutAndDuplicatePreserveWholePatternAtomically() && ok;
  ok = semanticClipboardRemapsInternalLinksAndRejectsCrossingLinks() && ok;
  ok = semanticDeleteRemovesPatternSourceAndOutputsAsOneClosure() && ok;
  ok = linearArrayUpdateKeepsIdentityAndReplacesOnlyOwnedOutputs() && ok;
  ok = radialArrayUpdateKeepsIdentityAndRemovesOldRing() && ok;
  ok = patternUpdateRejectsExternalAndRecipeDependencies() && ok;
  ok = detachingPatternKeepsBakedGeometryIndependent() && ok;
  ok = assetScatterRecipePinsEditableContract() && ok;
  ok = assetScatterRecipeMutationIsAtomicEditableAndBakeable() && ok;
  return ok ? 0 : 1;
}
