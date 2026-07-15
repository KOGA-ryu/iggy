#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "runtime/collision/CollisionQuery.hpp"

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool addObject(cr::CreativeDocument& document,
               cr::CreativeObjectKind kind,
               std::string name,
               cr::CreativeVec3 position,
               std::optional<cr::CreativeBounds> bounds = std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  if (bounds.has_value()) {
    request.bounds = *bounds;
    request.hasBoundsOverride = true;
  }
  return document.createObject(request).accepted;
}

cr::CreativeDocumentCreateReceipt createObject(
    cr::CreativeDocument& document,
    cr::CreativeObjectKind kind,
    std::string name,
    cr::CreativeVec3 position,
    std::optional<cr::CreativeObjectId> parentId = std::nullopt,
    std::optional<cr::CreativeBounds> bounds = std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.parentId = parentId;
  if (bounds.has_value()) {
    request.bounds = *bounds;
    request.hasBoundsOverride = true;
  }
  return document.createObject(request);
}

cr::CreativeDocument playableDocument(bool includeActors = true,
                                      cr::CreativeDocumentId id = 41U) {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Runtime Sandbox Map");
  static_cast<void>(document.assignId(id));
  static_cast<void>(addObject(
      document, cr::CreativeObjectKind::Floor, "Sandbox Floor",
      {0.0, 0.0, 0.0},
      cr::CreativeBounds{{-5.0, 0.0, -5.0}, {5.0, 0.25, 5.0}}));
  static_cast<void>(addObject(document, cr::CreativeObjectKind::SpawnPoint,
                              "Player Spawn", {0.0, 0.25, 0.0}));
  if (includeActors) {
    static_cast<void>(addObject(document, cr::CreativeObjectKind::NpcSpawn,
                                "Friendly NPC", {2.0, 0.25, 0.0}));
    static_cast<void>(addObject(document, cr::CreativeObjectKind::EnemySpawn,
                                "Hostile Monster", {-2.0, 0.25, 0.0}));
  }
  return document;
}

cr::CreativePlayPreparationResult prepare(cr::CreativeDocument& document) {
  static iggy3d::StaticMeshAssetCatalog catalog;
  cr::CreativePlayPreparationRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = &catalog;
  request.roomId = "runtime_sandbox_room";
  return cr::prepareCreativePlay(request);
}

const iggy3d::RoomAnchorAsset* findAnchor(const iggy3d::RoomAsset& room,
                                         std::string_view kind) {
  const auto found = std::find_if(
      room.anchors.begin(), room.anchors.end(),
      [kind](const iggy3d::RoomAnchorAsset& anchor) {
        return anchor.kind == kind;
      });
  return found == room.anchors.end() ? nullptr : &*found;
}

const iggy3d::ScenarioAiActorSeed* findAiSeed(
    const iggy3d::FixtureScenarioSeed& seed,
    std::string_view stableName) {
  const auto found = std::find_if(
      seed.aiActors.begin(), seed.aiActors.end(),
      [stableName](const iggy3d::ScenarioAiActorSeed& actor) {
        return actor.actorStableName == stableName;
      });
  return found == seed.aiActors.end() ? nullptr : &*found;
}

const iggy3d::AiActorState* findAiActor(const iggy3d::SessionState& state,
                                       iggy3d::EntityId entity) {
  const auto found = std::find_if(
      state.ai.actors.begin(), state.ai.actors.end(),
      [entity](const iggy3d::AiActorState& actor) {
        return actor.actor == entity;
      });
  return found == state.ai.actors.end() ? nullptr : &*found;
}

const iggy3d::CombatantState* findCombatant(
    const iggy3d::SessionState& state,
    iggy3d::EntityId entity) {
  const auto found = std::find_if(
      state.combat.combatants.begin(), state.combat.combatants.end(),
      [entity](const iggy3d::CombatantState& combatant) {
        return combatant.entity == entity;
      });
  return found == state.combat.combatants.end() ? nullptr : &*found;
}

cr::CreativeRuntimeInteractableState* findInteractable(
    cr::CreativeRuntimeSandbox& sandbox,
    cr::CreativeObjectId objectId) {
  const auto found = std::find_if(
      sandbox.interactables.begin(), sandbox.interactables.end(),
      [objectId](const cr::CreativeRuntimeInteractableState& state) {
        return state.definition.objectId == objectId;
      });
  return found == sandbox.interactables.end() ? nullptr : &*found;
}

bool moveEntity(cr::CreativeRuntimeSandbox& sandbox,
                iggy3d::EntityId entityId,
                iggy3d::Vec3 position) {
  const iggy3d::EntityState* entity =
      sandbox.session.state().world.findById(entityId);
  if (entity == nullptr) {
    return false;
  }
  iggy3d::Transform3 transform = entity->transform;
  transform.position = position;
  return sandbox.session.mutableStateForOwnedSystems()
             .world.updateTransform(entityId, transform)
             .status == iggy3d::WorldStatus::Ok;
}

const cr::CreativeRuntimeDoorStateCommand* findCommand(
    const cr::CreativeRuntimeLogicActivationPlan& plan,
    cr::CreativeObjectId objectId) {
  const auto found = std::find_if(
      plan.commands.begin(), plan.commands.end(),
      [objectId](const cr::CreativeRuntimeDoorStateCommand& command) {
        return command.objectId == objectId;
      });
  return found == plan.commands.end() ? nullptr : &*found;
}

bool pureLogicPlannerPairsAutomaticSignals() {
  constexpr cr::CreativeObjectId kSource = 11U;
  const std::vector<cr::CreativeRuntimeLogicLink> links{
      {kSource, 21U, cr::CreativeLogicLinkAction::Toggle, false},
      {kSource, 22U, cr::CreativeLogicLinkAction::Open, false},
      {kSource, 23U, cr::CreativeLogicLinkAction::Close, false},
  };
  const std::vector<cr::CreativeRuntimeDoorStateFact> doors{
      {21U, false}, {22U, true}, {23U, true}};
  const cr::CreativeRuntimeLogicActivationPlan pulse =
      cr::planCreativeRuntimeLogicActivation(
          links, doors, kSource, cr::CreativeRuntimeLogicSignal::Pulse);
  const cr::CreativeRuntimeLogicActivationPlan activate =
      cr::planCreativeRuntimeLogicActivation(
          links, doors, kSource, cr::CreativeRuntimeLogicSignal::Activate);
  const std::vector<cr::CreativeRuntimeDoorStateFact> activatedDoors{
      {21U, true}, {22U, true}, {23U, false}};
  const cr::CreativeRuntimeLogicActivationPlan deactivate =
      cr::planCreativeRuntimeLogicActivation(
          links, activatedDoors, kSource,
          cr::CreativeRuntimeLogicSignal::Deactivate);
  const std::vector<cr::CreativeRuntimeLogicLink> missingDoorLinks{
      {kSource, 99U, cr::CreativeLogicLinkAction::Toggle, false}};
  const cr::CreativeRuntimeLogicActivationPlan missingDoor =
      cr::planCreativeRuntimeLogicActivation(
          missingDoorLinks, doors, kSource,
          cr::CreativeRuntimeLogicSignal::Pulse);
  const cr::CreativeRuntimeLogicActivationPlan invalidSignal =
      cr::planCreativeRuntimeLogicActivation(
          links, doors, kSource,
          static_cast<cr::CreativeRuntimeLogicSignal>(255U));
  const std::vector<cr::CreativeRuntimeDoorStateFact> duplicateDoors{
      {21U, false}, {21U, true}};
  const cr::CreativeRuntimeLogicActivationPlan duplicateDoorFacts =
      cr::planCreativeRuntimeLogicActivation(
          links, duplicateDoors, kSource,
          cr::CreativeRuntimeLogicSignal::Pulse);

  const cr::CreativeRuntimeDoorStateCommand* pulseToggle =
      findCommand(pulse, 21U);
  const cr::CreativeRuntimeDoorStateCommand* pulseOpen =
      findCommand(pulse, 22U);
  const cr::CreativeRuntimeDoorStateCommand* pulseClose =
      findCommand(pulse, 23U);
  const cr::CreativeRuntimeDoorStateCommand* activeToggle =
      findCommand(activate, 21U);
  const cr::CreativeRuntimeDoorStateCommand* inactiveToggle =
      findCommand(deactivate, 21U);
  const cr::CreativeRuntimeDoorStateCommand* inactiveOpen =
      findCommand(deactivate, 22U);
  const cr::CreativeRuntimeDoorStateCommand* inactiveClose =
      findCommand(deactivate, 23U);

  return expect(pulse.ok && pulse.commands.size() == 3U &&
                    pulseToggle != nullptr && pulseToggle->open &&
                    pulseOpen != nullptr && pulseOpen->open &&
                    pulseClose != nullptr && !pulseClose->open,
                "pulse applies authored Toggle Open and Close actions") &&
         expect(activate.ok && activeToggle != nullptr &&
                    activeToggle->open,
                "activation applies Toggle against the entry state") &&
         expect(deactivate.ok && inactiveToggle != nullptr &&
                    !inactiveToggle->open && inactiveOpen != nullptr &&
                    !inactiveOpen->open && inactiveClose != nullptr &&
                    inactiveClose->open,
                "deactivation reverses paired pressure-plate actions") &&
         expect(!missingDoor.ok && missingDoor.commands.empty(),
                "planner fails closed when a linked door fact is absent") &&
         expect(!invalidSignal.ok && !duplicateDoorFacts.ok,
                "planner rejects invalid enums and duplicate door facts");
}

bool pureSeedMapsPlayerAndActorPolicies() {
  cr::CreativeDocument document = playableDocument();
  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "seed setup prepares payload");
  }

  const cr::CreativePlayActivationPayload& payload = *prepared.payload;
  const iggy3d::RoomAnchorAsset* npc = findAnchor(payload.room, "npc");
  const iggy3d::RoomAnchorAsset* monster =
      findAnchor(payload.room, "monster");
  if (npc == nullptr || monster == nullptr) {
    return expect(false, "seed setup has actor anchors");
  }

  const cr::CreativeRuntimeScenarioSeedResult result =
      cr::buildCreativeRuntimeScenarioSeed(payload);
  const iggy3d::ScenarioAiActorSeed* npcAi =
      findAiSeed(result.seed, npc->runtimeStableName);
  const iggy3d::ScenarioAiActorSeed* monsterAi =
      findAiSeed(result.seed, monster->runtimeStableName);

  return expect(result.accepted &&
                    result.status ==
                        cr::CreativeRuntimeScenarioSeedStatus::Built &&
                    result.reasonCode == "creative_runtime_seed_built",
                "valid payload builds scenario seed") &&
         expect(result.summary.sourceAnchorCount == 3U &&
                    result.summary.playerEntityCount == 1U &&
                    result.summary.npcEntityCount == 1U &&
                    result.summary.monsterEntityCount == 1U &&
                    result.summary.ignoredAnchorCount == 0U,
                "seed summary separates runtime actor semantics") &&
         expect(result.seed.players.size() == 1U &&
                    result.seed.entities.size() == 3U &&
                    result.seed.entities.front().kind ==
                        iggy3d::ScenarioEntityKind::Player &&
                    result.seed.entities.front().stableName ==
                        payload.playerSpawn.runtimeStableName &&
                    result.seed.players.front().actorStableName ==
                        result.seed.entities.front().stableName,
                "player is first entity and owns local slot zero") &&
         expect(result.seed.entities.front().combatantEnabled &&
                    result.seed.entities.front().combatant.factionId == 1U &&
                    result.seed.entities.front().combatant.hitPoints == 10,
                "player combat defaults are explicit") &&
         expect(npcAi != nullptr && npcAi->behaviorProfileId == "passive" &&
                    monsterAi != nullptr &&
                    monsterAi->behaviorProfileId == "default",
                "npc and monster profiles follow semantic policy") &&
         expect(std::all_of(
                    result.seed.entities.begin() + 1U,
                    result.seed.entities.end(),
                    [](const iggy3d::ScenarioEntitySeed& entity) {
                      return iggy3d::isScenarioTargetActionSupported(
                                 entity.targeting,
                                 iggy3d::ScenarioTargetAction::Interact) &&
                             entity.interaction.kind ==
                                 iggy3d::ScenarioInteractionKind::Inspect &&
                             entity.interaction.repeatable;
                    }),
                "runtime actors expose repeatable inspect interaction") &&
         expect(result.seed.objectives.size() == 1U &&
                    result.seed.objectives.front().condition == "None" &&
                    result.seed.objectives.front().initialStatus ==
                        iggy3d::ObjectiveStatusSeed::Active,
                "sandbox objective is inert but runtime-valid") &&
         expect(cr::toString(result.status) == "built",
                "seed status string is stable");
}

bool activationOwnsCollisionSessionAndReasoning() {
  cr::CreativeDocument document = playableDocument();
  const std::uint64_t authoredRevision = document.revision();
  const std::size_t authoredObjectCount = document.objectCount();
  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "activation setup prepares payload");
  }

  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  if (!activated.sandbox.has_value()) {
    return expect(false, "activation returns sandbox");
  }

  cr::CreativeRuntimeSandbox& sandbox = *activated.sandbox;
  const iggy3d::EntityState* player =
      sandbox.session.state().world.findById({1U});
  const iggy3d::RoomAnchorAsset* spawn = findAnchor(sandbox.room, "spawn");
  const iggy3d::RoomAnchorAsset* npcAnchor = findAnchor(sandbox.room, "npc");
  const iggy3d::RoomAnchorAsset* monsterAnchor =
      findAnchor(sandbox.room, "monster");
  const iggy3d::EntityState* npc =
      npcAnchor == nullptr
          ? nullptr
          : sandbox.session.state().world.findByStableName(
                npcAnchor->runtimeStableName);
  const iggy3d::EntityState* monster =
      monsterAnchor == nullptr
          ? nullptr
          : sandbox.session.state().world.findByStableName(
                monsterAnchor->runtimeStableName);
  const iggy3d::CollisionQueryResult ground =
      spawn == nullptr
          ? iggy3d::CollisionQueryResult{}
          : iggy3d::sampleSurfaceHeight(sandbox.collisionSurfaces,
                                        spawn->positionMeters);

  bool ok = expect(activated.receipt.accepted &&
                       activated.receipt.status ==
                           cr::CreativeRuntimeSandboxActivationStatus::Activated &&
                       activated.receipt.reasonCode ==
                           "creative_runtime_sandbox_activated",
                   "runtime sandbox activates") &&
            expect(sandbox.sourceDocumentId == document.id() &&
                       sandbox.sourceDocumentRevision == authoredRevision &&
                       cr::creativeRuntimeSandboxIsCurrent(sandbox, document),
                   "sandbox carries current authoring provenance") &&
            expect(sandbox.collisionSurfaces.size() > 0U &&
                       sandbox.collisionSurfaces.size() ==
                           sandbox.room.spatialSurfaces.size() &&
                       activated.receipt.collisionSurfaceCount ==
                           sandbox.collisionSurfaces.size() &&
                       ground.status == iggy3d::CollisionQueryStatus::Hit &&
                       ground.heightMeters == 0.25F,
                   "sandbox owns queryable baked collision") &&
            expect(player != nullptr && spawn != nullptr &&
                       player->kind == iggy3d::EntityKind::Player &&
                       player->stableName == spawn->runtimeStableName &&
                       iggy3d::nearlyEqual(player->transform.position,
                                           spawn->positionMeters),
                   "session player starts at validated spawn") &&
            expect(npc != nullptr && monster != nullptr &&
                       npc->kind == iggy3d::EntityKind::Npc &&
                       monster->kind == iggy3d::EntityKind::Npc,
                   "npc and monster anchors become runtime entities") &&
            expect(npc != nullptr && monster != nullptr &&
                       findAiActor(sandbox.session.state(), npc->id) != nullptr &&
                       findAiActor(sandbox.session.state(), monster->id) !=
                           nullptr,
                   "runtime actors have AI state") &&
            expect(npc != nullptr && monster != nullptr &&
                       findCombatant(sandbox.session.state(), npc->id) !=
                           nullptr &&
                       findCombatant(sandbox.session.state(), monster->id) !=
                           nullptr &&
                       findCombatant(sandbox.session.state(), {1U}) != nullptr,
                   "runtime actors share a valid combat state") &&
            expect(sandbox.reasoningGraph.nodeCount == 3U &&
                       sandbox.session.state().reasoningGraph.nodes.size() ==
                           sandbox.reasoningGraph.nodeCount &&
                       activated.receipt.reasoningGraph.nodeCount ==
                           sandbox.reasoningGraph.nodeCount,
                   "activation installs anchor-derived reasoning data") &&
            expect(sandbox.session.lifecycle() ==
                       iggy3d::SessionLifecycle::Playing &&
                       activated.receipt.initialStateHash ==
                           sandbox.session.stateHash(),
                   "sandbox owns a playable temporary session") &&
            expect(document.revision() == authoredRevision &&
                       document.objectCount() == authoredObjectCount,
                   "activation does not mutate authored content");

  const iggy3d::StatusResult tick =
      sandbox.session.tick(&sandbox.collisionSurfaces);
  ok = ok && expect(tick.status == iggy3d::ResultStatus::Ok,
                    "sandbox session ticks against owned collision") &&
       expect(document.revision() == authoredRevision &&
                  document.objectCount() == authoredObjectCount,
              "runtime tick remains isolated from authored content") &&
       expect(cr::toString(activated.receipt.status) == "activated",
              "activation status string is stable");
  return ok;
}

bool activationRejectsStaleAndMalformedPayloads() {
  cr::CreativeDocument staleDocument = playableDocument(false, 42U);
  cr::CreativePlayPreparationResult stalePrepared = prepare(staleDocument);
  if (!stalePrepared.payload.has_value()) {
    return expect(false, "stale setup prepares payload");
  }
  const bool mutated = addObject(staleDocument,
                                 cr::CreativeObjectKind::NpcSpawn,
                                 "Late NPC",
                                 {1.0, 0.25, 1.0});
  cr::CreativeRuntimeSandboxActivationRequest staleRequest;
  staleRequest.sourceDocument = &staleDocument;
  staleRequest.payload = std::move(*stalePrepared.payload);
  const cr::CreativeRuntimeSandboxActivationResult stale =
      cr::activateCreativeRuntimeSandbox(std::move(staleRequest));

  cr::CreativeDocument malformedDocument = playableDocument(false, 43U);
  cr::CreativePlayPreparationResult malformedPrepared =
      prepare(malformedDocument);
  if (!malformedPrepared.payload.has_value() ||
      malformedPrepared.payload->room.spatialSurfaces.empty()) {
    return expect(false, "malformed setup prepares collision payload");
  }
  malformedPrepared.payload->room.spatialSurfaces.front().normal.x =
      std::numeric_limits<float>::quiet_NaN();
  cr::CreativeRuntimeSandboxActivationRequest malformedRequest;
  malformedRequest.sourceDocument = &malformedDocument;
  malformedRequest.payload = std::move(*malformedPrepared.payload);
  const cr::CreativeRuntimeSandboxActivationResult malformed =
      cr::activateCreativeRuntimeSandbox(std::move(malformedRequest));

  cr::CreativeDocument invalidConfigDocument = playableDocument(false, 44U);
  cr::CreativePlayPreparationResult invalidConfigPrepared =
      prepare(invalidConfigDocument);
  if (!invalidConfigPrepared.payload.has_value()) {
    return expect(false, "invalid config setup prepares payload");
  }
  cr::CreativeRuntimeSandboxActivationRequest invalidConfigRequest;
  invalidConfigRequest.sourceDocument = &invalidConfigDocument;
  invalidConfigRequest.payload = std::move(*invalidConfigPrepared.payload);
  invalidConfigRequest.config.runtimeConfig.fixedTickRateHz = 0U;
  const cr::CreativeRuntimeSandboxActivationResult invalidConfig =
      cr::activateCreativeRuntimeSandbox(std::move(invalidConfigRequest));

  cr::CreativeDocument invalidLinkDocument = playableDocument(false, 45U);
  cr::CreativePlayPreparationResult invalidLinkPrepared =
      prepare(invalidLinkDocument);
  if (!invalidLinkPrepared.payload.has_value()) {
    return expect(false, "invalid link setup prepares payload");
  }
  invalidLinkPrepared.payload->logicLinks.push_back(
      {999'998U, 999'999U, cr::CreativeLogicLinkAction::Toggle, false});
  cr::CreativeRuntimeSandboxActivationRequest invalidLinkRequest;
  invalidLinkRequest.sourceDocument = &invalidLinkDocument;
  invalidLinkRequest.payload = std::move(*invalidLinkPrepared.payload);
  const cr::CreativeRuntimeSandboxActivationResult invalidLink =
      cr::activateCreativeRuntimeSandbox(std::move(invalidLinkRequest));

  return expect(mutated && !stale.receipt.accepted &&
                    stale.receipt.status ==
                        cr::CreativeRuntimeSandboxActivationStatus::StalePayload &&
                    !stale.sandbox.has_value(),
                "revision drift rejects stale payload before activation") &&
         expect(!malformed.receipt.accepted &&
                    malformed.receipt.status ==
                        cr::CreativeRuntimeSandboxActivationStatus::
                            InvalidCollisionSurfaces &&
                    !malformed.sandbox.has_value(),
                "malformed collision fails closed") &&
         expect(!invalidConfig.receipt.accepted &&
                    invalidConfig.receipt.status ==
                        cr::CreativeRuntimeSandboxActivationStatus::InvalidConfig &&
                    !invalidConfig.sandbox.has_value(),
                "invalid runtime config fails before session creation") &&
         expect(!invalidLink.receipt.accepted &&
                    invalidLink.receipt.status ==
                        cr::CreativeRuntimeSandboxActivationStatus::InvalidPayload &&
                    !invalidLink.sandbox.has_value(),
                "runtime link endpoints fail closed before session creation");
}

bool sandboxFreshnessAndStopAreExplicit() {
  cr::CreativeDocument document = playableDocument(false, 45U);
  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "stop setup prepares payload");
  }
  const std::uint64_t revision = document.revision();
  const std::size_t objectCount = document.objectCount();

  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  if (!activated.sandbox.has_value()) {
    return expect(false, "stop setup activates sandbox");
  }

  const cr::CreativeRuntimeSandboxStopReceipt stopped =
      cr::stopCreativeRuntimeSandbox(activated.sandbox);
  const cr::CreativeRuntimeSandboxStopReceipt stoppedAgain =
      cr::stopCreativeRuntimeSandbox(activated.sandbox);

  return expect(stopped.requested && stopped.stopped &&
                    stopped.status ==
                        cr::CreativeRuntimeSandboxStopStatus::Stopped &&
                    stopped.documentId == document.id() &&
                    stopped.documentRevision == revision &&
                    stopped.finalStateHash != 0U &&
                    !activated.sandbox.has_value(),
                "stop receipt destroys active runtime owner") &&
         expect(stoppedAgain.requested && !stoppedAgain.stopped &&
                    stoppedAgain.status ==
                        cr::CreativeRuntimeSandboxStopStatus::NoActiveSandbox,
                "repeated stop reports no active sandbox") &&
         expect(document.revision() == revision &&
                    document.objectCount() == objectCount,
                "start and stop leave authored document untouched") &&
         expect(cr::toString(stopped.status) == "stopped" &&
                    cr::toString(stoppedAgain.status) == "no_active_sandbox",
                "stop status strings are stable");
}

bool runningSnapshotDoesNotTrackLaterDocumentEdits() {
  cr::CreativeDocument document = playableDocument(false, 46U);
  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "isolation setup prepares payload");
  }

  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  if (!activated.sandbox.has_value()) {
    return expect(false, "isolation setup activates sandbox");
  }

  const std::size_t runtimeAnchorCount = activated.sandbox->room.anchors.size();
  const std::size_t runtimeEntityCount =
      activated.sandbox->session.state().world.size();
  const bool added = addObject(document, cr::CreativeObjectKind::EnemySpawn,
                               "Post Activation Monster",
                               {3.0, 0.25, 0.0});

  return expect(added &&
                    !cr::creativeRuntimeSandboxIsCurrent(*activated.sandbox,
                                                         document),
                "later authoring revision marks sandbox stale") &&
         expect(activated.sandbox->room.anchors.size() ==
                    runtimeAnchorCount &&
                    activated.sandbox->session.state().world.size() ==
                        runtimeEntityCount,
                "running snapshot does not absorb later authoring edits");
}

bool automaticSourcesCountOccupantsAndRearm() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Automatic Logic Sources");
  static_cast<void>(document.assignId(48U));
  const bool floor = addObject(
      document, cr::CreativeObjectKind::Floor, "Floor", {0.0, 0.0, 0.0},
      cr::CreativeBounds{{-10.0, 0.0, -10.0}, {10.0, 0.25, 10.0}});
  const bool spawn = addObject(document, cr::CreativeObjectKind::SpawnPoint,
                               "Player Spawn", {-4.0, 0.25, 0.0});
  const bool npc = addObject(document, cr::CreativeObjectKind::NpcSpawn,
                             "Occupancy NPC", {-4.0, 0.25, 3.0});
  const cr::CreativeDocumentCreateReceipt trigger = createObject(
      document, cr::CreativeObjectKind::TriggerZone, "Entry Trigger",
      {-1.0, 0.25, -1.0}, std::nullopt,
      cr::CreativeBounds{{-1.0, 0.25, -1.0}, {1.0, 2.25, 1.0}});
  const cr::CreativeDocumentCreateReceipt plate = createObject(
      document, cr::CreativeObjectKind::PressurePlate, "Door Plate",
      {2.0, 0.25, -0.75}, std::nullopt,
      cr::CreativeBounds{{2.0, 0.25, -0.75}, {3.5, 0.35, 0.75}});
  const cr::CreativeDocumentCreateReceipt triggerDoor = createObject(
      document, cr::CreativeObjectKind::Door, "Trigger Door",
      {4.0, 0.25, -0.25}, std::nullopt,
      cr::CreativeBounds{{4.0, 0.25, -0.25}, {5.0, 2.5, 0.25}});
  const cr::CreativeDocumentCreateReceipt plateDoor = createObject(
      document, cr::CreativeObjectKind::Door, "Plate Door",
      {6.0, 0.25, -0.25}, std::nullopt,
      cr::CreativeBounds{{6.0, 0.25, -0.25}, {7.0, 2.5, 0.25}});
  if (!floor || !spawn || !npc || !trigger.accepted || !plate.accepted ||
      !triggerDoor.accepted || !plateDoor.accepted ||
      !document
           .setLogicLink({trigger.objectId, triggerDoor.objectId,
                          cr::CreativeLogicLinkAction::Toggle})
           .accepted ||
      !document
           .setLogicLink({plate.objectId, plateDoor.objectId,
                          cr::CreativeLogicLinkAction::Open})
           .accepted) {
    return expect(false, "automatic source setup creates authored map");
  }

  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "automatic source setup prepares payload");
  }
  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  if (!activated.sandbox.has_value()) {
    return expect(false, "automatic source sandbox activates");
  }

  cr::CreativeRuntimeSandbox& sandbox = *activated.sandbox;
  cr::CreativeRuntimeInteractableState* triggerState =
      findInteractable(sandbox, trigger.objectId);
  cr::CreativeRuntimeInteractableState* plateState =
      findInteractable(sandbox, plate.objectId);
  cr::CreativeRuntimeInteractableState* triggerDoorState =
      findInteractable(sandbox, triggerDoor.objectId);
  cr::CreativeRuntimeInteractableState* plateDoorState =
      findInteractable(sandbox, plateDoor.objectId);
  const iggy3d::RoomAnchorAsset* npcAnchor = findAnchor(sandbox.room, "npc");
  const iggy3d::EntityState* npcEntity =
      npcAnchor == nullptr
          ? nullptr
          : sandbox.session.state().world.findByStableName(
                npcAnchor->runtimeStableName);
  if (triggerState == nullptr || plateState == nullptr ||
      triggerDoorState == nullptr || plateDoorState == nullptr ||
      npcEntity == nullptr) {
    return expect(false, "automatic source runtime states are addressable");
  }
  const iggy3d::EntityId npcEntityId = npcEntity->id;
  const iggy3d::EntityState* triggerEntity =
      sandbox.session.state().world.findById(triggerState->entity);
  const iggy3d::EntityState* plateEntity =
      sandbox.session.state().world.findById(plateState->entity);

  const cr::CreativeRuntimeAutomaticLogicReceipt idle =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const cr::CreativeRuntimeInteractionEffectReceipt directAutomatic =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, triggerState->entity);
  if (!moveEntity(sandbox, {1U}, {0.0F, 0.25F, 0.0F})) {
    return expect(false, "player moves into trigger");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt triggerEntered =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const bool triggerOpened = triggerDoorState->doorOpen;
  const std::size_t triggerCountAfterEnter = triggerState->occupantCount;
  if (!moveEntity(sandbox, npcEntityId, {0.5F, 0.25F, 0.0F})) {
    return expect(false, "npc moves into occupied trigger");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt sharedTrigger =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const std::size_t sharedTriggerCount = triggerState->occupantCount;
  if (!moveEntity(sandbox, {1U}, {-4.0F, 0.25F, 0.0F})) {
    return expect(false, "player leaves shared trigger");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt oneTriggerOccupant =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const std::size_t oneTriggerOccupantCount = triggerState->occupantCount;
  if (!moveEntity(sandbox, npcEntityId, {-4.0F, 0.25F, 3.0F})) {
    return expect(false, "npc leaves trigger");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt triggerRearmed =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const std::size_t rearmedTriggerCount = triggerState->occupantCount;
  if (!moveEntity(sandbox, {1U}, {0.0F, 0.25F, 0.0F})) {
    return expect(false, "player re-enters trigger");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt triggerReentered =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const bool triggerClosed = !triggerDoorState->doorOpen;
  const cr::CreativeRuntimeOccupancyTransition triggerReentryTransition =
      triggerState->lastOccupancyTransition;

  if (!moveEntity(sandbox, {1U}, {2.5F, 0.25F, 0.0F})) {
    return expect(false, "player moves onto plate");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt plateEntered =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const bool plateOpened = plateDoorState->doorOpen;
  const std::size_t plateCountAfterEnter = plateState->occupantCount;
  if (!moveEntity(sandbox, npcEntityId, {2.75F, 0.25F, 0.0F})) {
    return expect(false, "npc moves onto occupied plate");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt sharedPlate =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const std::size_t sharedPlateCount = plateState->occupantCount;
  if (!moveEntity(sandbox, {1U}, {-4.0F, 0.25F, 0.0F})) {
    return expect(false, "player leaves shared plate");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt onePlateOccupant =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const std::size_t onePlateOccupantCount = plateState->occupantCount;
  if (!moveEntity(sandbox, npcEntityId, {-4.0F, 0.25F, 3.0F})) {
    return expect(false, "npc leaves plate");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt plateExited =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const std::size_t exitedPlateCount = plateState->occupantCount;

  return expect(activated.receipt.scenario.controlEntityCount == 2U &&
                    activated.receipt.scenario.automaticControlEntityCount ==
                        2U &&
                    triggerEntity != nullptr && plateEntity != nullptr &&
                    !triggerEntity->targeting.targetable &&
                    !plateEntity->targeting.targetable,
                "automatic sources seed as non-targetable controls") &&
         expect(idle.accepted &&
                    idle.status ==
                        cr::CreativeRuntimeAutomaticLogicStatus::NoTransition &&
                    idle.evaluatedSourceCount == 2U &&
                    idle.occupiedSourceCount == 0U,
                "empty automatic sources remain idle") &&
         expect(!directAutomatic.accepted &&
                    directAutomatic.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::
                            UnsupportedTarget,
                "automatic source cannot be activated through interaction") &&
         expect(triggerEntered.accepted &&
                    triggerEntered.occupancyTransitionCount == 1U &&
                    triggerEntered.activationEffectCount == 1U &&
                    triggerCountAfterEnter == 1U && triggerOpened,
                "first trigger entry pulses linked door") &&
         expect(sharedTrigger.occupancyTransitionCount == 0U &&
                    sharedTriggerCount == 2U &&
                    oneTriggerOccupant.occupancyTransitionCount == 0U &&
                    oneTriggerOccupantCount == 1U &&
                    triggerRearmed.occupancyTransitionCount == 1U &&
                    triggerRearmed.activationEffectCount == 0U &&
                    rearmedTriggerCount == 0U,
                "trigger counts actors and rearms only when empty") &&
         expect(triggerReentered.activationEffectCount == 1U && triggerClosed,
                "rearmed trigger pulses again on a later entry") &&
         expect(triggerReentryTransition ==
                        cr::CreativeRuntimeOccupancyTransition::Entered &&
                    cr::toString(triggerReentryTransition) ==
                        "entered" &&
                    cr::toString(triggerState->definition.logicSourceMode) ==
                        "pulse_on_enter" &&
                    cr::findCreativeRuntimeInteractableByObjectId(
                        sandbox, trigger.objectId) == triggerState &&
                    triggerState->lastOccupancyTransition ==
                        cr::CreativeRuntimeOccupancyTransition::Exited,
                "runtime source exposes stable monitor facts") &&
         expect(plateEntered.activationEffectCount == 1U && plateOpened &&
                    plateCountAfterEnter == 1U &&
                    sharedPlate.occupancyTransitionCount == 0U &&
                    sharedPlateCount == 2U &&
                    onePlateOccupant.occupancyTransitionCount == 0U &&
                    onePlateOccupantCount == 1U,
                "plate stays active until every occupant leaves") &&
         expect(plateExited.activationEffectCount == 1U &&
                    !plateDoorState->doorOpen &&
                    exitedPlateCount == 0U &&
                    plateExited.status ==
                        cr::CreativeRuntimeAutomaticLogicStatus::Applied &&
                    cr::toString(plateExited.status) == "applied" &&
                    plateState->lastOccupancyTransition ==
                        cr::CreativeRuntimeOccupancyTransition::Exited &&
                    cr::toString(plateState->definition.logicSourceMode) ==
                        "hold_while_occupied",
                "final plate exit reverses its Open action");
}

bool authoredInteractablesOwnExplicitCircuitsAndDynamicGeometry() {
  cr::CreativeDocument document = playableDocument(false, 47U);
  const cr::CreativeDocumentCreateReceipt circuit = createObject(
      document, cr::CreativeObjectKind::Group, "West Door Circuit",
      {0.0, 0.0, 0.0});
  const cr::CreativeDocumentCreateReceipt door = createObject(
      document, cr::CreativeObjectKind::Door, "West Door",
      {0.0, 0.25, -2.0}, circuit.objectId,
      cr::CreativeBounds{{-0.5, 0.25, -2.1}, {0.5, 2.5, -1.9}});
  const cr::CreativeDocumentCreateReceipt secondDoor = createObject(
      document, cr::CreativeObjectKind::Door, "East Door",
      {2.0, 0.25, -2.0}, circuit.objectId,
      cr::CreativeBounds{{1.5, 0.25, -2.1}, {2.5, 2.5, -1.9}});
  const cr::CreativeDocumentCreateReceipt button = createObject(
      document, cr::CreativeObjectKind::Button, "West Door Button",
      {0.5, 1.0, -1.7}, circuit.objectId);
  const cr::CreativeDocumentCreateReceipt lever = createObject(
      document, cr::CreativeObjectKind::Lever, "West Door Lever",
      {1.0, 1.0, -1.7}, circuit.objectId);
  const cr::CreativeDocumentCreateReceipt unlinked = createObject(
      document, cr::CreativeObjectKind::Switch, "Unlinked Switch",
      {-0.5, 1.0, -1.7});
  const cr::CreativeDocumentCreateReceipt pickup = createObject(
      document, cr::CreativeObjectKind::LootPoint, "Map Key",
      {1.0, 0.6, -1.0});
  if (!circuit.accepted || !door.accepted || !secondDoor.accepted ||
      !button.accepted || !lever.accepted || !unlinked.accepted ||
      !pickup.accepted) {
    return expect(false, "interactable setup creates authored objects");
  }
  const cr::CreativeLogicLinkMutationReceipt leverOpens =
      document.setLogicLink(
          {lever.objectId, door.objectId, cr::CreativeLogicLinkAction::Open});
  const cr::CreativeLogicLinkMutationReceipt leverCloses =
      document.setLogicLink({lever.objectId, secondDoor.objectId,
                             cr::CreativeLogicLinkAction::Close});
  if (!leverOpens.accepted || !leverCloses.accepted) {
    return expect(false, "interactable setup creates explicit links");
  }
  const std::uint64_t authoredRevision = document.revision();
  const std::size_t authoredObjectCount = document.objectCount();

  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "interactable setup prepares payload");
  }
  const cr::CreativeRuntimeScenarioSeedResult seed =
      cr::buildCreativeRuntimeScenarioSeed(*prepared.payload);
  if (!seed.accepted) {
    return expect(false, "interactable payload builds runtime seed");
  }
  const auto findDefinition = [&](cr::CreativeObjectId objectId) {
    return std::find_if(
        prepared.payload->interactables.begin(),
        prepared.payload->interactables.end(),
        [objectId](const cr::CreativeRuntimeInteractableDefinition& definition) {
          return definition.objectId == objectId;
        });
  };
  const auto doorDefinition = findDefinition(door.objectId);
  const auto buttonDefinition = findDefinition(button.objectId);
  const auto leverDefinition = findDefinition(lever.objectId);
  const auto unlinkedDefinition = findDefinition(unlinked.objectId);
  const auto pickupDefinition = findDefinition(pickup.objectId);
  if (doorDefinition == prepared.payload->interactables.end() ||
      buttonDefinition == prepared.payload->interactables.end() ||
      leverDefinition == prepared.payload->interactables.end() ||
      unlinkedDefinition == prepared.payload->interactables.end() ||
      pickupDefinition == prepared.payload->interactables.end()) {
    return expect(false, "catalog contains all authored interactables");
  }

  const std::string doorStableName = doorDefinition->stableName;
  const std::string buttonStableName = buttonDefinition->stableName;
  const std::string leverStableName = leverDefinition->stableName;
  const std::string unlinkedStableName = unlinkedDefinition->stableName;
  const std::string pickupStableName = pickupDefinition->stableName;
  const std::string pickupItemId = pickupDefinition->itemId;
  const std::size_t explicitLinkCount = static_cast<std::size_t>(std::count_if(
      prepared.payload->logicLinks.begin(), prepared.payload->logicLinks.end(),
      [](const cr::CreativeRuntimeLogicLink& link) {
        return !link.compatibilityFallback;
      }));
  const std::size_t compatibilityLinkCount =
      prepared.payload->logicLinks.size() - explicitLinkCount;
  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  if (!activated.sandbox.has_value()) {
    return expect(false, "interactable sandbox activates");
  }
  cr::CreativeRuntimeSandbox& sandbox = *activated.sandbox;
  const iggy3d::EntityState* doorEntity =
      sandbox.session.state().world.findByStableName(doorStableName);
  const iggy3d::EntityState* buttonEntity =
      sandbox.session.state().world.findByStableName(buttonStableName);
  const iggy3d::EntityState* leverEntity =
      sandbox.session.state().world.findByStableName(leverStableName);
  const iggy3d::EntityState* unlinkedEntity =
      sandbox.session.state().world.findByStableName(unlinkedStableName);
  const iggy3d::EntityState* pickupEntity =
      sandbox.session.state().world.findByStableName(pickupStableName);
  if (doorEntity == nullptr || buttonEntity == nullptr ||
      leverEntity == nullptr ||
      unlinkedEntity == nullptr || pickupEntity == nullptr) {
    return expect(false, "interactable seeds become runtime entities");
  }

  const std::size_t closedMeshCount = sandbox.room.staticMeshes.size();
  const std::size_t closedSurfaceCount = sandbox.room.spatialSurfaces.size();
  const std::size_t closedColliderCount = sandbox.collisionSurfaces.size();
  const std::vector<std::string> closedMeshOrder =
      sandbox.roomStaticMeshOrder;
  const std::vector<std::string> closedSurfaceOrder =
      sandbox.roomSpatialSurfaceOrder;
  const cr::CreativeRuntimeInteractionEffectReceipt opened =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, doorEntity->id);
  const bool doorMeshRemoved = std::none_of(
      sandbox.room.staticMeshes.begin(), sandbox.room.staticMeshes.end(),
      [&doorStableName](const iggy3d::RoomStaticMeshAsset& mesh) {
        return mesh.id == doorStableName;
      });
  const cr::CreativeRuntimeInteractionEffectReceipt closed =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, doorEntity->id);
  const cr::CreativeRuntimeInteractionEffectReceipt explicitApplied =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, leverEntity->id);
  const cr::CreativeRuntimeInteractionEffectReceipt explicitNoChange =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, leverEntity->id);
  const std::uint64_t liveGeometryRevision = sandbox.geometryRevision;
  sandbox.geometryRevision = std::numeric_limits<std::uint64_t>::max();
  const cr::CreativeRuntimeInteractionEffectReceipt saturatedNoChange =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, leverEntity->id);
  sandbox.geometryRevision = liveGeometryRevision;
  const cr::CreativeRuntimeInteractionEffectReceipt circuitOpened =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, buttonEntity->id);
  const cr::CreativeRuntimeInteractionEffectReceipt circuitClosed =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, buttonEntity->id);
  const cr::CreativeRuntimeInteractionEffectReceipt noLink =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, unlinkedEntity->id);
  const bool meshOrderRestored =
      sandbox.room.staticMeshes.size() == closedMeshOrder.size() &&
      std::equal(sandbox.room.staticMeshes.begin(),
                 sandbox.room.staticMeshes.end(), closedMeshOrder.begin(),
                 [](const iggy3d::RoomStaticMeshAsset& mesh,
                    const std::string& id) { return mesh.id == id; });
  const bool surfaceOrderRestored =
      sandbox.room.spatialSurfaces.size() == closedSurfaceOrder.size() &&
      std::equal(sandbox.room.spatialSurfaces.begin(),
                 sandbox.room.spatialSurfaces.end(),
                 closedSurfaceOrder.begin(),
                 [](const iggy3d::RoomSpatialSurface& surface,
                    const std::string& id) { return surface.id == id; });

  return expect(prepared.validation.passed &&
                    seed.summary.sourceInteractableCount == 6U &&
                    seed.summary.doorEntityCount == 2U &&
                    seed.summary.controlEntityCount == 3U &&
                    seed.summary.pickupEntityCount == 1U &&
                    seed.summary.ignoredAnchorCount == 0U &&
                    explicitLinkCount == 2U &&
                    compatibilityLinkCount == 2U,
                "catalog maps authored roles without proximity inference") &&
         expect(doorEntity->kind == iggy3d::EntityKind::Door &&
                    buttonEntity->kind == iggy3d::EntityKind::Marker &&
                    pickupEntity->kind == iggy3d::EntityKind::Pickup &&
                    pickupEntity->interaction.itemId == pickupItemId &&
                    pickupEntity->interaction.deactivateTargetOnSuccess,
                "scenario entities expose deterministic interaction contracts") &&
         expect(opened.accepted && opened.changed &&
                    opened.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::DoorOpened &&
                    opened.geometryRevision == 1U && doorMeshRemoved &&
                    sandbox.geometryRevision == 5U,
                "direct door activation publishes an open geometry revision") &&
         expect(closed.accepted &&
                    closed.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::DoorClosed &&
                    closedMeshCount == sandbox.room.staticMeshes.size() &&
                    closedSurfaceCount == sandbox.room.spatialSurfaces.size() &&
                    closedColliderCount == sandbox.collisionSurfaces.size(),
                "closing restores exact geometry and collision cardinality") &&
         expect(explicitApplied.accepted && explicitApplied.changed &&
                    explicitApplied.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::LinksApplied &&
                    explicitApplied.affectedDoorCount == 2U &&
                    explicitNoChange.accepted && !explicitNoChange.changed &&
                    explicitNoChange.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::LinksNoChange &&
                    saturatedNoChange.accepted &&
                    !saturatedNoChange.changed &&
                    saturatedNoChange.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::LinksNoChange,
                "authored Open and Close links apply exactly and then no-op") &&
         expect(circuitOpened.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::CircuitOpened &&
                    circuitOpened.affectedDoorCount == 2U &&
                    circuitClosed.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::CircuitClosed &&
                    circuitClosed.affectedDoorCount == 2U &&
                    meshOrderRestored && surfaceOrderRestored,
                "shared parent circuit toggles linked doors and restores order") &&
         expect(noLink.accepted && !noLink.changed &&
                    noLink.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::NoLinkedDoor,
                "ungrouped control reports no link instead of guessing") &&
         expect(document.revision() == authoredRevision &&
                    document.objectCount() == authoredObjectCount,
                "runtime interactable effects leave authored content untouched");
}

}  // namespace

int main() {
  const bool ok = pureLogicPlannerPairsAutomaticSignals() &&
                  pureSeedMapsPlayerAndActorPolicies() &&
                  activationOwnsCollisionSessionAndReasoning() &&
                  activationRejectsStaleAndMalformedPayloads() &&
                  sandboxFreshnessAndStopAreExplicit() &&
                  runningSnapshotDoesNotTrackLaterDocumentEdits() &&
                  automaticSourcesCountOccupantsAndRearm() &&
                  authoredInteractablesOwnExplicitCircuitsAndDynamicGeometry();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
