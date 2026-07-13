#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"
#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"
#include "app/iggy3d/creative/tools/SelectionTransformCommands.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <span>
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

bool groupingPreservesGeometryAndCanonicalHierarchy() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Groups");
  static_cast<void>(document.assignId(201U));
  const cr::CreativeObjectId left = createCrate(document, "Left", {0, 0, 0});
  const cr::CreativeObjectId right =
      createCrate(document, "Right", {4, 0, 0});
  const cr::CreativeTransform leftBefore = document.findObject(left)->transform;
  const cr::CreativeTransform rightBefore =
      document.findObject(right)->transform;
  const std::array selected{right, left, right};
  const cr::CreativeGroupCommandReceipt grouped =
      cr::groupDocumentObjectsAtomically(document, selected);
  const cr::CreativeObject* group =
      document.findObject(grouped.groupObjectId);
  const cr::CreativeObject* leftAfter = document.findObject(left);
  const cr::CreativeObject* rightAfter = document.findObject(right);
  const std::array nestedSelection{grouped.groupObjectId, left};
  const cr::CreativeHierarchySelection hierarchy =
      cr::resolveCreativeObjectHierarchy(document, nestedSelection);
  const cr::CreativeObjectId interactionRoot =
      cr::resolveCreativeHierarchyInteractionRoot(
          document, std::span{&grouped.groupObjectId, 1U}, left);

  return expect(grouped.accepted && grouped.changed &&
                    grouped.status == cr::CreativeGroupCommandStatus::Applied,
                "two unique roots group atomically") &&
         expect(group != nullptr && group->kind == cr::CreativeObjectKind::Group &&
                    grouped.selectionObjectIds.size() == 1U &&
                    grouped.selectionObjectIds.front() == group->id,
                "group command returns one selectable root") &&
         expect(leftAfter != nullptr && rightAfter != nullptr &&
                    leftAfter->parentId == group->id &&
                    rightAfter->parentId == group->id &&
                    leftAfter->transform.position.x ==
                        leftBefore.position.x &&
                    rightAfter->transform.position.x ==
                        rightBefore.position.x,
                "grouping changes hierarchy without moving geometry") &&
         expect(hierarchy.accepted && hierarchy.rootObjectIds.size() == 1U &&
                    hierarchy.rootObjectIds.front() == group->id &&
                    hierarchy.objectIds.size() == 3U,
                "ancestor selection absorbs selected descendants once") &&
         expect(interactionRoot == group->id,
                "visible child hit resolves to its selected group root");
}

bool groupTransformsAndDuplicatesAsOneSelectionRoot() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Transform");
  static_cast<void>(document.assignId(202U));
  const cr::CreativeObjectId first = createCrate(document, "A", {0, 0, 0});
  const cr::CreativeObjectId second = createCrate(document, "B", {4, 0, 0});
  const std::array selected{first, second};
  const cr::CreativeGroupCommandReceipt grouped =
      cr::groupDocumentObjectsAtomically(document, selected);
  if (!expect(grouped.accepted, "transform group setup")) {
    return false;
  }
  const double groupX = document.findObject(grouped.groupObjectId)
                            ->transform.position.x;
  const double firstX = document.findObject(first)->transform.position.x;
  const double secondX = document.findObject(second)->transform.position.x;
  cr::CreativeTransformCommandRequest move;
  move.kind = cr::CreativeTransformCommandKind::Translate;
  move.translation = {3, 2, -1};
  const cr::CreativeObjectId groupId = grouped.groupObjectId;
  const cr::CreativeTransformCommandReceipt moved =
      cr::transformDocumentObjectsAtomically(
          document, std::span{&groupId, 1U}, move);
  const cr::CreativeDuplicateCommandReceipt duplicated =
      cr::duplicateDocumentObjectsAtomically(
          document, std::span{&groupId, 1U});
  const cr::CreativeObjectId duplicateRoot =
      duplicated.duplicatedSelectionObjectIds.empty()
          ? cr::kInvalidObjectId
          : duplicated.duplicatedSelectionObjectIds.front();
  const cr::CreativeHierarchySelection duplicateHierarchy =
      cr::resolveCreativeObjectHierarchy(
          document, std::span{&duplicateRoot, 1U});

  return expect(moved.accepted && moved.changed && moved.objectCount == 3U &&
                    moved.pivot.x == groupX,
                "one group root expands to all transform objects") &&
         expect(document.findObject(groupId)->transform.position.x ==
                        groupX + 3.0 &&
                    document.findObject(first)->transform.position.x ==
                        firstX + 3.0 &&
                    document.findObject(second)->transform.position.x ==
                        secondX + 3.0,
                "group translation moves root and descendants equally") &&
         expect(duplicated.accepted &&
                    duplicated.duplicatedObjectCount == 3U,
                "duplicate copies the complete hierarchy") &&
         expect(duplicated.duplicatedSelectionObjectIds.size() == 1U,
                "duplicate remaps one selected hierarchy root") &&
         expect(duplicateHierarchy.accepted &&
                    duplicateHierarchy.objectIds.size() == 3U,
                "duplicated root owns both remapped descendants");
}

bool ungroupAndHierarchyRemovalAreAtomic() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Ungroup");
  static_cast<void>(document.assignId(203U));
  const cr::CreativeObjectId first = createCrate(document, "A", {0, 0, 0});
  const cr::CreativeObjectId second = createCrate(document, "B", {2, 0, 0});
  const std::array selected{first, second};
  const cr::CreativeGroupCommandReceipt grouped =
      cr::groupDocumentObjectsAtomically(document, selected);
  const cr::CreativeObjectId groupId = grouped.groupObjectId;
  const cr::CreativeGroupCommandReceipt ungrouped =
      cr::ungroupDocumentObjectAtomically(document, groupId);
  bool ok = expect(ungrouped.accepted && ungrouped.changed &&
                       document.findObject(groupId) == nullptr &&
                       !document.findObject(first)->parentId.has_value() &&
                       !document.findObject(second)->parentId.has_value() &&
                       ungrouped.selectionObjectIds.size() == 2U,
                   "ungroup restores children and removes only the container");

  const cr::CreativeGroupCommandReceipt regrouped =
      cr::groupDocumentObjectsAtomically(document, selected);
  const cr::CreativeMutationRequest lock{
      0U, first, cr::CreativeMutationKind::SetLocked,
      cr::makeLockPayload(true)};
  static_cast<void>(cr::applyDocumentMutation(document, lock));
  const std::uint64_t revisionBeforeReject = document.revision();
  const cr::CreativeGroupCommandReceipt rejected =
      cr::ungroupDocumentObjectAtomically(document, regrouped.groupObjectId);
  ok = expect(!rejected.accepted &&
                  rejected.status ==
                      cr::CreativeGroupCommandStatus::MutationRejected &&
                  document.revision() == revisionBeforeReject &&
                  document.findObject(first)->parentId ==
                      regrouped.groupObjectId,
              "locked child rejects ungroup without a partial hierarchy") &&
       ok;

  static_cast<void>(cr::applyDocumentMutation(
      document, first, cr::CreativeMutationKind::SetLocked,
      cr::makeLockPayload(false)));
  const cr::CreativeHierarchyRemoveReceipt removed =
      cr::removeCreativeObjectHierarchyAtomically(
          document, regrouped.groupObjectId);
  return expect(removed.accepted && removed.changed &&
                    removed.removedObjectIds.size() == 3U &&
                    document.objectCount() == 0U,
                "hierarchy removal deletes descendants before the group") &&
         ok;
}

}  // namespace

int main() {
  return groupingPreservesGeometryAndCanonicalHierarchy() &&
                 groupTransformsAndDuplicatesAsOneSelectionRoot() &&
                 ungroupAndHierarchyRemovalAreAtomic()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
