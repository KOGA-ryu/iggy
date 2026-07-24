#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::TargetRef target(cr::Id id) {
  return cr::TargetRef{id};
}

cr::CreativeObjectId addObject(
    cr::CreativeDocument& document,
    std::string name,
    std::vector<std::string> tags = {},
    bool visible = true,
    bool locked = false) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = std::move(name);
  request.tags = std::move(tags);
  request.visible = visible;
  request.hasVisibleOverride = true;
  request.locked = locked;
  request.hasLockedOverride = true;
  return document.createObject(request).objectId;
}

cr::CreativeToolIntent selectIntent(cr::TargetRef selectedTarget) {
  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::SelectObjectCandidate;
  intent.tool = cr::Tool::Select;
  intent.pointer.target = selectedTarget;
  return intent;
}

bool defaultStateIsEmpty() {
  const cr::CreativeSelectionState state =
      cr::makeDefaultCreativeSelectionState();

  return expect(state.selectedTarget.value == cr::kInvalidId,
                "default selected empty") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "default candidate empty");
}

bool settingSelectedTargetChangesState() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt receipt =
      cr::setSelectedTarget(state, target(7));

  return expect(receipt.accepted, "set selected accepted") &&
         expect(receipt.changed, "set selected changed") &&
         expect(receipt.requestedChange ==
                    cr::CreativeSelectionChangeKind::SetSelectedTarget,
                "set selected requested") &&
         expect(receipt.appliedChange ==
                    cr::CreativeSelectionChangeKind::SetSelectedTarget,
                "set selected applied") &&
         expect(receipt.selectedTargetBefore.value == cr::kInvalidId,
                "set selected before") &&
         expect(receipt.selectedTargetAfter.value == 7U,
                "set selected after") &&
         expect(state.selectedTarget.value == 7U, "set selected state");
}

bool settingSameSelectedTargetIsNoChange() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt first =
      cr::setSelectedTarget(state, target(7));
  const cr::CreativeSelectionReceipt second =
      cr::setSelectedTarget(state, target(7));

  return expect(first.changed, "same setup changed") &&
         expect(second.accepted, "same selected accepted") &&
         expect(!second.changed, "same selected unchanged") &&
         expect(second.appliedChange == cr::CreativeSelectionChangeKind::None,
                "same selected no applied change") &&
         expect(second.selectedTargetBefore.value == 7U,
                "same selected before") &&
         expect(second.selectedTargetAfter.value == 7U,
                "same selected after");
}

bool clearingEmptySelectionIsNoChange() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt receipt = cr::clearSelection(state);

  return expect(receipt.accepted, "clear empty accepted") &&
         expect(!receipt.changed, "clear empty unchanged") &&
         expect(receipt.selectedTargetAfter.value == cr::kInvalidId,
                "clear empty selected") &&
         expect(receipt.candidateTargetAfter.value == cr::kInvalidId,
                "clear empty candidate") &&
         expect(receipt.message == "selection_already_empty",
                "clear empty message");
}

bool clearingNonEmptySelectionClearsCandidate() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt selected =
      cr::setSelectedTarget(state, target(7));
  const cr::CreativeSelectionReceipt candidate =
      cr::updateSelectionCandidate(state, target(9));
  const cr::CreativeSelectionReceipt cleared = cr::clearSelection(state);

  return expect(selected.changed, "clear setup selected") &&
         expect(candidate.changed, "clear setup candidate") &&
         expect(cleared.accepted, "clear non-empty accepted") &&
         expect(cleared.changed, "clear non-empty changed") &&
         expect(cleared.appliedChange ==
                    cr::CreativeSelectionChangeKind::ClearSelection,
                "clear non-empty applied") &&
         expect(cleared.selectedTargetBefore.value == 7U,
                "clear non-empty selected before") &&
         expect(cleared.candidateTargetBefore.value == 9U,
                "clear non-empty candidate before") &&
         expect(state.selectedTarget.value == cr::kInvalidId,
                "clear non-empty selected state") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "clear non-empty candidate state");
}

bool updatingCandidateDoesNotMutateSelected() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt selected =
      cr::setSelectedTarget(state, target(7));
  const cr::CreativeSelectionReceipt candidate =
      cr::updateSelectionCandidate(state, target(9));

  return expect(selected.changed, "candidate setup selected") &&
         expect(candidate.accepted, "candidate accepted") &&
         expect(candidate.changed, "candidate changed") &&
         expect(candidate.appliedChange ==
                    cr::CreativeSelectionChangeKind::UpdateCandidate,
                "candidate applied") &&
         expect(state.selectedTarget.value == 7U,
                "candidate leaves selected") &&
         expect(state.candidateTarget.value == 9U,
                "candidate updates candidate");
}

bool selectionRevisionTracksCommittedIdentityOnly() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const std::uint64_t initialRevision = state.selectionRevision;
  const cr::CreativeSelectionReceipt candidate =
      cr::updateSelectionCandidate(state, target(9));
  const std::uint64_t candidateRevision = state.selectionRevision;
  const std::array firstTargets{target(1), target(2)};
  const cr::CreativeSelectionReceipt first =
      cr::setSelectedTargets(state, firstTargets, target(2));
  const std::uint64_t firstRevision = state.selectionRevision;
  const cr::CreativeSelectionReceipt unchanged =
      cr::setSelectedTargets(state, firstTargets, target(2));
  const std::uint64_t unchangedRevision = state.selectionRevision;
  const std::array replacementTargets{target(1), target(3)};
  const cr::CreativeSelectionReceipt replacement =
      cr::setSelectedTargets(state, replacementTargets, target(3));

  return expect(candidate.changed && candidateRevision == initialRevision,
                "candidate changes do not consume a selection revision") &&
         expect(first.changed && firstRevision == initialRevision + 1U,
                "first committed selection advances revision") &&
         expect(!unchanged.changed && unchangedRevision == firstRevision,
                "identical committed selection remains unchanged") &&
         expect(replacement.changed &&
                    state.selectionRevision == initialRevision + 2U,
                "same-count identity replacement advances revision");
}

bool applyingSelectObjectCandidateSelectsTarget() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt receipt =
      cr::applySelectionToolIntent(state, selectIntent(target(42)));

  return expect(receipt.accepted, "select intent accepted") &&
         expect(receipt.changed, "select intent changed") &&
         expect(receipt.selectedTargetBefore.value == cr::kInvalidId,
                "select intent before") &&
         expect(receipt.selectedTargetAfter.value == 42U,
                "select intent after") &&
         expect(state.selectedTarget.value == 42U,
                "select intent state selected") &&
         expect(receipt.message == "select_candidate_selected",
                "select intent message");
}

bool invalidSelectObjectCandidateClearsSelection() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt selected =
      cr::setSelectedTarget(state, target(42));
  const cr::CreativeSelectionReceipt candidate =
      cr::updateSelectionCandidate(state, target(7));
  const cr::CreativeSelectionReceipt cleared =
      cr::applySelectionToolIntent(state, selectIntent({}));

  return expect(selected.changed, "invalid setup selected") &&
         expect(candidate.changed, "invalid setup candidate") &&
         expect(cleared.accepted, "invalid select accepted") &&
         expect(cleared.changed, "invalid select changed") &&
         expect(cleared.appliedChange ==
                    cr::CreativeSelectionChangeKind::ClearSelection,
                "invalid select clears") &&
         expect(cleared.selectedTargetBefore.value == 42U,
                "invalid select selected before") &&
         expect(cleared.candidateTargetBefore.value == 7U,
                "invalid select candidate before") &&
         expect(state.selectedTarget.value == cr::kInvalidId,
                "invalid select selected cleared") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "invalid select candidate cleared") &&
         expect(cleared.message == "select_candidate_cleared_selection",
                "invalid select message");
}

bool nonSelectionIntentIsNoOp() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  const cr::CreativeSelectionReceipt selected =
      cr::setSelectedTarget(state, target(42));

  cr::CreativeToolIntent intent;
  intent.kind = cr::CreativeToolIntentKind::PreviewPointer;
  intent.tool = cr::Tool::Select;
  intent.pointer.target = target(7);

  const cr::CreativeSelectionReceipt receipt =
      cr::applySelectionToolIntent(state, intent);

  return expect(selected.changed, "non-selection setup selected") &&
         expect(!receipt.accepted, "non-selection not accepted") &&
         expect(!receipt.changed, "non-selection unchanged") &&
         expect(receipt.appliedChange == cr::CreativeSelectionChangeKind::None,
                "non-selection no applied change") &&
         expect(state.selectedTarget.value == 42U,
                "non-selection selected preserved") &&
         expect(state.candidateTarget.value == cr::kInvalidId,
                "non-selection candidate preserved") &&
         expect(receipt.message == "non_selection_intent",
                "non-selection message");
}

bool selectionCapacityRejectsWithoutPartialChange() {
  cr::CreativeSelectionState state = cr::makeDefaultCreativeSelectionState();
  static_cast<void>(cr::setSelectedTarget(state, target(7U)));
  std::vector<cr::TargetRef> targets;
  targets.reserve(cr::kCreativeSelectionTargetCapacity + 1U);
  for (std::size_t index = 0U;
       index <= cr::kCreativeSelectionTargetCapacity; ++index) {
    targets.push_back(target(static_cast<cr::Id>(index + 1U)));
  }

  const cr::CreativeSelectionReceipt receipt =
      cr::setSelectedTargets(state, targets);
  return expect(!receipt.accepted && !receipt.changed,
                "selection capacity rejects") &&
         expect(receipt.message == "selection_capacity_exceeded",
                "selection capacity reports reason") &&
         expect(state.selectedTarget.value == 7U &&
                    state.selectedTargets.size() == 1U &&
                    state.selectedTargets.front().value == 7U,
                "selection capacity preserves prior state");
}

bool semanticResolutionPreservesInspectionFacts() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Selection");
  const cr::CreativeObjectId objectId =
      addObject(document, "Hidden Locked", {}, false, true);
  const cr::CreativeSemanticSelectionResolution resolved =
      cr::resolveCreativeSemanticSelection(document, objectId);
  const cr::CreativeSemanticSelectionResolution missing =
      cr::resolveCreativeSemanticSelection(document, 999U);

  return expect(resolved.accepted &&
                    resolved.status ==
                        cr::CreativeSemanticSelectionStatus::Ready &&
                    resolved.primaryOwner ==
                        cr::CreativeSemanticSelectionOwner::AuthoredObject,
                "authored object resolves") &&
         expect(!resolved.objectVisible && resolved.objectLocked &&
                    resolved.objectKind == cr::CreativeObjectKind::Crate,
                "hidden locked object remains inspectable") &&
         expect(!missing.accepted &&
                    missing.status ==
                        cr::CreativeSemanticSelectionStatus::MissingObject &&
                    missing.reasonCode == "creative_selection_object_missing",
                "missing semantic object fails closed");
}

bool semanticResolutionReportsInheritedInspectionFacts() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Selection Hierarchy");
  cr::CreativeDocumentCreateRequest parentRequest;
  parentRequest.kind = cr::CreativeObjectKind::Group;
  parentRequest.name = "Hidden Locked Parent";
  const cr::CreativeDocumentCreateReceipt parent =
      document.createObject(parentRequest);

  cr::CreativeDocumentCreateRequest childRequest;
  childRequest.kind = cr::CreativeObjectKind::Crate;
  childRequest.name = "Locally Visible Child";
  childRequest.parentId = parent.objectId;
  const cr::CreativeDocumentCreateReceipt child =
      document.createObject(childRequest);
  const cr::CreativeDocumentMutationReceipt hidden =
      cr::setDocumentObjectVisible(document, parent.objectId, false);
  const cr::CreativeDocumentMutationReceipt locked =
      cr::setDocumentObjectLocked(document, parent.objectId, true);
  const cr::CreativeObject* childObject = document.findObject(child.objectId);
  const cr::CreativeSemanticSelectionResolution resolved =
      cr::resolveCreativeSemanticSelection(document, child.objectId);

  return expect(parent.accepted && child.accepted && hidden.changed &&
                    locked.changed && childObject != nullptr &&
                    childObject->visible && !childObject->locked,
                "hierarchy selection fixture preserves local child flags") &&
         expect(resolved.accepted && !resolved.objectVisible &&
                    resolved.objectLocked && resolved.hasParent &&
                    resolved.parentObjectId == parent.objectId,
                "semantic selection reports inherited visibility and lock");
}

bool semanticResolutionPreservesNestedRecipeAncestry() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Ownership");
  const cr::CreativeObjectId sourceId = addObject(document, "Source");

  cr::CreativeWorldLayout layout;
  cr::CreativeWorldLayoutObject source;
  source.kind = cr::CreativeObjectKind::Crate;
  source.stableKey = "layout_crate";
  source.name = "Layout Crate";
  layout.objects.push_back(source);
  const std::string provenanceTag = cr::creativeWorldLayoutProvenanceTag(
      layout, cr::CreativeWorldLayoutTable::Object, 0U);
  const cr::CreativeObjectId generatedId = addObject(
      document, "Generated",
      {cr::creativeWorldLayoutTag(layout.stableKey), provenanceTag});

  cr::CreativePatternRecipeMutationRequest request;
  request.kind = cr::CreativePatternRecipeMutationKind::Add;
  request.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  request.recipe.sourceObjectIds = {sourceId};
  request.recipe.generatedObjectIds = {generatedId};
  const cr::CreativePatternRecipeMutationReceipt recipe =
      document.applyPatternRecipeMutation(request);
  const cr::CreativeSemanticSelectionResolution resolved =
      cr::resolveCreativeSemanticSelection(document, generatedId, &layout);

  return expect(recipe.accepted && recipe.changed,
                "nested ownership fixture recipe added") &&
         expect(resolved.accepted &&
                    resolved.primaryOwner ==
                        cr::CreativeSemanticSelectionOwner::PatternRecipe &&
                    resolved.patternRecipeId == recipe.recipeId &&
                    resolved.patternRecipeKind ==
                        cr::CreativePatternRecipeKind::LinearArray,
                "nearest pattern owner resolves") &&
         expect(resolved.worldLayoutSource.owned &&
                    resolved.worldLayoutSource.table ==
                        cr::CreativeWorldLayoutTable::Object &&
                    resolved.worldLayoutSource.index == 0U,
                "underlying world layout ancestry remains available");
}

bool semanticSetResolutionFindsTheDeepestSharedOwner() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Selection Set Ownership");
  cr::CreativeWorldLayout layout;
  layout.stableKey = "selection_set_layout";

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_house";
  layout.buildings.push_back(building);

  for (std::size_t levelIndex = 0U; levelIndex < 2U; ++levelIndex) {
    cr::CreativeWorldLayoutLevel level;
    level.buildingIndex = 0U;
    level.stableKey = "level_" + std::to_string(levelIndex);
    layout.levels.push_back(level);

    cr::CreativeWorldLayoutRoom room;
    room.buildingIndex = 0U;
    room.levelIndex = levelIndex;
    room.stableKey = "room_" + std::to_string(levelIndex);
    layout.rooms.push_back(room);
  }

  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 0U;
  opening.stableKey = "opening_room_0";
  layout.openings.push_back(opening);

  const std::string layoutTag = cr::creativeWorldLayoutTag(layout.stableKey);
  const cr::CreativeObjectId roomObject = addObject(
      document, "Room Geometry",
      {layoutTag, cr::creativeWorldLayoutProvenanceTag(
                      layout, cr::CreativeWorldLayoutTable::Room, 0U)});
  const cr::CreativeObjectId openingObject = addObject(
      document, "Opening Geometry",
      {layoutTag, cr::creativeWorldLayoutProvenanceTag(
                      layout, cr::CreativeWorldLayoutTable::Opening, 0U)});
  const cr::CreativeObjectId upperRoomObject = addObject(
      document, "Upper Room Geometry",
      {layoutTag, cr::creativeWorldLayoutProvenanceTag(
                      layout, cr::CreativeWorldLayoutTable::Room, 1U)});
  const cr::CreativeObjectId authoredObject =
      addObject(document, "Authored Crate");

  cr::CreativePatternRecipeMutationRequest patternRequest;
  patternRequest.kind = cr::CreativePatternRecipeMutationKind::Add;
  patternRequest.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  patternRequest.recipe.sourceObjectIds = {authoredObject};
  patternRequest.recipe.generatedObjectIds = {roomObject, openingObject};
  const cr::CreativePatternRecipeMutationReceipt pattern =
      document.applyPatternRecipeMutation(patternRequest);

  const std::array roomSelection{roomObject, openingObject};
  const cr::CreativeSemanticSelectionSetResolution room =
      cr::resolveCreativeSemanticSelectionSet(
          document, roomSelection, openingObject, &layout);
  const std::array buildingSelection{roomObject, upperRoomObject};
  const cr::CreativeSemanticSelectionSetResolution buildingScope =
      cr::resolveCreativeSemanticSelectionSet(
          document, buildingSelection, roomObject, &layout);
  const std::array completeBuildingSelection{
      roomObject, openingObject, upperRoomObject};
  const cr::CreativeWorldLayoutSourceRef incompleteBuildingSource =
      cr::resolveCompleteCreativeWorldLayoutBuildingSelectionSource(
          document, buildingSelection, layout);
  const cr::CreativeWorldLayoutSourceRef completeBuildingSource =
      cr::resolveCompleteCreativeWorldLayoutBuildingSelectionSource(
          document, completeBuildingSelection, layout);
  const std::array mixedSelection{roomObject, authoredObject};
  const cr::CreativeSemanticSelectionSetResolution mixed =
      cr::resolveCreativeSemanticSelectionSet(
          document, mixedSelection, authoredObject, &layout);
  const std::array missingSelection{roomObject,
                                    static_cast<cr::CreativeObjectId>(9999U)};
  const cr::CreativeSemanticSelectionSetResolution missing =
      cr::resolveCreativeSemanticSelectionSet(
          document, missingSelection, roomObject, &layout);

  return expect(pattern.accepted && pattern.changed,
                "semantic set fixture records one pattern recipe") &&
         expect(room.accepted && room.resolvedCount == 2U &&
                    room.authoredOwnerCount == 0U &&
                    room.patternOwnerCount == 2U &&
                    room.worldLayoutOwnerCount == 0U &&
                    room.primaryObjectId == openingObject &&
                    room.primaryOwner ==
                        cr::CreativeSemanticSelectionOwner::PatternRecipe &&
                    room.patternRecipeId == pattern.recipeId,
                "one pattern owns every selected generated member") &&
         expect(room.commonWorldLayoutSource.table ==
                        cr::CreativeWorldLayoutTable::Room &&
                    room.commonWorldLayoutSource.index == 0U,
                "pattern selection retains its deepest shared room") &&
         expect(buildingScope.accepted &&
                    buildingScope.authoredOwnerCount == 0U &&
                    buildingScope.patternOwnerCount == 1U &&
                    buildingScope.worldLayoutOwnerCount == 1U &&
                    buildingScope.primaryOwner ==
                        cr::CreativeSemanticSelectionOwner::WorldLayoutSource &&
                    buildingScope.commonWorldLayoutSource.table ==
                        cr::CreativeWorldLayoutTable::Building &&
                    buildingScope.commonWorldLayoutSource.index == 0U,
                "members on different floors converge at their building") &&
         expect(mixed.accepted &&
                    mixed.authoredOwnerCount == 1U &&
                    mixed.patternOwnerCount == 1U &&
                    mixed.worldLayoutOwnerCount == 0U &&
                    mixed.primaryOwner ==
                        cr::CreativeSemanticSelectionOwner::AuthoredObject &&
                    mixed.commonWorldLayoutSource.table ==
                        cr::CreativeWorldLayoutTable::None &&
                    mixed.patternRecipeId ==
                        cr::kInvalidCreativePatternRecipeId,
                "mixed authored and generated selection invents no owner") &&
         expect(!cr::resolveCreativeSemanticObjectAction(
                     buildingScope,
                     cr::CreativeSemanticObjectAction::TransformSelection)
                     .allowed &&
                    !cr::resolveCreativeSemanticObjectAction(
                         mixed, cr::CreativeSemanticObjectAction::Delete)
                         .allowed,
                "mixed nearest owners reject mutation atomically") &&
         expect(incompleteBuildingSource.table ==
                        cr::CreativeWorldLayoutTable::None &&
                    completeBuildingSource.table ==
                        cr::CreativeWorldLayoutTable::Building &&
                    completeBuildingSource.index == 0U,
                "complete building promotion requires every generated member") &&
         expect(!missing.accepted && missing.resolvedCount == 1U &&
                    missing.missingCount == 1U &&
                    missing.status ==
                        cr::CreativeSemanticSelectionStatus::MissingObject,
                "missing members fail the complete requested set closed");
}

bool semanticActionDescriptorsCoverTheClosedActionSet() {
  using Action = cr::CreativeSemanticObjectAction;
  using Effect = cr::CreativeSemanticObjectActionEffect;
  using LockPolicy = cr::CreativeSemanticObjectActionLockPolicy;
  struct Expected {
    Action action = Action::Count;
    std::string_view id;
    Effect effect = Effect::ReadOnly;
    LockPolicy lockPolicy = LockPolicy::Ignore;
  };
  constexpr std::array expected{
      Expected{Action::Inspect, "inspect", Effect::ReadOnly,
               LockPolicy::Ignore},
      Expected{Action::Copy, "copy", Effect::ReadOnly,
               LockPolicy::Ignore},
      Expected{Action::Duplicate, "duplicate", Effect::Mutation,
               LockPolicy::RequireUnlocked},
      Expected{Action::Delete, "delete", Effect::Mutation,
               LockPolicy::RequireUnlocked},
      Expected{Action::Cut, "cut", Effect::Mutation,
               LockPolicy::RequireUnlocked},
      Expected{Action::Rename, "rename", Effect::Mutation,
               LockPolicy::RequireUnlocked},
      Expected{Action::SetVisible, "set_visible", Effect::Mutation,
               LockPolicy::RequireUnlocked},
      Expected{Action::SetLocked, "set_locked", Effect::Mutation,
               LockPolicy::Ignore},
      Expected{Action::TransformSelection, "transform_selection",
               Effect::Mutation, LockPolicy::RequireUnlocked},
      Expected{Action::SetTransform, "set_transform", Effect::Mutation,
               LockPolicy::RequireUnlocked},
      Expected{Action::StructuralMutation, "structural_mutation",
               Effect::Mutation, LockPolicy::RequireUnlocked},
  };
  bool ok = expect(
      expected.size() == cr::kCreativeSemanticObjectActionAdmissionCount &&
          cr::kCreativeSemanticObjectActionDescriptors.size() ==
              expected.size(),
      "semantic action descriptor table covers every closed action");
  for (std::size_t index = 0U; index < expected.size(); ++index) {
    const Expected& want = expected[index];
    const cr::CreativeSemanticObjectActionDescriptor& descriptor =
        cr::kCreativeSemanticObjectActionDescriptors[index];
    ok = expect(
             descriptor.action == static_cast<Action>(index) &&
                 descriptor.action == want.action &&
                 descriptor.id == want.id &&
                 descriptor.effect == want.effect &&
                 descriptor.lockPolicy == want.lockPolicy &&
                 cr::creativeSemanticObjectActionDescriptor(want.action) ==
                     &descriptor &&
                 cr::creativeSemanticObjectActionRequiresUnlockedSelection(
                     want.action) ==
                     (want.lockPolicy == LockPolicy::RequireUnlocked),
             "semantic action descriptor matches its closed-enum row") &&
         ok;
    for (std::size_t earlier = 0U; earlier < index; ++earlier) {
      ok = expect(
               descriptor.id !=
                   cr::kCreativeSemanticObjectActionDescriptors[earlier].id,
               "semantic action descriptor ids are unique") &&
           ok;
    }
  }
  return expect(
             cr::creativeSemanticObjectActionDescriptor(Action::Count) ==
                     nullptr &&
                 !cr::creativeSemanticObjectActionRequiresUnlockedSelection(
                     Action::Count),
             "invalid semantic action descriptor lookup fails closed") &&
         ok;
}

bool semanticActionPolicyRoutesEveryOwnerClass() {
  using Action = cr::CreativeSemanticObjectAction;
  using Owner = cr::CreativeSemanticSelectionOwner;
  using Route = cr::CreativeSemanticObjectActionRoute;
  using Table = cr::CreativeWorldLayoutTable;

  const auto policy = [](Owner owner, Action action,
                         Table table = Table::None,
                         std::size_t contributors = 0U) {
    return cr::resolveCreativeSemanticObjectAction(
        owner, action, table, contributors);
  };
  cr::CreativeSemanticSelectionSetResolution distinctPatterns;
  distinctPatterns.accepted = true;
  distinctPatterns.status = cr::CreativeSemanticSelectionStatus::Ready;
  distinctPatterns.primaryOwner = Owner::AuthoredObject;
  distinctPatterns.patternOwnerCount = 2U;
  cr::CreativeSemanticSelectionSetResolution distinctWorldSources;
  distinctWorldSources.accepted = true;
  distinctWorldSources.status = cr::CreativeSemanticSelectionStatus::Ready;
  distinctWorldSources.primaryOwner = Owner::AuthoredObject;
  distinctWorldSources.worldLayoutOwnerCount = 2U;

  return expect(policy(Owner::AuthoredObject, Action::Rename).allowed &&
                    policy(Owner::AuthoredObject, Action::Rename).route ==
                        Route::Document &&
                    cr::creativeSemanticActionUsesDocumentMutation(
                        policy(Owner::AuthoredObject, Action::Delete)),
                "authored edits route to the document") &&
         expect(policy(Owner::PatternRecipe, Action::Delete).allowed &&
                    policy(Owner::PatternRecipe, Action::Delete).route ==
                        Route::SemanticDocument &&
                    cr::creativeSemanticActionUsesDocumentMutation(
                        policy(Owner::PatternRecipe, Action::Delete)) &&
                    policy(Owner::PatternRecipe, Action::TransformSelection)
                            .route == Route::PatternRecipe,
                "pattern delete and transform route through recipe semantics") &&
         expect(!policy(Owner::PatternRecipe, Action::Rename).allowed &&
                    !policy(Owner::PatternRecipe, Action::SetVisible).allowed &&
                    !policy(Owner::PatternRecipe, Action::SetLocked).allowed &&
                    !policy(Owner::PatternRecipe, Action::SetTransform).allowed,
                "pattern output rejects raw object edits") &&
         expect(policy(Owner::WorldLayoutSource, Action::Duplicate,
                       Table::Building)
                            .route == Route::WorldLayoutSource &&
                    policy(Owner::WorldLayoutSource, Action::Delete,
                           Table::Building)
                            .route == Route::WorldLayoutSource &&
                    policy(Owner::WorldLayoutSource, Action::Rename,
                           Table::Building)
                            .route == Route::WorldLayoutSource &&
                    policy(Owner::WorldLayoutSource, Action::SetVisible,
                           Table::Building)
                            .route == Route::WorldLayoutSource &&
                    policy(Owner::WorldLayoutSource,
                           Action::TransformSelection, Table::Building)
                            .route == Route::WorldLayoutSource,
                "building actions route to the World Layout source") &&
         expect(!policy(Owner::WorldLayoutSource, Action::Cut,
                        Table::Building)
                     .allowed &&
                    !policy(Owner::WorldLayoutSource, Action::SetLocked,
                            Table::Building)
                         .allowed &&
                    !policy(Owner::WorldLayoutSource,
                            Action::StructuralMutation, Table::Building)
                         .allowed &&
                    !cr::creativeSemanticActionUsesDocumentMutation(
                        policy(Owner::WorldLayoutSource, Action::Delete,
                               Table::Building)),
                "World Layout output rejects source-less mutations") &&
         expect(policy(Owner::WorldLayoutSource, Action::SetTransform,
                       Table::Object, 1U)
                            .route == Route::RefineThenAdopt &&
                    policy(Owner::WorldLayoutSource, Action::SetTransform,
                           Table::Box, 1U)
                            .route == Route::RefineThenAdopt,
                "one-contributor object and box permit explicit refinement") &&
         expect(!policy(Owner::WorldLayoutSource, Action::SetTransform,
                        Table::Object, 2U)
                     .allowed &&
                    !policy(Owner::WorldLayoutSource, Action::SetTransform,
                            Table::Room, 1U)
                         .allowed &&
                    !policy(Owner::WorldLayoutSource,
                            Action::TransformSelection, Table::Object)
                         .allowed &&
                    !policy(Owner::WorldLayoutSource, Action::SetVisible,
                            Table::Box)
                         .allowed,
                "ambiguous and structural World Layout output stays protected") &&
         expect(cr::resolveCreativeSemanticObjectAction(
                        distinctPatterns, Action::Delete)
                        .route == Route::SemanticDocument &&
                    !cr::resolveCreativeSemanticObjectAction(
                         distinctWorldSources, Action::Delete)
                         .allowed,
                "same-owner sets never fall back to authored mutation") &&
         expect(!policy(Owner::None, Action::Inspect).allowed,
                "missing owner rejects even inspection");
}

bool structuralMutationAdmissionIsExplicitAndFailClosed() {
  using AdmissionStatus =
      cr::CreativeStructuralMutationAdmissionStatus;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Structural Mutation Admission");
  const cr::CreativeObjectId authored =
      addObject(document, "Authored Object");
  const cr::CreativeObjectId patternSeed =
      addObject(document, "Pattern Seed");
  const cr::CreativeObjectId patternOutput =
      addObject(document, "Pattern Output");
  const cr::CreativeObjectId worldOutput = addObject(
      document, "World Output",
      {"creative_world_layout:structural_admission_fixture"});
  cr::CreativePatternRecipeMutationRequest recipeRequest;
  recipeRequest.kind = cr::CreativePatternRecipeMutationKind::Add;
  recipeRequest.recipe.kind = cr::CreativePatternRecipeKind::LinearArray;
  recipeRequest.recipe.sourceObjectIds = {patternSeed};
  recipeRequest.recipe.generatedObjectIds = {patternOutput};
  const cr::CreativePatternRecipeMutationReceipt recipe =
      document.applyPatternRecipeMutation(recipeRequest);
  constexpr cr::CreativeObjectId missing = 9999U;

  struct SingleCase {
    cr::CreativeObjectId objectId = cr::kInvalidObjectId;
    AdmissionStatus status = AdmissionStatus::NotRequested;
    cr::CreativeSemanticSelectionStatus selectionStatus =
        cr::CreativeSemanticSelectionStatus::NotRequested;
    cr::CreativeSemanticObjectActionRoute route =
        cr::CreativeSemanticObjectActionRoute::Reject;
    bool selectionResolved = false;
    bool allowed = false;
    cr::CreativeObjectId failedObjectId = cr::kInvalidObjectId;
    std::string_view reasonCode;
  };
  const std::array singleCases{
      SingleCase{authored, AdmissionStatus::Ready,
                 cr::CreativeSemanticSelectionStatus::Ready,
                 cr::CreativeSemanticObjectActionRoute::Document, true, true,
                 cr::kInvalidObjectId,
                 "creative_semantic_action_document"},
      SingleCase{patternOutput, AdmissionStatus::OwnershipRejected,
                 cr::CreativeSemanticSelectionStatus::Ready,
                 cr::CreativeSemanticObjectActionRoute::Reject, true, false,
                 patternOutput,
                 "creative_semantic_action_pattern_owned"},
      SingleCase{worldOutput, AdmissionStatus::OwnershipRejected,
                 cr::CreativeSemanticSelectionStatus::Ready,
                 cr::CreativeSemanticObjectActionRoute::Reject, true, false,
                 worldOutput,
                 "creative_semantic_action_world_layout_owned"},
      SingleCase{missing, AdmissionStatus::InvalidSelection,
                 cr::CreativeSemanticSelectionStatus::MissingObject,
                 cr::CreativeSemanticObjectActionRoute::Reject, false, false,
                 missing, "creative_selection_object_missing"}};
  bool ok = expect(recipe.accepted && recipe.changed,
                   "structural admission fixture recipe is valid");
  for (const SingleCase& testCase : singleCases) {
    const cr::CreativeStructuralMutationAdmission admission =
        cr::resolveCreativeStructuralMutationAdmission(
            document, testCase.objectId);
    ok = expect(admission.requested &&
                    admission.requestedObjectCount == 1U &&
                    admission.status == testCase.status &&
                    admission.selectionStatus ==
                        testCase.selectionStatus &&
                    admission.route == testCase.route &&
                    admission.selectionResolved ==
                        testCase.selectionResolved &&
                    admission.resolvedObjectCount ==
                        (testCase.selectionResolved ? 1U : 0U) &&
                    admission.primaryObjectId == testCase.objectId &&
                    admission.allowed == testCase.allowed &&
                    admission.failedObjectId ==
                        testCase.failedObjectId &&
                    admission.reasonCode == testCase.reasonCode &&
                    cr::creativeStructuralMutationOwnershipRejected(
                        admission) ==
                        (testCase.status ==
                         AdmissionStatus::OwnershipRejected),
                "single-object structural admission matches its table row") &&
         ok;
  }

  const std::array authoredSet{authored, patternSeed};
  const cr::CreativeStructuralMutationAdmission authoredAdmission =
      cr::resolveCreativeStructuralMutationAdmission(
          document, authoredSet, patternSeed);
  const std::array mixedSet{authored, patternOutput};
  const cr::CreativeStructuralMutationAdmission mixedAdmission =
      cr::resolveCreativeStructuralMutationAdmission(
          document, mixedSet, authored);
  const std::array missingSet{authored, missing};
  const cr::CreativeStructuralMutationAdmission missingAdmission =
      cr::resolveCreativeStructuralMutationAdmission(
          document, missingSet, authored);
  const std::array<cr::CreativeObjectId, 0U> emptySet{};
  const cr::CreativeStructuralMutationAdmission emptyAdmission =
      cr::resolveCreativeStructuralMutationAdmission(document, emptySet);

  return expect(authoredAdmission.allowed &&
                    authoredAdmission.selectionResolved &&
                    authoredAdmission.status == AdmissionStatus::Ready &&
                    authoredAdmission.selectionStatus ==
                        cr::CreativeSemanticSelectionStatus::Ready &&
                    authoredAdmission.route ==
                        cr::CreativeSemanticObjectActionRoute::Document &&
                    authoredAdmission.requestedObjectCount == 2U &&
                    authoredAdmission.resolvedObjectCount == 2U &&
                    authoredAdmission.primaryObjectId == patternSeed &&
                    authoredAdmission.failedObjectId ==
                        cr::kInvalidObjectId,
                "authored set admits one raw document mutation") &&
         expect(!mixedAdmission.allowed &&
                    mixedAdmission.status ==
                        AdmissionStatus::OwnershipRejected &&
                    mixedAdmission.selectionStatus ==
                        cr::CreativeSemanticSelectionStatus::Ready &&
                    mixedAdmission.route ==
                        cr::CreativeSemanticObjectActionRoute::Reject &&
                    mixedAdmission.failedObjectId == patternOutput &&
                    mixedAdmission.reasonCode ==
                        "creative_semantic_action_mixed_ownership",
                "mixed ownership identifies the first protected object") &&
         expect(!missingAdmission.allowed &&
                    !missingAdmission.selectionResolved &&
                    missingAdmission.status ==
                        AdmissionStatus::InvalidSelection &&
                    missingAdmission.selectionStatus ==
                        cr::CreativeSemanticSelectionStatus::MissingObject &&
                    missingAdmission.route ==
                        cr::CreativeSemanticObjectActionRoute::Reject &&
                    missingAdmission.resolvedObjectCount == 1U &&
                    missingAdmission.failedObjectId == missing &&
                    missingAdmission.reasonCode ==
                        "creative_selection_set_object_missing" &&
                    !cr::creativeStructuralMutationOwnershipRejected(
                        missingAdmission),
                "missing set member remains a domain validation failure") &&
         expect(!emptyAdmission.allowed &&
                    !emptyAdmission.selectionResolved &&
                    emptyAdmission.status ==
                        AdmissionStatus::InvalidSelection &&
                    emptyAdmission.requestedObjectCount == 0U &&
                    emptyAdmission.failedObjectId ==
                        cr::kInvalidObjectId &&
                    emptyAdmission.reasonCode ==
                        "creative_selection_set_empty",
                "empty set is invalid rather than an ownership conflict") &&
         ok;
}

bool orphanedGeneratedOutputFailsClosed() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Orphaned Generated Output");
  const cr::CreativeObjectId orphaned = addObject(
      document, "Orphaned Wall",
      {"creative_world_layout:retired_layout",
       "creative_world_layout_source:building/retired"});
  cr::CreativeWorldLayout currentLayout;
  currentLayout.stableKey = "current_layout";
  const cr::CreativeSemanticSelectionResolution single =
      cr::resolveCreativeSemanticSelection(
          document, orphaned, &currentLayout);
  const std::array selection{orphaned};
  const cr::CreativeSemanticSelectionSetResolution set =
      cr::resolveCreativeSemanticSelectionSet(
          document, selection, orphaned, &currentLayout);

  return expect(single.accepted && !single.worldLayoutSource.owned &&
                    single.primaryOwner ==
                        cr::CreativeSemanticSelectionOwner::WorldLayoutSource,
                "orphaned output retains World Layout ownership") &&
         expect(cr::resolveCreativeSemanticObjectAction(
                    single, cr::CreativeSemanticObjectAction::Inspect)
                    .allowed &&
                    !cr::resolveCreativeSemanticObjectAction(
                         single, cr::CreativeSemanticObjectAction::Delete)
                         .allowed &&
                    !cr::resolveCreativeSemanticObjectAction(
                         single, cr::CreativeSemanticObjectAction::Rename)
                         .allowed,
                "orphaned output remains inspectable but immutable") &&
         expect(set.accepted &&
                    set.primaryOwner ==
                        cr::CreativeSemanticSelectionOwner::WorldLayoutSource &&
                    set.worldLayoutOwnerCount == 1U &&
                    set.authoredOwnerCount == 0U &&
                    !cr::resolveCreativeSemanticObjectAction(
                         set, cr::CreativeSemanticObjectAction::Delete)
                         .allowed,
                "orphaned output sets never fall back to authored deletion");
}

}  // namespace

int main() {
  const bool ok = defaultStateIsEmpty() &&
                  settingSelectedTargetChangesState() &&
                  settingSameSelectedTargetIsNoChange() &&
                  clearingEmptySelectionIsNoChange() &&
                  clearingNonEmptySelectionClearsCandidate() &&
                  updatingCandidateDoesNotMutateSelected() &&
                  selectionRevisionTracksCommittedIdentityOnly() &&
                  applyingSelectObjectCandidateSelectsTarget() &&
                  invalidSelectObjectCandidateClearsSelection() &&
                  nonSelectionIntentIsNoOp() &&
                  selectionCapacityRejectsWithoutPartialChange() &&
                  semanticResolutionPreservesInspectionFacts() &&
                  semanticResolutionReportsInheritedInspectionFacts() &&
                  semanticResolutionPreservesNestedRecipeAncestry() &&
                  semanticSetResolutionFindsTheDeepestSharedOwner() &&
                  semanticActionDescriptorsCoverTheClosedActionSet() &&
                  semanticActionPolicyRoutesEveryOwnerClass() &&
                  structuralMutationAdmissionIsExplicitAndFailClosed() &&
                  orphanedGeneratedOutputFailsClosed();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
