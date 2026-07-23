#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/recipes/AuthoringContract.hpp"
#include "app/iggy3d/creative/recipes/ObjectLibraryRecipe.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool authoringFamilyContractsAreExhaustiveAndEnforceLifecycleLaws() {
  const std::span<const cr::CreativeAuthoringContract> contracts =
      cr::creativeAuthoringContracts();
  const cr::CreativeAuthoringContract* terrain =
      cr::findCreativeAuthoringContract(cr::CreativeAuthoringFamily::Terrain);
  const cr::CreativeAuthoringContract* volume =
      cr::findCreativeAuthoringContract(cr::CreativeAuthoringFamily::Volume);
  const cr::CreativeAuthoringContract* road =
      cr::findCreativeAuthoringContract(cr::CreativeAuthoringFamily::Road);
  const cr::CreativeAuthoringContract* watercourse =
      cr::findCreativeAuthoringContract(
          cr::CreativeAuthoringFamily::Watercourse);
  const cr::CreativeAuthoringContract* bridge =
      cr::findCreativeAuthoringContract(cr::CreativeAuthoringFamily::Bridge);
  const cr::CreativeAuthoringContract* retainingEdge =
      cr::findCreativeAuthoringContract(
          cr::CreativeAuthoringFamily::RetainingEdge);

  auto brokenParametric = cr::kCreativeAuthoringContracts;
  brokenParametric[static_cast<std::size_t>(
      cr::CreativeAuthoringFamily::Terrain)]
      .capabilities &=
      static_cast<cr::CreativeAuthoringCapabilities>(
          ~cr::capability(cr::CreativeAuthoringCapability::Reconcile));
  auto brokenDestructive = cr::kCreativeAuthoringContracts;
  brokenDestructive[static_cast<std::size_t>(
      cr::CreativeAuthoringFamily::Volume)]
      .capabilities |=
      cr::capability(cr::CreativeAuthoringCapability::DurableSource);

  cr::CreativeAuthoringOperationRecord record;
  record.family = cr::CreativeAuthoringFamily::Volume;
  record.kind = cr::CreativeAuthoringOperationKind::Apply;
  record.lifecycle = cr::CreativeAuthoringLifecycle::Destructive;
  record.action = "volume_fill";
  record.requestFingerprint = 0x1234U;
  const cr::CreativeAuthoringOperationRecord validRecord = record;
  record.lifecycle = cr::CreativeAuthoringLifecycle::Parametric;
  const auto factoryRecord = cr::makeCreativeAuthoringOperationRecord(
      cr::CreativeAuthoringFamily::Terrain,
      cr::CreativeAuthoringOperationKind::Apply, "terrain_apply", 0x5678U,
      12U);
  const auto reconcileRecord = cr::makeCreativeAuthoringOperationRecord(
      cr::CreativeAuthoringFamily::Terrain,
      cr::CreativeAuthoringOperationKind::Reconcile, "terrain_reconcile",
      0x6789U, 4U);
  const auto destructiveRecord = cr::makeCreativeAuthoringOperationRecord(
      cr::CreativeAuthoringFamily::Prefab,
      cr::CreativeAuthoringOperationKind::Destructive, "prefab_detach",
      0x789aU, 3U);

  return expect(
             contracts.size() ==
                     static_cast<std::size_t>(
                         cr::CreativeAuthoringFamily::Count) &&
                 cr::validateCreativeAuthoringContracts(contracts).valid(),
             "authoring family table is exhaustive and valid") &&
         expect(terrain != nullptr &&
                    terrain->sourceStore ==
                        cr::CreativeAuthoringSourceStore::
                            TerrainOperationStack &&
                    cr::creativeAuthoringContractHas(
                        *terrain,
                        cr::CreativeAuthoringCapability::DurableSource) &&
                    cr::creativeAuthoringContractHas(
                        *terrain,
                        cr::CreativeAuthoringCapability::Reconcile),
                "terrain declares durable parametric ownership") &&
         expect(road != nullptr && watercourse != nullptr &&
                    bridge != nullptr && retainingEdge != nullptr &&
                    road->sourceStore ==
                        cr::CreativeAuthoringSourceStore::WorldLayoutSource &&
                    watercourse->sourceStore ==
                        cr::CreativeAuthoringSourceStore::WorldLayoutSource &&
                    bridge->sourceStore ==
                        cr::CreativeAuthoringSourceStore::WorldLayoutSource &&
                    retainingEdge->sourceStore ==
                        cr::CreativeAuthoringSourceStore::WorldLayoutSource,
                "structural and authored path recipe families are explicit") &&
         expect(volume != nullptr &&
                    volume->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Destructive &&
                    volume->sourceStore ==
                        cr::CreativeAuthoringSourceStore::None &&
                    cr::creativeAuthoringContractHas(
                        *volume,
                        cr::CreativeAuthoringCapability::DestructiveRecord),
                "volume declares honest destructive ownership") &&
         expect(cr::validateCreativeAuthoringContracts(brokenParametric)
                        .status ==
                    cr::CreativeAuthoringContractValidationStatus::
                        ParametricContractInvalid,
                "parametric family cannot lose reconciliation") &&
         expect(cr::validateCreativeAuthoringContracts(brokenDestructive)
                        .status ==
                    cr::CreativeAuthoringContractValidationStatus::
                        DestructiveContractInvalid,
                "destructive family cannot invent durable source") &&
         expect(cr::validateCreativeAuthoringOperationRecord(validRecord) &&
                    !cr::validateCreativeAuthoringOperationRecord(record),
                "operation record lifecycle must match its family contract") &&
         expect(factoryRecord.has_value() && reconcileRecord.has_value() &&
                    destructiveRecord.has_value() &&
                    factoryRecord->kind ==
                        cr::CreativeAuthoringOperationKind::Apply &&
                    factoryRecord->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Parametric &&
                    factoryRecord->affectedMemberCount == 12U &&
                    reconcileRecord->kind ==
                        cr::CreativeAuthoringOperationKind::Reconcile &&
                    reconcileRecord->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Parametric &&
                    destructiveRecord->kind ==
                        cr::CreativeAuthoringOperationKind::Destructive &&
                    destructiveRecord->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Destructive &&
                    !cr::makeCreativeAuthoringOperationRecord(
                         cr::CreativeAuthoringFamily::Count,
                         cr::CreativeAuthoringOperationKind::Apply, "invalid",
                         0x5678U, 1U)
                         .has_value() &&
                    !cr::makeCreativeAuthoringOperationRecord(
                         cr::CreativeAuthoringFamily::Terrain,
                         cr::CreativeAuthoringOperationKind::Apply, "",
                         0x5678U, 1U)
                         .has_value() &&
                    !cr::makeCreativeAuthoringOperationRecord(
                         cr::CreativeAuthoringFamily::Terrain,
                         cr::CreativeAuthoringOperationKind::Apply,
                         "terrain_apply", 0U, 1U)
                         .has_value() &&
                    !cr::makeCreativeAuthoringOperationRecord(
                         cr::CreativeAuthoringFamily::Terrain,
                         cr::CreativeAuthoringOperationKind::Destructive,
                         "terrain_destroy", 0x5678U, 1U)
                         .has_value() &&
                    !cr::makeCreativeAuthoringOperationRecord(
                         cr::CreativeAuthoringFamily::Volume,
                         cr::CreativeAuthoringOperationKind::Reconcile,
                         "volume_reconcile", 0x5678U, 1U)
                         .has_value(),
                "operation record factory derives phase lifecycle and fails closed");
}

cr::CreativeRecipePlan parentedRecipe() {
  cr::CreativeRecipePlan plan;
  plan.kind = cr::CreativeRecipeKind::Building;
  plan.instanceKey = "recipe_house";
  plan.instanceName = "Recipe House";

  cr::CreativeRecipeObjectPlan root;
  root.createRequest.kind = cr::CreativeObjectKind::Room;
  root.createRequest.name = "Recipe House";
  root.role = cr::CreativeRecipeObjectRole::Source;
  root.stableKey = "root";
  plan.objects.push_back(root);

  cr::CreativeRecipeObjectPlan floor;
  floor.createRequest.kind = cr::CreativeObjectKind::Floor;
  floor.createRequest.name = "Recipe Floor";
  floor.role = cr::CreativeRecipeObjectRole::Generated;
  floor.stableKey = "floor.main";
  floor.parentObjectIndex = 0U;
  plan.objects.push_back(floor);
  return plan;
}

cr::CreativeAppState appStateWithDocument(cr::CreativeDocumentId id) {
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Recipe Test");
  static_cast<void>(document.assignId(id));
  static_cast<void>(appState.facade.installDocument(std::move(document)));
  return appState;
}

bool symbolicParentAndProvenanceMaterializeDeterministically() {
  const cr::CreativeRecipeMaterializeResult result =
      cr::materializeCreativeRecipe(parentedRecipe(), 40U);

  return expect(result.receipt.accepted, "recipe materialization accepted") &&
         expect(result.receipt.status == cr::CreativeRecipeStatus::Ready,
                "recipe materialization ready") &&
         expect(result.receipt.objectCount == 2U,
                "recipe materialization object count") &&
         expect(result.receipt.sourceObjectCount == 1U &&
                    result.receipt.generatedObjectCount == 1U,
                "recipe materialization role counts") &&
         expect(result.receipt.resolvedParentCount == 1U,
                "recipe materialization parent count") &&
         expect(result.createRequests.size() == 2U,
                "recipe materialization request count") &&
         expect(!result.createRequests[0].parentId.has_value(),
                "recipe source remains root") &&
         expect(result.createRequests[1].parentId == 40U,
                "recipe child resolves source id") &&
         expect(cr::creativeRecipeRequestHasProvenance(
                    result.createRequests[0], cr::CreativeRecipeKind::Building,
                    cr::CreativeRecipeObjectRole::Source, "root"),
                "recipe source provenance") &&
         expect(cr::creativeRecipeRequestHasProvenance(
                    result.createRequests[1], cr::CreativeRecipeKind::Building,
                    cr::CreativeRecipeObjectRole::Generated, "floor.main"),
                "recipe generated provenance") &&
         expect(cr::creativeRecipeRequestHasInstanceProvenance(
                    result.createRequests[1], cr::CreativeRecipeKind::Building,
                    "recipe_house", cr::CreativeRecipeObjectRole::Generated,
                    "floor.main"),
                "recipe instance provenance");
}

bool explicitObjectIdsResolveStableParentIdentity() {
  const std::array<cr::CreativeObjectId, 2> objectIds{41U, 99U};
  const cr::CreativeRecipeMaterializeResult materialized =
      cr::materializeCreativeRecipe(parentedRecipe(), objectIds);
  const std::array<cr::CreativeObjectId, 2> duplicateIds{41U, 41U};
  const cr::CreativeRecipeMaterializeResult duplicate =
      cr::materializeCreativeRecipe(parentedRecipe(), duplicateIds);
  const std::array<cr::CreativeObjectId, 1> shortIds{41U};
  const cr::CreativeRecipeMaterializeResult shortMap =
      cr::materializeCreativeRecipe(parentedRecipe(), shortIds);

  return expect(materialized.receipt.accepted &&
                    materialized.receipt.firstObjectId == 41U &&
                    materialized.createRequests.size() == 2U &&
                    materialized.createRequests[1].parentId == 41U,
                "explicit id map resolves symbolic parent") &&
         expect(!duplicate.receipt.accepted &&
                    duplicate.receipt.reasonCode ==
                        "creative_recipe_object_ids_invalid" &&
                    duplicate.createRequests.empty(),
                "duplicate explicit ids fail closed") &&
         expect(!shortMap.receipt.accepted &&
                    shortMap.receipt.reasonCode ==
                        "creative_recipe_object_ids_invalid" &&
                    shortMap.createRequests.empty(),
                "incomplete explicit id map fails closed");
}

bool invalidKeysParentsAndAllocatorOverflowFailClosed() {
  cr::CreativeRecipePlan duplicate = parentedRecipe();
  duplicate.objects[1].stableKey = "root";
  const cr::CreativeRecipeMaterializeResult duplicateResult =
      cr::materializeCreativeRecipe(duplicate, 1U);

  cr::CreativeRecipePlan forward = parentedRecipe();
  forward.objects[0].parentObjectIndex = 1U;
  const cr::CreativeRecipeMaterializeResult forwardResult =
      cr::materializeCreativeRecipe(forward, 1U);

  const cr::CreativeRecipeMaterializeResult overflowResult =
      cr::materializeCreativeRecipe(
          parentedRecipe(), std::numeric_limits<cr::CreativeObjectId>::max());
  cr::CreativeRecipePlan finalId = parentedRecipe();
  finalId.objects.resize(1U);
  const cr::CreativeRecipeMaterializeResult finalIdResult =
      cr::materializeCreativeRecipe(
          finalId, std::numeric_limits<cr::CreativeObjectId>::max());

  return expect(!duplicateResult.receipt.accepted &&
                    duplicateResult.receipt.status ==
                        cr::CreativeRecipeStatus::DuplicateStableKey,
                "duplicate recipe key rejected") &&
         expect(duplicateResult.createRequests.empty(),
                "duplicate recipe emits no partial requests") &&
         expect(!forwardResult.receipt.accepted &&
                    forwardResult.receipt.status ==
                        cr::CreativeRecipeStatus::InvalidParentReference,
                "forward recipe parent rejected") &&
         expect(!overflowResult.receipt.accepted &&
                    overflowResult.receipt.status ==
                        cr::CreativeRecipeStatus::ObjectIdOverflow,
                "recipe allocator overflow rejected") &&
         expect(!finalIdResult.receipt.accepted &&
                    finalIdResult.receipt.status ==
                        cr::CreativeRecipeStatus::ObjectIdOverflow,
                "recipe allocator preserves invalid-id sentinel after create");
}

bool definitionFingerprintPinsSemanticOutputAndRejectsStalePlans() {
  cr::CreativeRecipePlan plan = parentedRecipe();
  plan.objects[0].createRequest.hasBoundsOverride = true;
  plan.objects[0].createRequest.bounds = {
      {0.0, 0.0, 0.0}, {4.0, 3.0, 5.0}};
  const std::uint64_t fingerprint = cr::fingerprintCreativeRecipePlan(plan);
  cr::CreativeRecipePlan identical = plan;
  const std::uint64_t identicalFingerprint =
      cr::fingerprintCreativeRecipePlan(identical);
  identical.objects[1].createRequest.name = "Changed Floor";
  const std::uint64_t changedFingerprint =
      cr::fingerprintCreativeRecipePlan(identical);

  plan.definitionFingerprint = fingerprint;
  const cr::CreativeRecipeMaterializeResult materialized =
      cr::materializeCreativeRecipe(plan, 20U);

  cr::CreativeRecipePlan stale = plan;
  stale.objects[0].createRequest.bounds.max.x = 8.0;
  const cr::CreativeRecipeMaterializeResult staleResult =
      cr::materializeCreativeRecipe(stale, 20U);

  cr::CreativeRecipePlan invalid = parentedRecipe();
  invalid.objects[0].createRequest.hasTransformOverride = true;
  invalid.objects[0].createRequest.transform.position.x =
      std::numeric_limits<double>::quiet_NaN();

  cr::CreativeRecipePlan doorPlan = parentedRecipe();
  cr::CreativeDocumentCreateRequest& doorRequest =
      doorPlan.objects[1].createRequest;
  doorRequest.kind = cr::CreativeObjectKind::Door;
  doorRequest.name = "Fingerprint Door";
  doorRequest.hasDoorSettingsOverride = true;
  doorRequest.door.leafArrangement =
      cr::CreativeDoorLeafArrangement::Double;
  doorRequest.door.hingeSide = cr::CreativeDoorHingeSide::MaximumEdge;
  doorRequest.door.swingSide = cr::CreativeDoorSwingSide::NegativeNormal;
  doorRequest.door.initialState = cr::CreativeDoorInitialState::Open;
  doorRequest.door.gameplayLocked = true;
  doorRequest.door.transitionSeconds = 0.8;
  const std::uint64_t doorPlanFingerprint =
      cr::fingerprintCreativeRecipePlan(doorPlan);
  const std::uint64_t doorObjectFingerprint =
      cr::fingerprintCreativeRecipeObjectPlan(doorPlan, 1U);
  cr::CreativeRecipePlan changedDoorPlan = doorPlan;
  changedDoorPlan.objects[1].createRequest.door.gameplayLocked = false;
  const std::uint64_t changedDoorPlanFingerprint =
      cr::fingerprintCreativeRecipePlan(changedDoorPlan);
  const std::uint64_t changedDoorObjectFingerprint =
      cr::fingerprintCreativeRecipeObjectPlan(changedDoorPlan, 1U);

  cr::CreativeObject doorState;
  doorState.id = 9U;
  doorState.kind = cr::CreativeObjectKind::Door;
  doorState.name = doorRequest.name;
  doorState.door = doorRequest.door;
  const std::uint64_t doorStateFingerprint =
      cr::fingerprintCreativeRecipeObjectState(doorState, {});
  doorState.door.transitionSeconds = 1.25;
  const std::uint64_t changedDoorStateFingerprint =
      cr::fingerprintCreativeRecipeObjectState(doorState, {});

  cr::CreativeRecipePlan windowPlan = parentedRecipe();
  cr::CreativeDocumentCreateRequest& windowRequest =
      windowPlan.objects[1].createRequest;
  windowRequest.kind = cr::CreativeObjectKind::Window;
  windowRequest.name = "Fingerprint Window";
  windowRequest.hasWindowSettingsOverride = true;
  windowRequest.window.insertKind =
      cr::CreativeWindowInsertKind::PairedShutters;
  const std::uint64_t windowPlanFingerprint =
      cr::fingerprintCreativeRecipePlan(windowPlan);
  const std::uint64_t windowObjectFingerprint =
      cr::fingerprintCreativeRecipeObjectPlan(windowPlan, 1U);
  cr::CreativeRecipePlan changedWindowPlan = windowPlan;
  changedWindowPlan.objects[1].createRequest.window.insertKind =
      cr::CreativeWindowInsertKind::Glazing;
  const std::uint64_t changedWindowPlanFingerprint =
      cr::fingerprintCreativeRecipePlan(changedWindowPlan);
  const std::uint64_t changedWindowObjectFingerprint =
      cr::fingerprintCreativeRecipeObjectPlan(changedWindowPlan, 1U);

  cr::CreativeObject windowState;
  windowState.id = 10U;
  windowState.kind = cr::CreativeObjectKind::Window;
  windowState.name = windowRequest.name;
  windowState.window = windowRequest.window;
  const std::uint64_t windowStateFingerprint =
      cr::fingerprintCreativeRecipeObjectState(windowState, {});
  windowState.window.insertKind = cr::CreativeWindowInsertKind::Glazing;
  const std::uint64_t changedWindowStateFingerprint =
      cr::fingerprintCreativeRecipeObjectState(windowState, {});

  return expect(fingerprint != 0U && fingerprint == identicalFingerprint,
                "identical recipe semantics have one fingerprint") &&
         expect(changedFingerprint != 0U &&
                    changedFingerprint != fingerprint,
                "materialized recipe changes alter the fingerprint") &&
         expect(materialized.receipt.accepted &&
                    materialized.createRequests.size() == 2U &&
                    cr::creativeRecipeRequestHasDefinitionFingerprint(
                        materialized.createRequests[0], fingerprint) &&
                    cr::creativeRecipeRequestHasDefinitionFingerprint(
                        materialized.createRequests[1], fingerprint),
                "materialization stamps every member with its definition") &&
         expect(!staleResult.receipt.accepted &&
                    staleResult.receipt.status ==
                        cr::CreativeRecipeStatus::InvalidRecipe &&
                    staleResult.receipt.reasonCode ==
                        "creative_recipe_definition_fingerprint_stale",
                "stale declared recipe fingerprint fails closed") &&
         expect(doorPlanFingerprint != 0U &&
                    doorObjectFingerprint != 0U &&
                    changedDoorPlanFingerprint != doorPlanFingerprint &&
                    changedDoorObjectFingerprint != doorObjectFingerprint,
                "door lock semantics participate in plan fingerprints") &&
         expect(doorStateFingerprint != 0U &&
                    changedDoorStateFingerprint != doorStateFingerprint,
                "door transition semantics participate in live fingerprints") &&
         expect(windowPlanFingerprint != 0U &&
                    windowObjectFingerprint != 0U &&
                    changedWindowPlanFingerprint != windowPlanFingerprint &&
                    changedWindowObjectFingerprint != windowObjectFingerprint,
                "window treatment participates in plan fingerprints") &&
         expect(windowStateFingerprint != 0U &&
                    changedWindowStateFingerprint != windowStateFingerprint,
                "window treatment participates in live fingerprints") &&
         expect(cr::fingerprintCreativeRecipePlan(invalid) == 0U,
                "non-finite recipe semantics cannot be fingerprinted");
}

bool generatedOutputFingerprintDetectsLaterSemanticRefinement() {
  cr::CreativeRecipePlan plan = parentedRecipe();
  plan.objects[0].createRequest.hasBoundsOverride = true;
  plan.objects[0].createRequest.bounds = {
      {0.0, 0.0, 0.0}, {4.0, 3.0, 5.0}};
  plan.objects[1].createRequest.hasBoundsOverride = true;
  plan.objects[1].createRequest.bounds = {
      {0.0, -0.1, 0.0}, {4.0, 0.0, 5.0}};
  plan.definitionFingerprint = cr::fingerprintCreativeRecipePlan(plan);
  const std::uint64_t rootPlanFingerprint =
      cr::fingerprintCreativeRecipeObjectPlan(plan, 0U);
  const std::uint64_t floorPlanFingerprint =
      cr::fingerprintCreativeRecipeObjectPlan(plan, 1U);
  const cr::CreativeRecipeMaterializeResult materialized =
      cr::materializeCreativeRecipe(plan, 1U);

  cr::CreativeDocument document = cr::CreativeDocument::create("Output Fingerprint");
  static_cast<void>(document.assignId(73U));
  for (const cr::CreativeDocumentCreateRequest& request :
       materialized.createRequests) {
    static_cast<void>(document.createObject(request));
  }
  const cr::CreativeObject* root = document.findObject(1U);
  const cr::CreativeObject* floor = document.findObject(2U);
  if (root == nullptr || floor == nullptr) {
    return expect(false, "fingerprint fixture materialized both objects");
  }
  const std::uint64_t storedRoot =
      cr::creativeRecipeObjectOutputFingerprint(*root);
  const std::uint64_t storedFloor =
      cr::creativeRecipeObjectOutputFingerprint(*floor);
  const std::uint64_t currentRoot =
      cr::fingerprintCreativeRecipeObjectState(*root, {});
  const std::uint64_t currentFloor =
      cr::fingerprintCreativeRecipeObjectState(
          *floor, cr::creativeRecipeObjectStableKey(*root));

  const cr::CreativeDocumentMutationReceipt moved = cr::moveDocumentObject(
      document, floor->id, {3.0, 2.0, 1.0});
  const cr::CreativeObject* refinedFloor = document.findObject(2U);
  const std::uint64_t refinedFingerprint =
      refinedFloor != nullptr
          ? cr::fingerprintCreativeRecipeObjectState(
                *refinedFloor, cr::creativeRecipeObjectStableKey(*root))
          : 0U;

  return expect(materialized.receipt.accepted &&
                    rootPlanFingerprint != 0U && floorPlanFingerprint != 0U,
                "semantic object plans produce nonzero fingerprints") &&
         expect(storedRoot == rootPlanFingerprint &&
                    storedFloor == floorPlanFingerprint &&
                    currentRoot == storedRoot && currentFloor == storedFloor,
                "fresh materialized state matches its immutable baseline") &&
         expect(moved.changed && refinedFingerprint != 0U &&
                    refinedFingerprint != storedFloor &&
                    cr::creativeRecipeObjectOutputFingerprint(*refinedFloor) ==
                        storedFloor,
                "manual mutation changes live state without rewriting baseline");
}

bool historyApplyIsAtomicAndCreatesOneUndoStep() {
  cr::CreativeAppState appState = appStateWithDocument(71U);
  const cr::CreativeRecipePlan plan = parentedRecipe();
  const cr::CreativeRecipeApplyReceipt applied =
      cr::applyCreativeRecipeWithHistory(appState, plan, "recipe_test");
  const cr::CreativeObject* root = appState.facade.document().findObject(1U);
  const cr::CreativeObject* floor = appState.facade.document().findObject(2U);
  const bool hierarchyCommitted = floor != nullptr && floor->parentId == 1U;
  const bool sourceProvenanceCommitted =
      root != nullptr && cr::creativeRecipeObjectHasProvenance(
                             *root, cr::CreativeRecipeKind::Building,
                             cr::CreativeRecipeObjectRole::Source, "root");
  const cr::CreativeAuthoringOperationRecord* operation =
      cr::creativeHistoryTargetOperation(
          appState.history, cr::CreativeHistoryDirection::Undo);
  const std::optional<cr::CreativeAuthoringOperationRecord> expectedOperation =
      operation != nullptr
          ? std::optional<cr::CreativeAuthoringOperationRecord>{*operation}
          : std::nullopt;
  const cr::CreativeHistoryApplyReceipt undone = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(applied.accepted && applied.changed,
                "recipe history apply accepted") &&
         expect(applied.status == cr::CreativeRecipeStatus::Applied,
                "recipe history apply status") &&
         expect(applied.createReceipt.appliedCreateCount == 2U,
                "recipe atomic create count") &&
         expect(applied.historyReceipt.recorded &&
                    cr::creativeUndoDepth(appState.history) == 0U,
                "recipe undo consumes sole snapshot") &&
         expect(root != nullptr && floor != nullptr,
                "recipe objects existed before undo") &&
         expect(hierarchyCommitted, "recipe hierarchy committed") &&
         expect(sourceProvenanceCommitted,
                "recipe committed source provenance") &&
         expect(expectedOperation.has_value() &&
                    expectedOperation->family ==
                        cr::CreativeAuthoringFamily::Building &&
                    expectedOperation->kind ==
                        cr::CreativeAuthoringOperationKind::Apply &&
                    expectedOperation->lifecycle ==
                        cr::CreativeAuthoringLifecycle::Parametric &&
                    expectedOperation->action == "Building" &&
                    expectedOperation->requestFingerprint ==
                        cr::fingerprintCreativeRecipePlan(plan) &&
                    expectedOperation->affectedMemberCount == 2U,
                "recipe history records its exact family plan") &&
         expect(undone.accepted && undone.changed,
                "recipe one-step undo accepted") &&
         expect(undone.targetOperation == expectedOperation,
                "recipe operation metadata survives undo") &&
         expect(appState.facade.document().objectCount() == 0U,
                "recipe undo removes complete transaction") &&
         expect(cr::creativeRedoDepth(appState.history) == 1U,
                "recipe undo creates one redo snapshot");
}

bool rejectedAtomicApplyPreservesDocumentAndHistory() {
  cr::CreativeAppState appState = appStateWithDocument(72U);
  cr::CreativeRecipePlan plan = parentedRecipe();
  plan.objects.erase(plan.objects.begin());
  plan.objects[0].parentObjectIndex.reset();
  plan.objects[0].createRequest.parentId = 999U;

  const cr::CreativeRecipeApplyReceipt applied =
      cr::applyCreativeRecipeWithHistory(appState, plan, "recipe_reject");
  return expect(!applied.accepted && !applied.changed,
                "rejected recipe not accepted") &&
         expect(applied.status == cr::CreativeRecipeStatus::ApplyRejected,
                "rejected recipe status") &&
         expect(appState.facade.document().objectCount() == 0U &&
                    appState.facade.document().revision() == 0U,
                "rejected recipe preserves document") &&
         expect(cr::creativeUndoDepth(appState.history) == 0U &&
                    cr::creativeRedoDepth(appState.history) == 0U,
                "rejected recipe records no history");
}

bool objectLibraryRecipeOwnsBoundedAndPointPlacementParity() {
  cr::CreativeObjectLibraryRecipeRequest request;
  request.stableKey = "estate_props";
  request.name = "Estate Props";

  cr::CreativeObjectLibraryPlacementSpec bridge;
  bridge.kind = cr::CreativeObjectKind::Bridge;
  bridge.stableKey = "bridge.ditch";
  bridge.name = "Ditch Bridge";
  bridge.bounds = {{1.0, 2.0, 3.0}, {5.0, 2.5, 7.0}};
  bridge.tags = {"map_template:test"};
  request.placements.push_back(bridge);

  cr::CreativeObjectLibraryPlacementSpec spawn;
  spawn.kind = cr::CreativeObjectKind::SpawnPoint;
  spawn.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  spawn.stableKey = "anchor.player";
  spawn.name = "Player Arrival";
  spawn.point = {8.0, 3.0, -2.0};
  request.placements.push_back(spawn);

  cr::CreativeObjectLibraryPlacementSpec asset;
  asset.kind = cr::CreativeObjectKind::Rock;
  asset.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  asset.stableKey = "asset.boulder";
  asset.name = "Boulder";
  asset.assetId = "boulder_01";
  asset.point = {12.0, 1.5, 4.0};
  asset.assetSourceBounds = {{-1.0, 0.0, -0.5}, {1.0, 2.0, 0.5}};
  asset.hasAssetSourceBounds = true;
  asset.yawRadians = 1.5707963267948966;
  asset.scale = {2.0, 0.5, 1.5};
  request.placements.push_back(asset);

  const cr::CreativeObjectLibraryRecipeResult recipe =
      cr::buildCreativeObjectLibraryRecipe(request);
  const cr::CreativeRecipeMaterializeResult materialized =
      cr::materializeCreativeRecipe(recipe.plan, 20U);
  const bool placementParity =
      materialized.createRequests.size() == 3U &&
      materialized.createRequests[0].kind == cr::CreativeObjectKind::Bridge &&
      materialized.createRequests[0].hasBoundsOverride &&
      materialized.createRequests[0].bounds.min.x == 1.0 &&
      materialized.createRequests[0].bounds.max.z == 7.0 &&
      materialized.createRequests[1].kind ==
          cr::CreativeObjectKind::SpawnPoint &&
      materialized.createRequests[1].hasTransformOverride &&
      materialized.createRequests[1].transform.position.x == 8.0 &&
      materialized.createRequests[1].transform.position.y == 3.0 &&
      materialized.createRequests[1].transform.position.z == -2.0 &&
      materialized.createRequests[2].assetId == "boulder_01" &&
      materialized.createRequests[2].hasTransformOverride &&
      materialized.createRequests[2].hasBoundsOverride &&
      materialized.createRequests[2].bounds.min.x == 11.0 &&
      materialized.createRequests[2].bounds.max.y == 3.5 &&
      materialized.createRequests[2].transform.rotationEulerRadians.y ==
          asset.yawRadians &&
      materialized.createRequests[2].transform.scale.x == 2.0 &&
      materialized.createRequests[2].transform.scale.y == 0.5 &&
      materialized.createRequests[2].transform.scale.z == 1.5;

  return expect(recipe.receipt.accepted &&
                    recipe.receipt.status ==
                        cr::CreativeObjectLibraryRecipeStatus::Ready,
                "object library recipe accepted") &&
         expect(recipe.plan.kind == cr::CreativeRecipeKind::ObjectLibrary &&
                    recipe.receipt.boundedPlacementCount == 1U &&
                    recipe.receipt.pointPlacementCount == 2U,
                "object library recipe owns placement modes") &&
         expect(materialized.receipt.accepted && placementParity,
                "object library materialization preserves exact placement") &&
         expect(cr::creativeRecipeRequestHasInstanceProvenance(
                    materialized.createRequests[0],
                    cr::CreativeRecipeKind::ObjectLibrary, "estate_props",
                    cr::CreativeRecipeObjectRole::Source, "bridge.ditch") &&
                    cr::creativeRecipeRequestHasInstanceProvenance(
                        materialized.createRequests[1],
                        cr::CreativeRecipeKind::ObjectLibrary,
                        "estate_props", cr::CreativeRecipeObjectRole::Source,
                        "anchor.player"),
                "object library emits shared recipe provenance");
}

bool invalidObjectLibraryPlacementsFailWithoutPartialPlan() {
  cr::CreativeObjectLibraryRecipeRequest invalidBounds;
  invalidBounds.stableKey = "invalid_bounds";
  invalidBounds.name = "Invalid Bounds";
  cr::CreativeObjectLibraryPlacementSpec bridge;
  bridge.kind = cr::CreativeObjectKind::Bridge;
  bridge.stableKey = "bridge";
  bridge.name = "Bridge";
  bridge.bounds = {{0.0, 0.0, 0.0},
                   {std::numeric_limits<double>::quiet_NaN(), 1.0, 1.0}};
  invalidBounds.placements.push_back(bridge);
  const cr::CreativeObjectLibraryRecipeResult bounded =
      cr::buildCreativeObjectLibraryRecipe(invalidBounds);

  cr::CreativeObjectLibraryRecipeRequest invalidPoint;
  invalidPoint.stableKey = "invalid_point";
  invalidPoint.name = "Invalid Point";
  cr::CreativeObjectLibraryPlacementSpec spawn;
  spawn.kind = cr::CreativeObjectKind::SpawnPoint;
  spawn.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  spawn.stableKey = "spawn";
  spawn.name = "Spawn";
  spawn.point = {0.0, std::numeric_limits<double>::infinity(), 0.0};
  invalidPoint.placements.push_back(spawn);
  const cr::CreativeObjectLibraryRecipeResult point =
      cr::buildCreativeObjectLibraryRecipe(invalidPoint);

  return expect(!bounded.receipt.accepted && bounded.plan.objects.empty() &&
                    bounded.receipt.status ==
                        cr::CreativeObjectLibraryRecipeStatus::InvalidPlacement,
                "non-finite object library bounds reject atomically") &&
         expect(!point.receipt.accepted && point.plan.objects.empty() &&
                    point.receipt.status ==
                        cr::CreativeObjectLibraryRecipeStatus::InvalidPlacement,
                "non-finite object library point rejects atomically");
}

}  // namespace

int main() {
  const bool ok =
      authoringFamilyContractsAreExhaustiveAndEnforceLifecycleLaws() &&
      symbolicParentAndProvenanceMaterializeDeterministically() &&
      explicitObjectIdsResolveStableParentIdentity() &&
      invalidKeysParentsAndAllocatorOverflowFailClosed() &&
      definitionFingerprintPinsSemanticOutputAndRejectsStalePlans() &&
      generatedOutputFingerprintDetectsLaterSemanticRefinement() &&
      historyApplyIsAtomicAndCreatesOneUndoStep() &&
      rejectedAtomicApplyPreservesDocumentAndHistory() &&
      objectLibraryRecipeOwnsBoundedAndPointPlacementParity() &&
      invalidObjectLibraryPlacementsFailWithoutPartialPlan();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
