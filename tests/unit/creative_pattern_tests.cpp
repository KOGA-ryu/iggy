#include "app/iggy3d/creative/tools/Pattern.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
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
         expect(document.revision() == revisionBefore + 4U,
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
                    document.revision() == revisionBefore + 4U,
                "array execution keeps originals and adds four objects");
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

}  // namespace

int main() {
  bool ok = true;
  ok = planUsesOrdinalOffsetsWithoutAccumulation() && ok;
  ok = planRejectsInvalidAndOversizedRequests() && ok;
  ok = batchPasteRemapsEachCopyIndependently() && ok;
  ok = copyDerivesDeterministicPlacementAnchor() && ok;
  ok = singlePasteRetainsTheExistingReceiptContract() && ok;
  ok = lateBatchFailureRollsBackEarlierStagedCopy() && ok;
  ok = arrayExecutionCreatesCopiesAndIdentifiesFinalGroup() && ok;
  ok = arrayExecutionRejectsMissingSourceWithoutMutation() && ok;
  return ok ? 0 : 1;
}
