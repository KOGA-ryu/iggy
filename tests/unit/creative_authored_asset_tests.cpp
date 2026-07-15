#include "EditorAssetLibrary.hpp"
#include "EditorAuthoredAssets.hpp"
#include "EditorObjectActions.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "core/math/Mat4.hpp"

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::fabs(lhs - rhs) < 1.0e-8;
}

bool vecNear(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

const cr::CreativeObject* clipboardObjectNamed(
    const cr::CreativeClipboard& clipboard,
    std::string_view name) {
  const auto found = std::find_if(
      clipboard.objects.begin(), clipboard.objects.end(),
      [name](const cr::CreativeObject& object) {
        return object.name == name;
      });
  return found == clipboard.objects.end() ? nullptr : &*found;
}

cr::CreativeObjectId createCrate(cr::CreativeDocument& document,
                                 std::string_view name,
                                 cr::CreativeVec3 position,
                                 std::optional<cr::CreativeObjectId> parent =
                                     std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = std::string{name};
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.parentId = parent;
  return document.createObject(request).objectId;
}

void selectOnly(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  static_cast<void>(facade.dispatchToolInput(input));
}

cr::CreativeAuthoredAssetCaptureResult makeTwoCrateDefinition() {
  cr::CreativeDocument source = cr::CreativeDocument::create("Source");
  static_cast<void>(source.assignId(701U));
  const cr::CreativeObjectId left =
      createCrate(source, "Left", {10.0, 0.0, 20.0});
  const cr::CreativeObjectId right =
      createCrate(source, "Right", {12.0, 0.0, 20.0});
  const std::array selected{left, right};
  return cr::captureCreativeAuthoredAsset(
      {&source, selected, "authored_0001", "Twin Crates", 702U});
}

bool captureInstantiateSelectAndUnpack() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  if (!expect(captured.accepted && captured.capturedObjectCount == 2U &&
                  captured.storageDocument.objectCount() == 3U,
              "selection captures with a durable source-root wrapper")) {
    return false;
  }

  cr::CreativeDocument target = cr::CreativeDocument::create("Target");
  static_cast<void>(target.assignId(703U));
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = &captured.definition;
  placement.targetAnchor = {5.0, 1.0, -3.0};
  placement.yawRadians = 1.5707963267948966;
  const cr::CreativeAuthoredAssetInstanceReceipt placed =
      cr::instantiateCreativeAuthoredAssetAtomically(target, placement);
  const cr::CreativeObject* root =
      target.findObject(placed.instanceRootObjectId);
  const cr::CreativeObjectId childId =
      placed.instanceObjectIds.empty() ? cr::kInvalidObjectId
                                       : placed.instanceObjectIds.front();
  const cr::CreativeObject* child = target.findObject(childId);
  const cr::CreativeObjectId selectedRoot =
      cr::resolveCreativeHierarchyInteractionRoot(
          target, std::span{&placed.instanceRootObjectId, 1U}, childId);
  const bool rootFacts =
      root != nullptr &&
      root->kind == cr::CreativeObjectKind::PrefabInstance &&
      !root->visible && root->assetId == "authored_0001";
  const cr::CreativeObjectWorldExtent rootExtent =
      root != nullptr ? cr::resolveCreativeObjectWorldExtent(*root)
                      : cr::CreativeObjectWorldExtent{};
  const cr::CreativeBoundsMetrics sourceMetrics =
      cr::measureCreativeBounds(captured.definition.sourceBounds);
  const cr::CreativeVec3 rotatedSourceCenter =
      cr::rotateCreativeVectorEulerXyz(
          sourceMetrics.center, root != nullptr
                                    ? root->transform.rotationEulerRadians
                                    : cr::CreativeVec3{});
  const cr::CreativeVec3 expectedRootCenter{
      placement.targetAnchor.x + rotatedSourceCenter.x,
      placement.targetAnchor.y + rotatedSourceCenter.y,
      placement.targetAnchor.z + rotatedSourceCenter.z};
  const cr::CreativeVec3 actualRootCenter =
      cr::measureCreativeBounds({rootExtent.min, rootExtent.max}).center;
  const bool childFacts =
      child != nullptr && child->parentId == placed.instanceRootObjectId &&
      selectedRoot == placed.instanceRootObjectId;
  const std::size_t placedObjectCount = target.objectCount();

  const cr::CreativeGroupCommandReceipt unpacked =
      cr::ungroupDocumentObjectAtomically(target,
                                          placed.instanceRootObjectId);
  return expect(placed.accepted && placed.changed &&
                    placedObjectCount == 3U,
                "authored definition instantiates root and expanded children") &&
         expect(rootFacts,
                "instance root carries semantic identity without rendering") &&
         expect(rootExtent.valid &&
                    std::fabs(actualRootCenter.x - expectedRootCenter.x) <
                        1.0e-9 &&
                    std::fabs(actualRootCenter.y - expectedRootCenter.y) <
                        1.0e-9 &&
                    std::fabs(actualRootCenter.z - expectedRootCenter.z) <
                        1.0e-9,
                "instance root resolves local bounds through one transform") &&
         expect(childFacts,
                "visible child resolves to the instance interaction root") &&
         expect(unpacked.accepted && unpacked.changed &&
                    target.findObject(placed.instanceRootObjectId) == nullptr &&
                    target.objectCount() == 2U &&
                    !target.findObject(childId)->parentId.has_value(),
                "unpack removes only the instance container");
}

bool invalidDefinitionCannotPartiallyMutate() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  captured.definition.rootObjectIds.push_back(999'999U);
  cr::CreativeDocument target = cr::CreativeDocument::create("Atomic");
  static_cast<void>(target.assignId(704U));
  const std::uint64_t revisionBefore = target.revision();
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = &captured.definition;
  placement.targetAnchor = {1.0, 0.0, 1.0};
  const cr::CreativeAuthoredAssetInstanceReceipt rejected =
      cr::instantiateCreativeAuthoredAssetAtomically(target, placement);
  return expect(!rejected.accepted && !rejected.changed &&
                    target.objectCount() == 0U &&
                    target.revision() == revisionBefore,
                "missing source-root remap rejects the complete transaction");
}

bool refreshAllInstancesIsAtomic() {
  const cr::CreativeAuthoredAssetCaptureResult original =
      makeTwoCrateDefinition();
  cr::CreativeDocument updatedSource =
      cr::CreativeDocument::create("Updated Source");
  static_cast<void>(updatedSource.assignId(716U));
  const cr::CreativeObjectId left =
      createCrate(updatedSource, "Left", {10.0, 0.0, 20.0});
  const cr::CreativeObjectId right =
      createCrate(updatedSource, "Right", {12.0, 0.0, 20.0});
  const cr::CreativeObjectId detail =
      createCrate(updatedSource, "Detail", {11.0, 2.0, 20.0});
  const std::array updatedSelection{left, right, detail};
  const cr::CreativeAuthoredAssetCaptureResult updated =
      cr::captureCreativeAuthoredAsset(
          {&updatedSource, updatedSelection, "authored_0001", "Twin Crates",
           717U});

  cr::CreativeDocument target = cr::CreativeDocument::create("Refresh");
  static_cast<void>(target.assignId(718U));
  cr::CreativeDocumentCreateRequest groupRequest;
  groupRequest.kind = cr::CreativeObjectKind::Group;
  groupRequest.name = "Parent";
  const cr::CreativeObjectId parentId = target.createObject(groupRequest).objectId;
  cr::CreativeAuthoredAssetPlacementRequest firstPlacement;
  firstPlacement.definition = &original.definition;
  firstPlacement.targetAnchor = {5.0, 1.0, -3.0};
  firstPlacement.yawRadians = 1.5707963267948966;
  firstPlacement.parentId = parentId;
  const cr::CreativeAuthoredAssetInstanceReceipt first =
      cr::instantiateCreativeAuthoredAssetAtomically(target, firstPlacement);
  cr::CreativeAuthoredAssetPlacementRequest secondPlacement;
  secondPlacement.definition = &original.definition;
  secondPlacement.targetAnchor = {-7.0, 3.0, 9.0};
  secondPlacement.yawRadians = -0.5;
  const cr::CreativeAuthoredAssetInstanceReceipt second =
      cr::instantiateCreativeAuthoredAssetAtomically(target, secondPlacement);
  if (!expect(original.accepted && updated.accepted && first.accepted &&
                  second.accepted && parentId != cr::kInvalidObjectId,
              "refresh fixtures are valid")) {
    return false;
  }
  const cr::CreativeObject firstRootBefore =
      *target.findObject(first.instanceRootObjectId);
  const cr::CreativeObject secondRootBefore =
      *target.findObject(second.instanceRootObjectId);
  std::vector<cr::CreativeObjectId> oldChildIds = first.instanceObjectIds;
  oldChildIds.insert(oldChildIds.end(), second.instanceObjectIds.begin(),
                     second.instanceObjectIds.end());

  const cr::CreativeAuthoredAssetRefreshReceipt refreshed =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(target,
                                                          updated.definition);
  const cr::CreativeObject* firstRootAfter =
      target.findObject(first.instanceRootObjectId);
  const cr::CreativeObject* secondRootAfter =
      target.findObject(second.instanceRootObjectId);
  const cr::CreativeHierarchySelection firstHierarchy =
      cr::resolveCreativeObjectHierarchy(
          target, std::span{&first.instanceRootObjectId, 1U});
  const cr::CreativeHierarchySelection secondHierarchy =
      cr::resolveCreativeObjectHierarchy(
          target, std::span{&second.instanceRootObjectId, 1U});
  const bool oldChildrenRemoved =
      std::all_of(oldChildIds.begin(), oldChildIds.end(),
                  [&target](cr::CreativeObjectId objectId) {
                    return !target.containsObject(objectId);
                  });
  cr::CreativeAuthoredAssetPlacementRequest firstUpdatedPlacement =
      firstPlacement;
  firstUpdatedPlacement.definition = &updated.definition;
  const cr::CreativeAuthoredAssetPlacementPlan expectedFirst =
      cr::planCreativeAuthoredAssetPlacement(firstUpdatedPlacement);
  const bool rootsPreserved =
      firstRootAfter != nullptr && secondRootAfter != nullptr &&
      firstRootAfter->id == firstRootBefore.id &&
      secondRootAfter->id == secondRootBefore.id &&
      firstRootAfter->name == firstRootBefore.name &&
      secondRootAfter->name == secondRootBefore.name &&
      firstRootAfter->parentId == firstRootBefore.parentId &&
      secondRootAfter->parentId == secondRootBefore.parentId &&
      vecNear(firstRootAfter->transform.position,
              firstRootBefore.transform.position) &&
      vecNear(firstRootAfter->transform.rotationEulerRadians,
              firstRootBefore.transform.rotationEulerRadians) &&
      vecNear(secondRootAfter->transform.position,
              secondRootBefore.transform.position) &&
      vecNear(secondRootAfter->transform.rotationEulerRadians,
              secondRootBefore.transform.rotationEulerRadians) &&
      expectedFirst.accepted &&
      cr::creativeBoundsExactlyEqual(firstRootAfter->bounds,
                                     expectedFirst.rootRequest.bounds);

  const cr::CreativeObjectId lockedChildId =
      firstHierarchy.objectIds.size() > 1U ? firstHierarchy.objectIds.back()
                                          : cr::kInvalidObjectId;
  const cr::CreativeDocumentMutationReceipt locked =
      cr::setDocumentObjectLocked(target, lockedChildId, true);
  const std::uint64_t lockedRevision = target.revision();
  const std::size_t lockedObjectCount = target.objectCount();
  const cr::CreativeAuthoredAssetRefreshReceipt lockRejected =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(target,
                                                          updated.definition);
  const bool lockWasAtomic =
      cr::documentMutationChanged(locked.status) && !lockRejected.accepted &&
      lockRejected.status ==
          cr::CreativeAuthoredAssetRefreshStatus::LockedObject &&
      lockRejected.refreshedInstanceCount == 0U &&
      lockRejected.removedObjectCount == 0U &&
      lockRejected.createdObjectCount == 0U &&
      target.revision() == lockedRevision &&
      target.objectCount() == lockedObjectCount &&
      target.containsObject(lockedChildId);

  static_cast<void>(cr::setDocumentObjectLocked(target, lockedChildId, false));
  const cr::CreativeDocumentMutationReceipt scaled = cr::applyDocumentMutation(
      target, second.instanceRootObjectId, cr::CreativeMutationKind::Scale,
      cr::CreativeMutationPayload{cr::ScaleMutation{{2.0, 1.0, 1.0}}});
  const std::uint64_t scaledRevision = target.revision();
  const std::size_t scaledObjectCount = target.objectCount();
  const cr::CreativeAuthoredAssetRefreshReceipt scaleRejected =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(target,
                                                          updated.definition);
  const bool scaleWasAtomic =
      cr::documentMutationChanged(scaled.status) && !scaleRejected.accepted &&
      scaleRejected.status ==
          cr::CreativeAuthoredAssetRefreshStatus::UnsupportedInstance &&
      target.revision() == scaledRevision &&
      target.objectCount() == scaledObjectCount;

  cr::CreativeAuthoredAssetDefinition recursive = updated.definition;
  recursive.content.objects.front().kind =
      cr::CreativeObjectKind::PrefabInstance;
  recursive.content.objects.front().assetId = recursive.assetId;
  const std::uint64_t recursiveRevision = target.revision();
  const cr::CreativeAuthoredAssetRefreshReceipt recursionRejected =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(target, recursive);
  cr::CreativeDocument empty = cr::CreativeDocument::create("No Instances");
  static_cast<void>(empty.assignId(719U));
  const cr::CreativeAuthoredAssetRefreshReceipt noMatches =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(empty,
                                                          updated.definition);

  return expect(refreshed.accepted && refreshed.changed &&
                    refreshed.status ==
                        cr::CreativeAuthoredAssetRefreshStatus::Refreshed &&
                    refreshed.matchedInstanceCount == 2U &&
                    refreshed.refreshedInstanceCount == 2U &&
                    refreshed.removedObjectCount == 4U &&
                    refreshed.createdObjectCount == 6U,
                "refresh replaces every matching instance in one receipt") &&
         expect(firstHierarchy.accepted && secondHierarchy.accepted &&
                    firstHierarchy.objectIds.size() == 4U &&
                    secondHierarchy.objectIds.size() == 4U &&
                    oldChildrenRemoved && rootsPreserved,
                "refresh preserves roots and replaces only descendants") &&
         expect(lockWasAtomic,
                "locked descendants reject the complete refresh") &&
         expect(scaleWasAtomic,
                "unsupported instance transforms reject the complete refresh") &&
         expect(!recursionRejected.accepted &&
                    recursionRejected.status ==
                        cr::CreativeAuthoredAssetRefreshStatus::InvalidDefinition &&
                    target.revision() == recursiveRevision,
                "self-recursive definitions fail before mutation") &&
         expect(!noMatches.accepted &&
                    noMatches.status ==
                        cr::CreativeAuthoredAssetRefreshStatus::NoMatchingInstances &&
                    empty.objectCount() == 0U,
                "refresh reports when no placed instances match");
}

bool syncStatesProtectLocalInstanceEdits() {
  const cr::CreativeAuthoredAssetCaptureResult original =
      makeTwoCrateDefinition();
  cr::CreativeDocument updatedSource =
      cr::CreativeDocument::create("Updated Sync Source");
  static_cast<void>(updatedSource.assignId(720U));
  const cr::CreativeObjectId left =
      createCrate(updatedSource, "Left", {10.0, 0.0, 20.0});
  const cr::CreativeObjectId right =
      createCrate(updatedSource, "Right", {12.0, 0.0, 20.0});
  const cr::CreativeObjectId detail =
      createCrate(updatedSource, "Detail", {11.0, 2.0, 20.0});
  const std::array updatedSelection{left, right, detail};
  const cr::CreativeAuthoredAssetCaptureResult updated =
      cr::captureCreativeAuthoredAsset(
          {&updatedSource, updatedSelection, "authored_0001", "Twin Crates",
           721U});

  cr::CreativeDocument target = cr::CreativeDocument::create("Sync States");
  static_cast<void>(target.assignId(722U));
  const auto place = [&target](const cr::CreativeAuthoredAssetDefinition& source,
                               cr::CreativeVec3 position,
                               double yaw) {
    cr::CreativeAuthoredAssetPlacementRequest request;
    request.definition = &source;
    request.targetAnchor = position;
    request.yawRadians = yaw;
    return cr::instantiateCreativeAuthoredAssetAtomically(target, request);
  };
  const cr::CreativeAuthoredAssetInstanceReceipt first =
      place(original.definition, {4.0, 0.0, 8.0}, 1.5707963267948966);
  const cr::CreativeAuthoredAssetInstanceReceipt second =
      place(original.definition, {14.0, 0.0, 8.0}, 0.0);
  const cr::CreativeAuthoredAssetInstanceReceipt third =
      place(original.definition, {24.0, 0.0, 8.0}, -0.5);
  const cr::CreativeAuthoredAssetInstanceReceipt fourth =
      place(updated.definition, {34.0, 0.0, 8.0}, 0.25);
  const cr::CreativeObjectId secondLocal =
      createCrate(target, "Second Local", {14.0, 1.0, 8.0},
                  second.instanceRootObjectId);
  const cr::CreativeObjectId thirdLocal =
      createCrate(target, "Third Local", {24.0, 1.0, 8.0},
                  third.instanceRootObjectId);
  const cr::CreativeDocumentMutationReceipt locked =
      cr::setDocumentObjectLocked(target, thirdLocal, true);
  if (!expect(original.accepted && updated.accepted && first.accepted &&
                  second.accepted && third.accepted && fourth.accepted &&
                  secondLocal != cr::kInvalidObjectId &&
                  thirdLocal != cr::kInvalidObjectId &&
                  cr::documentMutationChanged(locked.status),
              "sync fixture creates current, stale, and edited instances")) {
    return false;
  }

  const cr::CreativeAuthoredAssetSyncReceipt sourceChanged =
      cr::inspectCreativeAuthoredAssetInstanceSync(
          target, updated.definition, first.instanceRootObjectId);
  const cr::CreativeAuthoredAssetSyncReceipt locallyModified =
      cr::inspectCreativeAuthoredAssetInstanceSync(
          target, original.definition, second.instanceRootObjectId);
  const cr::CreativeAuthoredAssetSyncReceipt conflict =
      cr::inspectCreativeAuthoredAssetInstanceSync(
          target, updated.definition, second.instanceRootObjectId);
  const cr::CreativeAuthoredAssetSyncReceipt current =
      cr::inspectCreativeAuthoredAssetInstanceSync(
          target, updated.definition, fourth.instanceRootObjectId);
  const cr::CreativeAuthoredAssetSyncSummary before =
      cr::summarizeCreativeAuthoredAssetSync(target, updated.definition);
  const cr::CreativeObject* firstRoot =
      target.findObject(first.instanceRootObjectId);
  const bool provenanceStored =
      firstRoot != nullptr &&
      cr::creativeAuthoredAssetStoredSourceFingerprint(*firstRoot).has_value();

  cr::CreativeAuthoredAssetRefreshRequest safeRequest;
  safeRequest.definition = &updated.definition;
  safeRequest.mode = cr::CreativeAuthoredAssetRefreshMode::SafeInstances;
  const cr::CreativeAuthoredAssetRefreshReceipt safe =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(target, safeRequest);
  const bool safeProtectedEdits =
      safe.accepted && safe.refreshedInstanceCount == 1U &&
      safe.instanceRootObjectIds.size() == 1U &&
      safe.instanceRootObjectIds.front() == first.instanceRootObjectId &&
      target.containsObject(secondLocal) && target.containsObject(thirdLocal);

  cr::CreativeAuthoredAssetRefreshRequest selectedRequest;
  selectedRequest.definition = &updated.definition;
  selectedRequest.mode =
      cr::CreativeAuthoredAssetRefreshMode::SelectedInstance;
  selectedRequest.selectedInstanceRootObjectId = second.instanceRootObjectId;
  const cr::CreativeAuthoredAssetRefreshReceipt selected =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(target,
                                                          selectedRequest);
  const bool selectedOnly =
      selected.accepted && selected.refreshedInstanceCount == 1U &&
      !target.containsObject(secondLocal) && target.containsObject(thirdLocal);

  cr::CreativeAuthoredAssetRefreshRequest forceRequest;
  forceRequest.definition = &updated.definition;
  forceRequest.mode = cr::CreativeAuthoredAssetRefreshMode::ForceAll;
  const std::uint64_t revisionBeforeRejectedForce = target.revision();
  const cr::CreativeAuthoredAssetRefreshReceipt rejectedForce =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(target,
                                                          forceRequest);
  const bool forceRejectedAtomically =
      !rejectedForce.accepted &&
      rejectedForce.status ==
          cr::CreativeAuthoredAssetRefreshStatus::LockedObject &&
      target.revision() == revisionBeforeRejectedForce &&
      target.containsObject(thirdLocal);
  static_cast<void>(cr::setDocumentObjectLocked(target, thirdLocal, false));
  const cr::CreativeAuthoredAssetRefreshReceipt forced =
      cr::refreshCreativeAuthoredAssetInstancesAtomically(target,
                                                          forceRequest);
  const cr::CreativeAuthoredAssetSyncSummary after =
      cr::summarizeCreativeAuthoredAssetSync(target, updated.definition);

  return expect(sourceChanged.accepted &&
                    sourceChanged.state ==
                        cr::CreativeAuthoredAssetSyncState::SourceChanged &&
                    locallyModified.accepted &&
                    locallyModified.state ==
                        cr::CreativeAuthoredAssetSyncState::LocallyModified &&
                    conflict.accepted &&
                    conflict.state ==
                        cr::CreativeAuthoredAssetSyncState::Conflict &&
                    current.accepted &&
                    current.state ==
                        cr::CreativeAuthoredAssetSyncState::Current,
                "sync classifier exposes all four source/local states") &&
         expect(provenanceStored && before.accepted &&
                    before.matchedInstanceCount == 4U &&
                    before.currentInstanceCount == 1U &&
                    before.sourceChangedInstanceCount == 1U &&
                    before.locallyModifiedInstanceCount == 0U &&
                    before.conflictInstanceCount == 2U,
                "placement stores provenance and summary counts each state") &&
         expect(safeProtectedEdits,
                "safe refresh updates only source-changed instances") &&
         expect(selectedOnly,
                "selected refresh overwrites only the requested instance") &&
         expect(forceRejectedAtomically,
                "force refresh still rolls back when any target is locked") &&
         expect(forced.accepted && forced.refreshedInstanceCount == 4U &&
                    !target.containsObject(thirdLocal) && after.accepted &&
                    after.currentInstanceCount == 4U &&
                    after.sourceChangedInstanceCount == 0U &&
                    after.locallyModifiedInstanceCount == 0U &&
                    after.conflictInstanceCount == 0U,
                "force refresh overwrites conflicts and restores current state");
}

bool definitionsAreBoundedAndFailClosed() {
  cr::CreativeDocument oversized = cr::CreativeDocument::create("Oversized");
  static_cast<void>(oversized.assignId(708U));
  for (std::size_t index = 0U;
       index <= cr::kCreativeAuthoredAssetObjectCapacity; ++index) {
    static_cast<void>(createCrate(
        oversized, "Part", {static_cast<double>(index), 0.0, 0.0}));
  }
  const cr::CreativeAuthoredAssetLoadResult capacity =
      cr::loadCreativeAuthoredAssetDefinition(
          oversized, "authored_oversized", "Oversized");

  cr::CreativeDocument valid = cr::CreativeDocument::create("Valid");
  static_cast<void>(valid.assignId(709U));
  static_cast<void>(createCrate(valid, "Part", {}));
  const cr::CreativeAuthoredAssetLoadResult identity =
      cr::loadCreativeAuthoredAssetDefinition(valid, "invalid id", "Valid");
  cr::CreativeAuthoredAssetLoadResult loaded =
      cr::loadCreativeAuthoredAssetDefinition(
          valid, "authored_valid", "Valid");
  cr::CreativeAuthoredAssetPlacementRequest request;
  request.definition = &loaded.definition;
  request.targetAnchor.x = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeAuthoredAssetPlacementPlan placement =
      cr::planCreativeAuthoredAssetPlacement(request);

  return expect(!capacity.accepted &&
                    capacity.status ==
                        cr::CreativeAuthoredAssetStatus::CapacityExceeded,
                "definitions reject more than 256 expanded objects") &&
         expect(!identity.accepted &&
                    identity.status ==
                        cr::CreativeAuthoredAssetStatus::InvalidIdentity,
                "durable identifiers reject unsupported characters") &&
         expect(loaded.accepted && !placement.accepted &&
                    placement.status ==
                        cr::CreativeAuthoredAssetStatus::InvalidGeometry,
                "non-finite placement fails before mutation");
}

bool transformedInstanceUpdateFailsClosed() {
  const cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  cr::CreativeDocument target = cr::CreativeDocument::create("Scaled");
  static_cast<void>(target.assignId(714U));
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = &captured.definition;
  const cr::CreativeAuthoredAssetInstanceReceipt placed =
      cr::instantiateCreativeAuthoredAssetAtomically(target, placement);
  const cr::CreativeDocumentMutationReceipt scaled = cr::applyDocumentMutation(
      target, placed.instanceRootObjectId, cr::CreativeMutationKind::Scale,
      cr::CreativeMutationPayload{
          cr::ScaleMutation{{2.0, 1.0, 1.0}}});
  const cr::CreativeObject* root =
      target.findObject(placed.instanceRootObjectId);
  cr::CreativeAuthoredAssetInstanceCaptureRequest update;
  update.sourceDocument = &target;
  update.existingDefinition = &captured.definition;
  update.instanceRootObjectId = placed.instanceRootObjectId;
  update.definitionDocumentId = 715U;
  const cr::CreativeAuthoredAssetCaptureResult rejected =
      cr::captureCreativeAuthoredAssetInstance(update);
  return expect(placed.accepted &&
                    cr::documentMutationChanged(scaled.status) &&
                    root != nullptr &&
                    !cr::creativeAuthoredAssetInstanceTransformSupported(
                        *root) &&
                    !rejected.accepted &&
                    rejected.status ==
                        cr::CreativeAuthoredAssetStatus::InvalidGeometry,
                "scaled wrappers reject source update without partial output");
}

bool durableLibraryRoundTripsSelection() {
  const auto nonce = std::chrono::steady_clock::now()
                         .time_since_epoch()
                         .count();
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_authored_asset_test_" + std::to_string(nonce));
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Library");
  static_cast<void>(document.assignId(705U));
  const cr::CreativeObjectId objectId =
      createCrate(document, "Pillar", {4.0, 0.0, 8.0});
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "library test document installed")) {
    return false;
  }
  selectOnly(appState.facade, objectId);
  app::CreativeEditorAuthoredAssetLibrary library;
  const app::CreativeEditorAuthoredAssetLoadReceipt initialized =
      app::loadCreativeEditorAuthoredAssetLibrary(library, root);
  const app::CreativeEditorAuthoredAssetSaveReceipt saved =
      app::saveCreativeEditorSelectionAsAuthoredAsset(appState, library);
  app::CreativeEditorState editor;
  editor.authoredAssets = std::move(library);
  editor.catalog.model = cr::makeCreativeCatalog(
      std::span<const cr::CreativeObjectKind>{});
  editor.toolOptions.open = true;
  editor.toolOptions.commands =
      app::creativeEditorToolOptionCommandsForEntry(
          {cr::CreativeHeldItemKind::ObjectMove,
           cr::CreativeObjectKind::Unknown});
  app::refreshCreativeEditorObjectActionContext(
      appState, editor.authoredAssets, editor.toolOptions);
  const auto saveCommand = std::find(
      editor.toolOptions.commands.ids.begin(),
      editor.toolOptions.commands.ids.begin() +
          editor.toolOptions.commands.count,
      app::CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset);
  editor.toolOptions.selectedIndex = static_cast<std::size_t>(
      saveCommand - editor.toolOptions.commands.ids.begin());
  const bool commandSaved =
      saveCommand != editor.toolOptions.commands.ids.begin() +
                         editor.toolOptions.commands.count &&
      app::activateCreativeEditorToolOptionsSelection(appState, editor);
  const std::string equippedAssetId(
      cr::creativeHotbarAssetId(cr::selectedCreativeHotbarEntry(
          editor.interaction.hotbar)));
  const bool catalogPublished = std::any_of(
      editor.catalog.model.entries.begin(),
      editor.catalog.model.entries.end(),
      [&equippedAssetId](const cr::CreativeCatalogEntry& entry) {
        return entry.authoredComposite &&
               cr::creativeHotbarAssetId(entry.hotbarEntry) ==
                   equippedAssetId;
      });
  app::CreativeEditorAuthoredAssetLibrary reloaded;
  const app::CreativeEditorAuthoredAssetLoadReceipt loaded =
      app::loadCreativeEditorAuthoredAssetLibrary(reloaded, root);
  const cr::CreativeAuthoredAssetDefinition* definition =
      app::findCreativeEditorAuthoredAsset(reloaded, saved.assetId);
  const cr::CreativeAuthoredAssetDefinition* commandDefinition =
      app::findCreativeEditorAuthoredAsset(reloaded, equippedAssetId);
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);

  return expect(initialized.accepted && saved.accepted &&
                    saved.durableWriteOk &&
                    saved.capture.definition.assetId == saved.assetId &&
                    saved.capture.definition.content.objects.size() == 1U,
                "durable save preserves complete capture receipt facts") &&
         expect(commandSaved && equippedAssetId == "authored_0002" &&
                    catalogPublished &&
                    cr::selectedCreativeHotbarEntry(
                        editor.interaction.hotbar)
                            .objectKind ==
                        cr::CreativeObjectKind::PrefabInstance,
                "save command publishes and equips the authored asset") &&
         expect(loaded.accepted && loaded.loadedCount == 2U &&
                    definition != nullptr && definition->label == "Pillar" &&
                    definition->content.objects.size() == 1U &&
                    commandDefinition != nullptr,
                "startup scan reconstructs the authored definition");
}

bool updateCommandRoundTripsEditedInstance() {
  const auto nonce = std::chrono::steady_clock::now()
                         .time_since_epoch()
                         .count();
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_authored_asset_update_test_" + std::to_string(nonce));
  cr::CreativeAppState sourceState;
  cr::CreativeDocument source = cr::CreativeDocument::create("Source");
  static_cast<void>(source.assignId(711U));
  const cr::CreativeObjectId sourceObjectId =
      createCrate(source, "Pillar", {});
  if (!expect(sourceState.facade.installDocument(std::move(source)).accepted,
              "update source document installed")) {
    return false;
  }
  selectOnly(sourceState.facade, sourceObjectId);
  app::CreativeEditorAuthoredAssetLibrary library;
  const app::CreativeEditorAuthoredAssetLoadReceipt initialized =
      app::loadCreativeEditorAuthoredAssetLibrary(library, root);
  const app::CreativeEditorAuthoredAssetSaveReceipt saved =
      app::saveCreativeEditorSelectionAsAuthoredAsset(sourceState, library);
  const cr::CreativeAuthoredAssetDefinition* savedDefinition =
      app::findCreativeEditorAuthoredAsset(library, saved.assetId);
  if (!expect(initialized.accepted && saved.accepted &&
                  savedDefinition != nullptr,
              "update source asset initialized")) {
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return false;
  }
  const cr::CreativeAuthoredAssetDefinition originalDefinition =
      *savedDefinition;

  cr::CreativeDocument instances = cr::CreativeDocument::create("Instances");
  static_cast<void>(instances.assignId(712U));
  cr::CreativeAuthoredAssetPlacementRequest firstPlacement;
  firstPlacement.definition = savedDefinition;
  firstPlacement.targetAnchor = {10.0, 0.0, 10.0};
  firstPlacement.yawRadians = 1.5707963267948966;
  const cr::CreativeAuthoredAssetInstanceReceipt first =
      cr::instantiateCreativeAuthoredAssetAtomically(instances,
                                                     firstPlacement);
  cr::CreativeAuthoredAssetPlacementRequest secondPlacement;
  secondPlacement.definition = savedDefinition;
  secondPlacement.targetAnchor = {20.0, 0.0, 20.0};
  const cr::CreativeAuthoredAssetInstanceReceipt second =
      cr::instantiateCreativeAuthoredAssetAtomically(instances,
                                                     secondPlacement);
  const cr::CreativeObjectId addedObjectId = createCrate(
      instances, "Added Detail", {10.0, 0.0, 12.0},
      first.instanceRootObjectId);
  cr::CreativeAppState appState;
  if (!expect(first.accepted && second.accepted &&
                  addedObjectId != cr::kInvalidObjectId &&
                  appState.facade.installDocument(std::move(instances)).accepted,
              "two instances and one local edit installed")) {
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return false;
  }
  selectOnly(appState.facade, first.instanceRootObjectId);

  app::CreativeEditorState editor;
  editor.authoredAssets = std::move(library);
  const std::vector<cr::CreativeCatalogAsset> catalogAssets =
      app::creativeEditorAuthoredAssetCatalogEntries(editor.authoredAssets);
  editor.catalog.model = cr::makeCreativeCatalog(
      std::span<const cr::CreativeObjectKind>{}, catalogAssets);
  editor.interaction.hotbar.selectedSlot = 0U;
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  held = {cr::CreativeHeldItemKind::Material,
          cr::CreativeObjectKind::PrefabInstance};
  static_cast<void>(cr::setCreativeHotbarAsset(
      held, originalDefinition.assetId, originalDefinition.sourceBounds));
  editor.toolOptions.open = true;
  editor.toolOptions.commands =
      app::creativeEditorToolOptionCommandsForEntry(
          {cr::CreativeHeldItemKind::ObjectGroup,
           cr::CreativeObjectKind::Unknown});
  app::refreshCreativeEditorObjectActionContext(
      appState, editor.authoredAssets, editor.toolOptions);
  const auto updateCommand = std::find(
      editor.toolOptions.commands.ids.begin(),
      editor.toolOptions.commands.ids.begin() +
          editor.toolOptions.commands.count,
      app::CreativeEditorToolOptionsCommandId::UpdateSavedAsset);
  editor.toolOptions.selectedIndex = static_cast<std::size_t>(
      updateCommand - editor.toolOptions.commands.ids.begin());
  const std::uint64_t revisionBeforeUpdate =
      appState.facade.document().revision();
  const bool updated =
      updateCommand != editor.toolOptions.commands.ids.begin() +
                           editor.toolOptions.commands.count &&
      app::activateCreativeEditorToolOptionsSelection(appState, editor);

  const cr::CreativeAuthoredAssetDefinition* updatedDefinition =
      app::findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                          originalDefinition.assetId);
  const cr::CreativeHierarchySelection untouchedFirst =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&first.instanceRootObjectId, 1U});
  const cr::CreativeHierarchySelection untouchedSecond =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&second.instanceRootObjectId, 1U});
  const bool updateDidNotRewriteInstances =
      appState.facade.document().revision() == revisionBeforeUpdate + 1U &&
      untouchedFirst.accepted && untouchedFirst.objectIds.size() == 3U &&
      appState.facade.document().containsObject(addedObjectId) &&
      untouchedSecond.accepted && untouchedSecond.objectIds.size() == 2U;
  const cr::CreativeAuthoredAssetFingerprint updatedFingerprint =
      updatedDefinition == nullptr
          ? cr::CreativeAuthoredAssetFingerprint{}
          : cr::fingerprintCreativeAuthoredAssetDefinition(*updatedDefinition);
  const cr::CreativeObject* acknowledgedRoot =
      appState.facade.findObject(first.instanceRootObjectId);
  cr::CreativeAuthoredAssetPlacementRequest acknowledgedPlacement;
  acknowledgedPlacement.definition = updatedDefinition;
  if (acknowledgedRoot != nullptr) {
    acknowledgedPlacement.targetAnchor = acknowledgedRoot->transform.position;
    acknowledgedPlacement.yawRadians =
        acknowledgedRoot->transform.rotationEulerRadians.y;
    acknowledgedPlacement.parentId = acknowledgedRoot->parentId;
  }
  const cr::CreativeAuthoredAssetPlacementPlan acknowledgedPlan =
      cr::planCreativeAuthoredAssetPlacement(acknowledgedPlacement);
  const bool updateAcknowledgedSource =
      updatedFingerprint.valid && acknowledgedRoot != nullptr &&
      cr::creativeAuthoredAssetStoredSourceFingerprint(*acknowledgedRoot) ==
          std::optional<std::uint64_t>{updatedFingerprint.value} &&
      acknowledgedPlan.accepted &&
      cr::creativeBoundsExactlyEqual(acknowledgedRoot->bounds,
                                     acknowledgedPlan.rootRequest.bounds);
  const cr::CreativeObject* originalSourceObject =
      clipboardObjectNamed(originalDefinition.content, "Pillar");
  const cr::CreativeObject* updatedSourceObject =
      updatedDefinition == nullptr
          ? nullptr
          : clipboardObjectNamed(updatedDefinition->content, "Pillar");
  const bool sourceFramePreserved =
      originalSourceObject != nullptr && updatedSourceObject != nullptr &&
      vecNear(originalSourceObject->transform.position,
              updatedSourceObject->transform.position) &&
      vecNear(originalSourceObject->transform.rotationEulerRadians,
              updatedSourceObject->transform.rotationEulerRadians);
  const bool hotbarRefreshed =
      updatedDefinition != nullptr && held.hasAssetBounds &&
      cr::creativeBoundsExactlyEqual(held.assetSourceBounds,
                                     updatedDefinition->sourceBounds);
  const bool catalogRefreshed =
      updatedDefinition != nullptr &&
      std::any_of(editor.catalog.model.entries.begin(),
                  editor.catalog.model.entries.end(),
                  [updatedDefinition](const cr::CreativeCatalogEntry& entry) {
                    return entry.authoredComposite &&
                           cr::creativeHotbarAssetId(entry.hotbarEntry) ==
                               updatedDefinition->assetId &&
                           cr::creativeBoundsExactlyEqual(
                               entry.hotbarEntry.assetSourceBounds,
                               updatedDefinition->sourceBounds);
                  });

  const cr::CreativeObject firstRootBeforeRefresh =
      *appState.facade.findObject(first.instanceRootObjectId);
  const cr::CreativeObject secondRootBeforeRefresh =
      *appState.facade.findObject(second.instanceRootObjectId);
  std::vector<cr::CreativeObjectId> staleChildIds = first.instanceObjectIds;
  staleChildIds.insert(staleChildIds.end(), second.instanceObjectIds.begin(),
                       second.instanceObjectIds.end());
  staleChildIds.push_back(addedObjectId);
  const std::size_t objectCountBeforeRefresh =
      appState.facade.document().objectCount();
  const std::uint64_t undoDepthBeforeRefresh =
      cr::creativeUndoDepth(appState.history);
  editor.toolOptions.open = true;
  editor.toolOptions.commands =
      app::creativeEditorToolOptionCommandsForEntry(
          {cr::CreativeHeldItemKind::ObjectGroup,
           cr::CreativeObjectKind::Unknown});
  app::refreshCreativeEditorObjectActionContext(
      appState, editor.authoredAssets, editor.toolOptions);
  const auto refreshCommand = std::find(
      editor.toolOptions.commands.ids.begin(),
      editor.toolOptions.commands.ids.begin() +
          editor.toolOptions.commands.count,
      app::CreativeEditorToolOptionsCommandId::ForceRefreshSavedAssetInstances);
  editor.toolOptions.selectedIndex = static_cast<std::size_t>(
      refreshCommand - editor.toolOptions.commands.ids.begin());
  const bool instancesRefreshed =
      refreshCommand != editor.toolOptions.commands.ids.begin() +
                            editor.toolOptions.commands.count &&
      app::activateCreativeEditorToolOptionsSelection(appState, editor);
  const cr::CreativeHierarchySelection refreshedFirst =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&first.instanceRootObjectId, 1U});
  const cr::CreativeHierarchySelection refreshedSecond =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&second.instanceRootObjectId, 1U});
  const bool staleChildrenRemoved =
      std::all_of(staleChildIds.begin(), staleChildIds.end(),
                  [&appState](cr::CreativeObjectId objectId) {
                    return !appState.facade.document().containsObject(objectId);
                  });
  const cr::CreativeObject* firstRootAfterRefresh =
      appState.facade.findObject(first.instanceRootObjectId);
  const cr::CreativeObject* secondRootAfterRefresh =
      appState.facade.findObject(second.instanceRootObjectId);
  const bool rootsPreserved =
      firstRootAfterRefresh != nullptr && secondRootAfterRefresh != nullptr &&
      vecNear(firstRootAfterRefresh->transform.position,
              firstRootBeforeRefresh.transform.position) &&
      vecNear(firstRootAfterRefresh->transform.rotationEulerRadians,
              firstRootBeforeRefresh.transform.rotationEulerRadians) &&
      vecNear(secondRootAfterRefresh->transform.position,
              secondRootBeforeRefresh.transform.position) &&
      vecNear(secondRootAfterRefresh->transform.rotationEulerRadians,
              secondRootBeforeRefresh.transform.rotationEulerRadians);
  const bool refreshRecordedOnce =
      cr::creativeUndoDepth(appState.history) == undoDepthBeforeRefresh + 1U;
  const bool selectionPreserved =
      appState.facade.selectionState().selectedTarget.value ==
      static_cast<cr::Id>(first.instanceRootObjectId);

  const cr::CreativeHistoryApplyReceipt undoRefresh = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);
  const cr::CreativeHierarchySelection undoFirst =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&first.instanceRootObjectId, 1U});
  const cr::CreativeHierarchySelection undoSecond =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&second.instanceRootObjectId, 1U});
  const bool undoRestoredLocalState =
      undoRefresh.accepted &&
      appState.facade.document().objectCount() == objectCountBeforeRefresh &&
      undoFirst.accepted && undoFirst.objectIds.size() == 3U &&
      undoSecond.accepted && undoSecond.objectIds.size() == 2U &&
      appState.facade.document().containsObject(addedObjectId);
  const cr::CreativeHistoryApplyReceipt redoRefresh = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Redo);
  const cr::CreativeHierarchySelection redoFirst =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&first.instanceRootObjectId, 1U});
  const cr::CreativeHierarchySelection redoSecond =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&second.instanceRootObjectId, 1U});
  const bool redoRestoredRefresh =
      redoRefresh.accepted && redoFirst.accepted && redoSecond.accepted &&
      redoFirst.objectIds.size() == 3U && redoSecond.objectIds.size() == 3U &&
      !appState.facade.document().containsObject(addedObjectId);

  app::CreativeEditorAuthoredAssetLibrary reloaded;
  const app::CreativeEditorAuthoredAssetLoadReceipt loaded =
      app::loadCreativeEditorAuthoredAssetLibrary(reloaded, root);
  const cr::CreativeAuthoredAssetDefinition* durableDefinition =
      app::findCreativeEditorAuthoredAsset(reloaded,
                                          originalDefinition.assetId);
  cr::CreativeDocument replay = cr::CreativeDocument::create("Replay");
  static_cast<void>(replay.assignId(713U));
  cr::CreativeAuthoredAssetPlacementRequest replayPlacement;
  replayPlacement.definition = durableDefinition;
  const cr::CreativeAuthoredAssetInstanceReceipt replayed =
      durableDefinition == nullptr
          ? cr::CreativeAuthoredAssetInstanceReceipt{}
          : cr::instantiateCreativeAuthoredAssetAtomically(replay,
                                                           replayPlacement);
  const cr::CreativeObject* replaySourceObject = nullptr;
  for (cr::CreativeObjectId objectId : replayed.instanceObjectIds) {
    const cr::CreativeObject* object = replay.findObject(objectId);
    if (object != nullptr && object->name == "Pillar") {
      replaySourceObject = object;
      break;
    }
  }
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);

  return expect(updated && updatedDefinition != nullptr &&
                    updatedDefinition->content.objects.size() == 2U,
                "explicit update captures the edited instance contents") &&
         expect(updateDidNotRewriteInstances && updateAcknowledgedSource,
                "source update changes only selected instance metadata") &&
         expect(sourceFramePreserved,
                "rotated instance update removes placement yaw") &&
         expect(hotbarRefreshed && catalogRefreshed,
                "updated source bounds refresh catalog and hotbar facts") &&
         expect(instancesRefreshed && refreshedFirst.accepted &&
                    refreshedSecond.accepted &&
                    refreshedFirst.objectIds.size() == 3U &&
                    refreshedSecond.objectIds.size() == 3U &&
                    staleChildrenRemoved && rootsPreserved &&
                    selectionPreserved,
                "explicit refresh replaces all instances and preserves roots") &&
         expect(refreshRecordedOnce && undoRestoredLocalState &&
                    redoRestoredRefresh,
                "instance refresh is one undoable and redoable edit") &&
         expect(loaded.accepted && loaded.loadedCount == 1U &&
                    durableDefinition != nullptr &&
                    durableDefinition->content.objects.size() == 2U,
                "updated source survives durable reload") &&
         expect(replayed.accepted &&
                    replayed.instanceObjectIds.size() == 2U &&
                    replaySourceObject != nullptr,
                "reloaded source instantiates every edited child") &&
         expect(replaySourceObject != nullptr &&
                    vecNear(replaySourceObject->transform.position,
                            originalSourceObject->transform.position),
                "reloaded placement preserves the source-local frame");
}

bool editorSyncCommandsExposeStatusAndOneUndo() {
  const cr::CreativeAuthoredAssetCaptureResult original =
      makeTwoCrateDefinition();
  cr::CreativeDocument updatedSource =
      cr::CreativeDocument::create("Editor Sync Source");
  static_cast<void>(updatedSource.assignId(723U));
  const cr::CreativeObjectId left =
      createCrate(updatedSource, "Left", {10.0, 0.0, 20.0});
  const cr::CreativeObjectId right =
      createCrate(updatedSource, "Right", {12.0, 0.0, 20.0});
  const cr::CreativeObjectId detail =
      createCrate(updatedSource, "Detail", {11.0, 2.0, 20.0});
  const std::array selected{left, right, detail};
  const cr::CreativeAuthoredAssetCaptureResult updated =
      cr::captureCreativeAuthoredAsset(
          {&updatedSource, selected, "authored_0001", "Twin Crates", 724U});

  cr::CreativeDocument instances =
      cr::CreativeDocument::create("Editor Sync Instances");
  static_cast<void>(instances.assignId(725U));
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = &original.definition;
  const cr::CreativeAuthoredAssetInstanceReceipt first =
      cr::instantiateCreativeAuthoredAssetAtomically(instances, placement);
  placement.targetAnchor = {8.0, 0.0, 0.0};
  const cr::CreativeAuthoredAssetInstanceReceipt second =
      cr::instantiateCreativeAuthoredAssetAtomically(instances, placement);
  const cr::CreativeObjectId localObjectId =
      createCrate(instances, "Local", {8.0, 1.0, 0.0},
                  second.instanceRootObjectId);

  cr::CreativeAppState appState;
  if (!expect(original.accepted && updated.accepted && first.accepted &&
                  second.accepted && localObjectId != cr::kInvalidObjectId &&
                  appState.facade.installDocument(std::move(instances)).accepted,
              "editor sync fixture installed")) {
    return false;
  }
  selectOnly(appState.facade, first.instanceRootObjectId);
  app::CreativeEditorState editor;
  editor.authoredAssets.definitions.push_back(updated.definition);
  app::refreshCreativeEditorObjectActionContext(
      appState, editor.authoredAssets, editor.toolOptions);

  const std::string selectedStatus =
      app::creativeEditorObjectActionValueLabel(
          editor.toolOptions,
          app::CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstance);
  const std::string safeStatus = app::creativeEditorObjectActionValueLabel(
      editor.toolOptions,
      app::CreativeEditorToolOptionsCommandId::
          RefreshSafeSavedAssetInstances);
  const std::string forceStatus = app::creativeEditorObjectActionValueLabel(
      editor.toolOptions,
      app::CreativeEditorToolOptionsCommandId::
          ForceRefreshSavedAssetInstances);
  const bool commandsEnabled =
      app::creativeEditorObjectActionEnabled(
          editor, editor.toolOptions,
          app::CreativeEditorToolOptionsCommandId::RefreshSavedAssetInstance) &&
      app::creativeEditorObjectActionEnabled(
          editor, editor.toolOptions,
          app::CreativeEditorToolOptionsCommandId::
              RefreshSafeSavedAssetInstances) &&
      app::creativeEditorObjectActionEnabled(
          editor, editor.toolOptions,
          app::CreativeEditorToolOptionsCommandId::
              ForceRefreshSavedAssetInstances);

  const std::uint64_t undoBeforeSafe =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeEditorAuthoredAssetInstanceRefreshReceipt safe =
      app::refreshCreativeEditorAuthoredAssetInstances(
          appState, editor.authoredAssets, first.instanceRootObjectId,
          cr::CreativeAuthoredAssetRefreshMode::SafeInstances);
  const bool safeRecordedOnce =
      safe.accepted && safe.refresh.refreshedInstanceCount == 1U &&
      cr::creativeUndoDepth(appState.history) == undoBeforeSafe + 1U &&
      appState.facade.document().containsObject(localObjectId);
  const cr::CreativeHistoryApplyReceipt undoSafe = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  const std::uint64_t undoBeforeSelected =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeEditorAuthoredAssetInstanceRefreshReceipt selectedRefresh =
      app::refreshCreativeEditorAuthoredAssetInstances(
          appState, editor.authoredAssets, second.instanceRootObjectId,
          cr::CreativeAuthoredAssetRefreshMode::SelectedInstance);
  const bool selectedRecordedOnce =
      undoSafe.accepted && selectedRefresh.accepted &&
      selectedRefresh.refresh.refreshedInstanceCount == 1U &&
      cr::creativeUndoDepth(appState.history) == undoBeforeSelected + 1U &&
      !appState.facade.document().containsObject(localObjectId);

  return expect(selectedStatus == "SOURCE CHANGED" &&
                    safeStatus == "1 SOURCE CHANGED" &&
                    forceStatus == "2 INSTANCES" && commandsEnabled,
                "tool options expose cached sync states and refresh choices") &&
         expect(safeRecordedOnce,
                "safe refresh records exactly one history entry") &&
         expect(selectedRecordedOnce,
                "selected refresh records exactly one history entry");
}

bool libraryManagementIsDurableAndReferenceSafe() {
  const auto nonce =
      std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_asset_library_management_" + std::to_string(nonce));
  cr::CreativeAppState sourceState;
  cr::CreativeDocument source = cr::CreativeDocument::create("Library Source");
  static_cast<void>(source.assignId(740U));
  const cr::CreativeObjectId sourceId =
      createCrate(source, "Column", {4.0, 0.0, 7.0});
  if (!expect(sourceState.facade.installDocument(std::move(source)).accepted,
              "asset library source installed")) {
    return false;
  }
  selectOnly(sourceState.facade, sourceId);
  app::CreativeEditorAuthoredAssetLibrary library;
  const app::CreativeEditorAuthoredAssetLoadReceipt initialized =
      app::loadCreativeEditorAuthoredAssetLibrary(library, root);
  const app::CreativeEditorAuthoredAssetSaveReceipt saved =
      app::saveCreativeEditorSelectionAsAuthoredAsset(sourceState, library);
  const cr::CreativeAuthoredAssetDefinition* original =
      app::findCreativeEditorAuthoredAsset(library, saved.assetId);
  if (!expect(initialized.accepted && saved.accepted && original != nullptr,
              "asset library fixture saved")) {
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return false;
  }
  const cr::CreativeAuthoredAssetFingerprint originalFingerprint =
      cr::fingerprintCreativeAuthoredAssetDefinition(*original);

  const app::CreativeEditorAuthoredAssetMutationReceipt renamed =
      app::renameCreativeEditorAuthoredAsset(library, saved.assetId,
                                             "Stone Column");
  const cr::CreativeAuthoredAssetDefinition* renamedDefinition =
      app::findCreativeEditorAuthoredAsset(library, saved.assetId);
  const cr::CreativeAuthoredAssetFingerprint renamedFingerprint =
      renamedDefinition == nullptr
          ? cr::CreativeAuthoredAssetFingerprint{}
          : cr::fingerprintCreativeAuthoredAssetDefinition(*renamedDefinition);
  const bool renamePreservedGeometry =
      renamedDefinition != nullptr &&
      renamedDefinition->assetId == saved.assetId &&
      renamedDefinition->label == "Stone Column" && originalFingerprint.valid &&
      renamedFingerprint.valid &&
      originalFingerprint.value == renamedFingerprint.value;
  const cr::CreativeAuthoredAssetDefinition renamedSnapshot =
      renamedDefinition == nullptr ? cr::CreativeAuthoredAssetDefinition{}
                                   : *renamedDefinition;
  const app::CreativeEditorAuthoredAssetMutationReceipt duplicate =
      app::duplicateCreativeEditorAuthoredAsset(library, saved.assetId);
  renamedDefinition =
      app::findCreativeEditorAuthoredAsset(library, saved.assetId);
  const cr::CreativeAuthoredAssetDefinition* duplicateDefinition =
      app::findCreativeEditorAuthoredAsset(library, duplicate.assetId);
  const bool duplicateGeometryMatches =
      renamedDefinition != nullptr && duplicateDefinition != nullptr &&
      duplicateDefinition->assetId != renamedDefinition->assetId &&
      duplicateDefinition->content.objects.size() ==
          renamedSnapshot.content.objects.size() &&
      cr::creativeBoundsExactlyEqual(duplicateDefinition->sourceBounds,
                                     renamedSnapshot.sourceBounds);

  cr::CreativeDocument map = cr::CreativeDocument::create("Reference Map");
  static_cast<void>(map.assignId(741U));
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = renamedDefinition;
  const cr::CreativeAuthoredAssetInstanceReceipt instance =
      cr::instantiateCreativeAuthoredAssetAtomically(map, placement);
  cr::CreativeAuthoredAssetDefinition* dependency = nullptr;
  for (cr::CreativeAuthoredAssetDefinition& definition : library.definitions) {
    if (definition.assetId == duplicate.assetId) {
      dependency = &definition;
      break;
    }
  }
  if (dependency != nullptr && !dependency->content.objects.empty()) {
    dependency->content.objects.front().kind =
        cr::CreativeObjectKind::PrefabInstance;
    dependency->content.objects.front().assetId = saved.assetId;
  }
  const app::CreativeEditorAuthoredAssetReferenceSummary references =
      app::summarizeCreativeEditorAuthoredAssetReferences(map, library,
                                                          saved.assetId);
  const app::CreativeEditorAuthoredAssetMutationReceipt rejectedDelete =
      app::deleteCreativeEditorAuthoredAsset(map, library, saved.assetId);
  const app::CreativeEditorAuthoredAssetMutationReceipt deletedDuplicate =
      app::deleteCreativeEditorAuthoredAsset(map, library, duplicate.assetId);

  app::CreativeEditorAuthoredAssetLibrary reloaded;
  const app::CreativeEditorAuthoredAssetLoadReceipt loaded =
      app::loadCreativeEditorAuthoredAssetLibrary(reloaded, root);
  const cr::CreativeAuthoredAssetDefinition* durable =
      app::findCreativeEditorAuthoredAsset(reloaded, saved.assetId);
  const bool duplicateGone = app::findCreativeEditorAuthoredAsset(
                                 reloaded, duplicate.assetId) == nullptr;
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);

  return expect(renamed.accepted && renamed.durableWriteOk &&
                    renamed.assetId == saved.assetId && renamePreservedGeometry,
                "rename preserves identity and authored geometry") &&
         expect(duplicate.accepted && duplicate.durableWriteOk &&
                    duplicate.label == "Stone Column Copy" &&
                    duplicateGeometryMatches,
                "duplicate creates an independent durable identity") &&
         expect(instance.accepted && references.mapInstanceCount == 1U &&
                    references.authoredAssetDependencyCount == 1U &&
                    !rejectedDelete.accepted &&
                    rejectedDelete.reasonCode ==
                        "creative_asset_library_delete_referenced",
                "delete refuses map and authored-asset references") &&
         expect(deletedDuplicate.accepted && deletedDuplicate.durableWriteOk &&
                    loaded.accepted && loaded.loadedCount == 1U &&
                    durable != nullptr && durable->label == "Stone Column" &&
                    duplicateGone,
                "unreferenced delete is soft and durable");
}

bool isolatedAssetEditPreservesMapAndRequiresExplicitRefresh() {
  const auto nonce =
      std::chrono::steady_clock::now().time_since_epoch().count();
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_asset_edit_session_" + std::to_string(nonce));
  cr::CreativeAppState sourceState;
  cr::CreativeDocument source = cr::CreativeDocument::create("Edit Source");
  static_cast<void>(source.assignId(750U));
  const cr::CreativeObjectId sourceId =
      createCrate(source, "Base", {6.0, 0.0, 9.0});
  if (!expect(sourceState.facade.installDocument(std::move(source)).accepted,
              "asset edit source installed")) {
    return false;
  }
  selectOnly(sourceState.facade, sourceId);
  app::CreativeEditorAuthoredAssetLibrary library;
  static_cast<void>(app::loadCreativeEditorAuthoredAssetLibrary(library, root));
  const app::CreativeEditorAuthoredAssetSaveReceipt saved =
      app::saveCreativeEditorSelectionAsAuthoredAsset(sourceState, library);
  const cr::CreativeAuthoredAssetDefinition* original =
      app::findCreativeEditorAuthoredAsset(library, saved.assetId);
  if (!expect(saved.accepted && original != nullptr,
              "asset edit fixture saved")) {
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return false;
  }

  cr::CreativeDocument map = cr::CreativeDocument::create("Live Map");
  static_cast<void>(map.assignId(751U));
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = original;
  placement.targetAnchor = {20.0, 0.0, 20.0};
  const cr::CreativeAuthoredAssetInstanceReceipt instance =
      cr::instantiateCreativeAuthoredAssetAtomically(map, placement);
  cr::CreativeAppState mapState;
  if (!expect(instance.accepted &&
                  mapState.facade.installDocument(std::move(map)).accepted,
              "asset edit map installed")) {
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return false;
  }
  const std::size_t mapObjectCount = mapState.facade.document().objectCount();
  const std::uint64_t mapRevision = mapState.facade.document().revision();
  const std::uint64_t mapUndoDepth = cr::creativeUndoDepth(mapState.history);

  app::CreativeEditorState editor;
  editor.authoredAssets = std::move(library);
  editor.flyPos = {3.0F, 4.0F, 5.0F};
  editor.yawDegrees = 17.0F;
  editor.pitchDegrees = -11.0F;
  editor.interaction.moveTargetId = instance.instanceRootObjectId;
  editor.groupFocus.documentId = mapState.facade.document().id();
  editor.terrain.documentId = mapState.facade.document().id();
  editor.volume.active = true;
  editor.volume.cursorValid = true;
  const app::CreativeEditorAuthoredAssetMutationReceipt begun =
      app::beginCreativeEditorAuthoredAssetEdit(editor, saved.assetId);
  const bool workspaceTransientsStartedClean =
      editor.interaction.moveTargetId == cr::kInvalidObjectId &&
      editor.groupFocus.documentId == cr::kInvalidDocumentId &&
      editor.terrain.documentId == cr::kInvalidDocumentId &&
      !editor.volume.active && !editor.volume.cursorValid;
  cr::CreativeAppState& workspace =
      app::activeCreativeEditorAppState(editor, mapState);
  const cr::CreativeObjectId detailId = createCrate(
      workspace.facade.documentForPersistence(), "Capital", {0.0, 2.0, 0.0});
  const app::CreativeEditorAuthoredAssetMutationReceipt committed =
      app::saveCreativeEditorAuthoredAssetEdit(editor);
  const cr::CreativeAuthoredAssetDefinition* updated =
      app::findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                           saved.assetId);
  const cr::CreativeAuthoredAssetSyncReceipt sync =
      updated == nullptr ? cr::CreativeAuthoredAssetSyncReceipt{}
                         : cr::inspectCreativeAuthoredAssetInstanceSync(
                               mapState.facade.document(), *updated,
                               instance.instanceRootObjectId);
  const bool mapPreserved =
      mapState.facade.document().objectCount() == mapObjectCount &&
      mapState.facade.document().revision() == mapRevision &&
      cr::creativeUndoDepth(mapState.history) == mapUndoDepth &&
      &app::activeCreativeEditorAppState(editor, mapState) == &mapState &&
      editor.interaction.moveTargetId == instance.instanceRootObjectId &&
      editor.groupFocus.documentId == mapState.facade.document().id() &&
      editor.terrain.documentId == mapState.facade.document().id() &&
      editor.volume.active && editor.volume.cursorValid;
  const bool cameraRestored =
      editor.flyPos.x == 3.0F && editor.flyPos.y == 4.0F &&
      editor.flyPos.z == 5.0F && editor.yawDegrees == 17.0F &&
      editor.pitchDegrees == -11.0F;

  const cr::CreativeAuthoredAssetFingerprint savedFingerprint =
      updated == nullptr
          ? cr::CreativeAuthoredAssetFingerprint{}
          : cr::fingerprintCreativeAuthoredAssetDefinition(*updated);
  const app::CreativeEditorAuthoredAssetMutationReceipt begunAgain =
      app::beginCreativeEditorAuthoredAssetEdit(editor, saved.assetId);
  cr::CreativeAppState& discardedWorkspace =
      app::activeCreativeEditorAppState(editor, mapState);
  static_cast<void>(
      createCrate(discardedWorkspace.facade.documentForPersistence(),
                  "Discarded", {0.0, 4.0, 0.0}));
  const bool cancelled = app::cancelCreativeEditorAuthoredAssetEdit(editor);
  const cr::CreativeAuthoredAssetDefinition* afterCancel =
      app::findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                           saved.assetId);
  const cr::CreativeAuthoredAssetFingerprint cancelledFingerprint =
      afterCancel == nullptr
          ? cr::CreativeAuthoredAssetFingerprint{}
          : cr::fingerprintCreativeAuthoredAssetDefinition(*afterCancel);

  app::CreativeEditorAuthoredAssetLibrary reloaded;
  const app::CreativeEditorAuthoredAssetLoadReceipt loaded =
      app::loadCreativeEditorAuthoredAssetLibrary(reloaded, root);
  const cr::CreativeAuthoredAssetDefinition* durable =
      app::findCreativeEditorAuthoredAsset(reloaded, saved.assetId);
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);

  return expect(begun.accepted && detailId != cr::kInvalidObjectId &&
                    committed.accepted && committed.durableWriteOk &&
                    updated != nullptr && updated->content.objects.size() == 2U,
                "isolated workspace saves edited authored content") &&
         expect(workspaceTransientsStartedClean && mapPreserved &&
                    cameraRestored,
                "asset editing isolates transients and preserves map state") &&
         expect(sync.accepted &&
                    sync.state ==
                        cr::CreativeAuthoredAssetSyncState::SourceChanged,
                "saving marks instances source-changed without refreshing") &&
         expect(begunAgain.accepted && cancelled && savedFingerprint.valid &&
                    cancelledFingerprint.valid &&
                    savedFingerprint.value == cancelledFingerprint.value,
                "cancel discards the isolated workspace") &&
         expect(loaded.accepted && durable != nullptr &&
                    durable->content.objects.size() == 2U,
                "saved edit round-trips durably");
}

app::CreativeEditorState authoredEditor(
    const cr::CreativeAuthoredAssetDefinition& definition) {
  app::CreativeEditorState editor;
  editor.authoredAssets.definitions.push_back(definition);
  editor.interaction.hotbar.selectedSlot = 0U;
  cr::CreativeHotbarEntry& held = editor.interaction.hotbar.entries[0];
  held = {cr::CreativeHeldItemKind::Material,
          cr::CreativeObjectKind::PrefabInstance};
  static_cast<void>(cr::setCreativeHotbarAsset(
      held, definition.assetId, definition.sourceBounds));
  editor.placeCellSize = 1.0;
  editor.interaction.target.valid = true;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.faceNormal = {0.0, 1.0, 0.0};
  editor.interaction.target.grid.placerForward = {0.0, 0.0, -1.0};
  editor.interaction.target.grid.placementAnchor = {0.5, 0.0, 0.5};
  editor.interaction.target.grid.adjacentCellBounds =
      {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  return editor;
}

void setSecondary(cr::CreativeWorldActionFrame& actions,
                  bool down,
                  bool pressed,
                  bool released) {
  const std::size_t index =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Secondary);
  actions.down[index] = down;
  actions.pressed[index] = pressed;
  actions.released[index] = released;
}

bool gestureDeduplicatesAndCommitsOneUndo() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  cr::CreativeAppState appState;
  cr::CreativeDocument target = cr::CreativeDocument::create("Gestures");
  static_cast<void>(target.assignId(706U));
  if (!expect(appState.facade.installDocument(std::move(target)).accepted,
              "gesture document installed")) {
    return false;
  }
  app::CreativeEditorState editor = authoredEditor(captured.definition);
  cr::CreativeWorldActionFrame press;
  setSecondary(press, true, true, false);
  app::processCreativeAuthoredAssetFrame(appState, editor, press, 0U);
  const std::size_t firstCount = appState.facade.document().objectCount();
  const std::uint64_t firstRevision = appState.facade.document().revision();

  cr::CreativeWorldActionFrame held;
  setSecondary(held, true, false, false);
  app::processCreativeAuthoredAssetFrame(
      appState, editor, held, 199'000'000ULL);
  const bool beforeRepeatStable =
      appState.facade.document().revision() == firstRevision;
  app::processCreativeAuthoredAssetFrame(
      appState, editor, held, 200'000'000ULL);
  const bool stationaryDeduplicated =
      appState.facade.document().objectCount() == firstCount;

  editor.interaction.target.grid.placementAnchor = {2.5, 0.0, 0.5};
  editor.interaction.target.grid.adjacentCellBounds =
      {{2.0, 0.0, 0.0}, {3.0, 1.0, 1.0}};
  app::processCreativeAuthoredAssetFrame(
      appState, editor, held, 400'000'000ULL);
  const std::size_t movedCount = appState.facade.document().objectCount();
  cr::CreativeWorldActionFrame release;
  setSecondary(release, false, false, true);
  app::processCreativeAuthoredAssetFrame(
      appState, editor, release, 401'000'000ULL);
  const std::uint64_t completedRevision =
      appState.facade.document().revision();
  app::processCreativeAuthoredAssetFrame(
      appState, editor, press, 500'000'000ULL);
  app::processCreativeAuthoredAssetFrame(
      appState, editor, release, 501'000'000ULL);
  const bool crossGestureDuplicateRejected =
      appState.facade.document().revision() == completedRevision &&
      appState.facade.document().objectCount() == movedCount;
  const std::size_t undoDepth = cr::creativeUndoDepth(appState.history);
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(firstCount == 3U && editor.placedCount == 2U,
                "first press places one expanded instance immediately") &&
         expect(beforeRepeatStable && stationaryDeduplicated,
                "199 ms is stable and repeated target mutates once") &&
         expect(movedCount == 6U,
                "new target at the next cadence places another instance") &&
         expect(crossGestureDuplicateRejected,
                "a later gesture cannot duplicate the occupied target") &&
         expect(undoDepth == 1U && undo.accepted &&
                    undo.objectCountAfter == 0U,
                "press-hold-release commits exactly one undo record");
}

bool interruptionFinalizesChangedGesture() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  cr::CreativeAppState appState;
  cr::CreativeDocument target = cr::CreativeDocument::create("Interrupted");
  static_cast<void>(target.assignId(707U));
  if (!expect(appState.facade.installDocument(std::move(target)).accepted,
              "interruption document installed")) {
    return false;
  }
  app::CreativeEditorState editor = authoredEditor(captured.definition);
  cr::CreativeWorldActionFrame press;
  setSecondary(press, true, true, false);
  app::processCreativeAuthoredAssetFrame(appState, editor, press, 0U);
  app::finalizeCreativeEditorContinuousGestures(
      appState, editor, "creative_authored_asset_test_interruption");
  return expect(appState.facade.document().objectCount() == 3U &&
                    cr::creativeUndoDepth(appState.history) == 1U &&
                    !editor.interaction.authoredAssetStroke.repeat.active &&
                    !editor.interaction.authoredAssetStroke.transaction.active,
                "focus/modal/tool interruption commits and clears the stroke");
}

bool previewUsesCanonicalCompositeProxies() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  app::CreativeEditorState editor = authoredEditor(captured.definition);
  iggy3d::FrameInput frame;
  frame.camera.clipFromWorld = iggy3d::identityMat4();
  frame.camera.clipFromView = iggy3d::identityMat4();
  app::attachCreativeEditorPlacementPreviews(editor, false, frame);
  const bool commandsIncludeAuthoring = [&]() {
    const app::CreativeEditorToolOptionsCommandList commands =
        app::creativeEditorToolOptionCommandsForEntry(
            {cr::CreativeHeldItemKind::ObjectMove,
             cr::CreativeObjectKind::Unknown});
    bool save = false;
    bool edit = false;
    bool update = false;
    bool refreshThis = false;
    bool refreshSafe = false;
    bool refreshForce = false;
    for (std::size_t index = 0U; index < commands.count; ++index) {
      save = save || commands.ids[index] ==
                         app::CreativeEditorToolOptionsCommandId::
                             SaveSelectionAsAsset;
      edit = edit || commands.ids[index] ==
                         app::CreativeEditorToolOptionsCommandId::
                             EditGroupContents;
      update = update || commands.ids[index] ==
                             app::CreativeEditorToolOptionsCommandId::
                                 UpdateSavedAsset;
      refreshThis = refreshThis ||
                    commands.ids[index] ==
                        app::CreativeEditorToolOptionsCommandId::
                            RefreshSavedAssetInstance;
      refreshSafe = refreshSafe ||
                    commands.ids[index] ==
                        app::CreativeEditorToolOptionsCommandId::
                            RefreshSafeSavedAssetInstances;
      refreshForce = refreshForce ||
                     commands.ids[index] ==
                         app::CreativeEditorToolOptionsCommandId::
                             ForceRefreshSavedAssetInstances;
    }
    return commands.count == 15U && save && edit && update && refreshThis &&
           refreshSafe && refreshForce;
  }();
  return expect(frame.creativePreview.itemCount == 2U &&
                    frame.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementValid &&
                    frame.creativePreview.items[1].role ==
                        iggy3d::RenderCreativePreviewRole::Held,
                "authored asset shows target and held proxies") &&
         expect(iggy3d::renderCreativePreviewAssetId(
                    frame.creativePreview.items[0])
                    .empty() &&
                    iggy3d::renderCreativePreviewAssetId(
                        frame.creativePreview.items[1])
                        .empty(),
                "composite proxies never request a missing static mesh") &&
         expect(commandsIncludeAuthoring,
                "object tool exposes edit and save-as-asset commands");
}

}  // namespace

int main() {
  return captureInstantiateSelectAndUnpack() &&
                 invalidDefinitionCannotPartiallyMutate() &&
                 refreshAllInstancesIsAtomic() &&
                 syncStatesProtectLocalInstanceEdits() &&
                 definitionsAreBoundedAndFailClosed() &&
                 transformedInstanceUpdateFailsClosed() &&
                 durableLibraryRoundTripsSelection() &&
                 updateCommandRoundTripsEditedInstance() &&
                 editorSyncCommandsExposeStatusAndOneUndo() &&
                 libraryManagementIsDurableAndReferenceSafe() &&
                 isolatedAssetEditPreservesMapAndRequiresExplicitRefresh() &&
                 gestureDeduplicatesAndCommitsOneUndo() &&
                 interruptionFinalizesChangedGesture() &&
                 previewUsesCanonicalCompositeProxies()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
