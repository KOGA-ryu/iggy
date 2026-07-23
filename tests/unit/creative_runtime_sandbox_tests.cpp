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
  cr::CreativeDocumentCreateRequest spawnRequest;
  spawnRequest.kind = cr::CreativeObjectKind::SpawnPoint;
  spawnRequest.name = "Player Spawn";
  spawnRequest.transform.position = {0.0, 0.25, 0.0};
  spawnRequest.transform.rotationEulerRadians.y = 0.75;
  spawnRequest.hasTransformOverride = true;
  spawnRequest.playerSpawn.validationRadiusMeters = 0.65;
  spawnRequest.playerSpawn.fallbackPriority = 3U;
  spawnRequest.hasPlayerSpawnSettingsOverride = true;
  static_cast<void>(document.createObject(spawnRequest));
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

const cr::CreativeRuntimeLogicTargetStateCommand* findCommand(
    const cr::CreativeRuntimeLogicActivationPlan& plan,
    cr::CreativeObjectId objectId) {
  const auto found = std::find_if(
      plan.commands.begin(), plan.commands.end(),
      [objectId](const cr::CreativeRuntimeLogicTargetStateCommand& command) {
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
      {kSource, 24U, cr::CreativeLogicLinkAction::Enable, false},
      {kSource, 25U, cr::CreativeLogicLinkAction::Disable, false},
  };
  using Kind = cr::CreativeRuntimeInteractableKind;
  const std::vector<cr::CreativeRuntimeLogicTargetStateFact> targets{
      {21U, Kind::Door, false},     {22U, Kind::Door, true},
      {23U, Kind::Door, true},      {24U, Kind::Platform, false},
      {25U, Kind::Platform, true},
  };
  const cr::CreativeRuntimeLogicActivationPlan pulse =
      cr::planCreativeRuntimeLogicActivation(
          links, targets, kSource, cr::CreativeRuntimeLogicSignal::Pulse);
  const cr::CreativeRuntimeLogicActivationPlan activate =
      cr::planCreativeRuntimeLogicActivation(
          links, targets, kSource, cr::CreativeRuntimeLogicSignal::Activate);
  const std::vector<cr::CreativeRuntimeLogicTargetStateFact> activatedTargets{
      {21U, Kind::Door, true},      {22U, Kind::Door, true},
      {23U, Kind::Door, false},     {24U, Kind::Platform, true},
      {25U, Kind::Platform, false},
  };
  const cr::CreativeRuntimeLogicActivationPlan deactivate =
      cr::planCreativeRuntimeLogicActivation(
          links, activatedTargets, kSource,
          cr::CreativeRuntimeLogicSignal::Deactivate);
  const std::vector<cr::CreativeRuntimeLogicLink> missingTargetLinks{
      {kSource, 99U, cr::CreativeLogicLinkAction::Toggle, false}};
  const cr::CreativeRuntimeLogicActivationPlan missingTarget =
      cr::planCreativeRuntimeLogicActivation(
          missingTargetLinks, targets, kSource,
          cr::CreativeRuntimeLogicSignal::Pulse);
  const std::vector<cr::CreativeRuntimeLogicLink> invalidPlatformLinks{
      {kSource, 24U, cr::CreativeLogicLinkAction::Open, false}};
  const cr::CreativeRuntimeLogicActivationPlan invalidPlatformAction =
      cr::planCreativeRuntimeLogicActivation(
          invalidPlatformLinks, targets, kSource,
          cr::CreativeRuntimeLogicSignal::Pulse);
  const std::vector<cr::CreativeRuntimeLogicLink> invalidCompatibilityLinks{
      {kSource, 21U, cr::CreativeLogicLinkAction::Open, true}};
  const cr::CreativeRuntimeLogicActivationPlan invalidCompatibilityAction =
      cr::planCreativeRuntimeLogicActivation(
          invalidCompatibilityLinks, targets, kSource,
          cr::CreativeRuntimeLogicSignal::Pulse);
  const cr::CreativeRuntimeLogicActivationPlan invalidSignal =
      cr::planCreativeRuntimeLogicActivation(
          links, targets, kSource,
          static_cast<cr::CreativeRuntimeLogicSignal>(255U));
  const std::vector<cr::CreativeRuntimeLogicTargetStateFact> duplicateTargets{
      {21U, Kind::Door, false}, {21U, Kind::Door, true}};
  const cr::CreativeRuntimeLogicActivationPlan duplicateTargetFacts =
      cr::planCreativeRuntimeLogicActivation(
          links, duplicateTargets, kSource,
          cr::CreativeRuntimeLogicSignal::Pulse);

  const cr::CreativeRuntimeLogicTargetStateCommand* pulseToggle =
      findCommand(pulse, 21U);
  const cr::CreativeRuntimeLogicTargetStateCommand* pulseOpen =
      findCommand(pulse, 22U);
  const cr::CreativeRuntimeLogicTargetStateCommand* pulseClose =
      findCommand(pulse, 23U);
  const cr::CreativeRuntimeLogicTargetStateCommand* pulseEnable =
      findCommand(pulse, 24U);
  const cr::CreativeRuntimeLogicTargetStateCommand* pulseDisable =
      findCommand(pulse, 25U);
  const cr::CreativeRuntimeLogicTargetStateCommand* activeToggle =
      findCommand(activate, 21U);
  const cr::CreativeRuntimeLogicTargetStateCommand* inactiveToggle =
      findCommand(deactivate, 21U);
  const cr::CreativeRuntimeLogicTargetStateCommand* inactiveOpen =
      findCommand(deactivate, 22U);
  const cr::CreativeRuntimeLogicTargetStateCommand* inactiveClose =
      findCommand(deactivate, 23U);
  const cr::CreativeRuntimeLogicTargetStateCommand* inactiveEnable =
      findCommand(deactivate, 24U);
  const cr::CreativeRuntimeLogicTargetStateCommand* inactiveDisable =
      findCommand(deactivate, 25U);

  return expect(pulse.ok && pulse.commands.size() == 5U &&
                    pulseToggle != nullptr && pulseToggle->active &&
                    pulseOpen != nullptr && pulseOpen->active &&
                    pulseClose != nullptr && !pulseClose->active &&
                    pulseEnable != nullptr && pulseEnable->active &&
                    pulseDisable != nullptr && !pulseDisable->active,
                "pulse applies target-specific door and platform actions") &&
         expect(activate.ok && activeToggle != nullptr &&
                    activeToggle->active,
                "activation applies Toggle against the entry state") &&
         expect(deactivate.ok && inactiveToggle != nullptr &&
                    !inactiveToggle->active && inactiveOpen != nullptr &&
                    !inactiveOpen->active && inactiveClose != nullptr &&
                    inactiveClose->active && inactiveEnable != nullptr &&
                    !inactiveEnable->active && inactiveDisable != nullptr &&
                    inactiveDisable->active,
                "deactivation reverses paired pressure-plate actions") &&
         expect(!missingTarget.ok && missingTarget.commands.empty(),
                "planner fails closed when a linked target fact is absent") &&
         expect(!invalidPlatformAction.ok && !invalidCompatibilityAction.ok &&
                    !invalidSignal.ok && !duplicateTargetFacts.ok,
                "planner rejects incompatible actions and invalid facts");
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
                    result.seed.entities.front().combatant.hitPoints == 10 &&
                    result.seed.entities.front()
                            .transform.rotationEulerRadians.y ==
                        payload.playerSpawnYawRadians &&
                    payload.playerSpawnSettings.validationRadiusMeters == 0.65 &&
                    payload.playerSpawnSettings.fallbackPriority == 3U,
                "player profile pose and combat defaults are explicit") &&
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
  const cr::CreativeObjectId expectedSpawnObjectId =
      prepared.payload->playerSpawnObjectId;
  const std::string expectedPlayerProfileId =
      prepared.payload->playerSpawnSettings.playerProfileId;
  const float expectedSpawnYawRadians =
      prepared.payload->playerSpawnYawRadians;

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
                                           spawn->positionMeters) &&
                       player->transform.rotationEulerRadians.y ==
                           expectedSpawnYawRadians &&
                       sandbox.playerSpawnObjectId == expectedSpawnObjectId &&
                       sandbox.playerProfileId == expectedPlayerProfileId &&
                       sandbox.playerSpawnYawRadians ==
                           expectedSpawnYawRadians &&
                       activated.receipt.playerProfileId ==
                           expectedPlayerProfileId,
                   "session player consumes validated spawn identity and pose") &&
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
  const bool triggerOpened = triggerDoorState->targetActive;
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
  const bool triggerClosed = !triggerDoorState->targetActive;
  const cr::CreativeRuntimeOccupancyTransition triggerReentryTransition =
      triggerState->lastOccupancyTransition;

  if (!moveEntity(sandbox, {1U}, {2.5F, 0.25F, 0.0F})) {
    return expect(false, "player moves onto plate");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt plateEntered =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const bool plateOpened = plateDoorState->targetActive;
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
                    !plateDoorState->targetActive &&
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

bool holdSourceInitializesBeforeFirstRuntimeTick() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Initialized Hold Source");
  static_cast<void>(document.assignId(49U));
  const bool floor = addObject(
      document, cr::CreativeObjectKind::Floor, "Floor", {0.0, 0.0, 0.0},
      cr::CreativeBounds{{-8.0, 0.0, -8.0}, {8.0, 0.25, 8.0}});
  const bool spawn = addObject(document, cr::CreativeObjectKind::SpawnPoint,
                               "Player Spawn", {-4.0, 0.25, 0.0});
  const cr::CreativeDocumentCreateReceipt plate = createObject(
      document, cr::CreativeObjectKind::PressurePlate, "Platform Plate",
      {0.0, 0.25, 0.0}, std::nullopt,
      cr::CreativeBounds{{-1.0, 0.25, -1.0}, {1.0, 0.35, 1.0}});
  const cr::CreativeDocumentCreateReceipt platform = createObject(
      document, cr::CreativeObjectKind::Platform, "Lift Platform",
      {4.0, 0.25, 0.0}, std::nullopt,
      cr::CreativeBounds{{3.0, 0.25, -1.0}, {5.0, 0.6, 1.0}});
  if (!floor || !spawn || !plate.accepted || !platform.accepted ||
      !document
           .setLogicLink({plate.objectId, platform.objectId,
                          cr::CreativeLogicLinkAction::Enable})
           .accepted) {
    return expect(false, "hold initialization fixture is authored");
  }

  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "hold initialization fixture prepares");
  }
  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  if (!activated.sandbox.has_value()) {
    return expect(false, "hold initialization fixture activates");
  }

  cr::CreativeRuntimeSandbox& sandbox = *activated.sandbox;
  cr::CreativeRuntimeInteractableState* plateState =
      findInteractable(sandbox, plate.objectId);
  cr::CreativeRuntimeInteractableState* platformState =
      findInteractable(sandbox, platform.objectId);
  if (plateState == nullptr || platformState == nullptr) {
    return expect(false, "initialized hold states are addressable");
  }
  const bool initiallyRetracted =
      !platformState->targetActive && plateState->occupantCount == 0U &&
      sandbox.geometryRevision == 1U &&
      std::none_of(sandbox.room.staticMeshes.begin(),
                   sandbox.room.staticMeshes.end(),
                   [&platformState](const iggy3d::RoomStaticMeshAsset& mesh) {
                     return mesh.id == platformState->definition.roomMeshId;
                   });

  if (!moveEntity(sandbox, {1U}, {0.0F, 0.25F, 0.0F})) {
    return expect(false, "player moves onto initialized pressure plate");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt entered =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const bool enabled = platformState->targetActive &&
                       plateState->occupantCount == 1U &&
                       sandbox.geometryRevision == 2U;
  if (!moveEntity(sandbox, {1U}, {-4.0F, 0.25F, 0.0F})) {
    return expect(false, "player leaves initialized pressure plate");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt exited =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);

  return expect(initiallyRetracted,
                "empty pressure plate retracts Enable target at activation") &&
         expect(entered.accepted && entered.changed &&
                    entered.status ==
                        cr::CreativeRuntimeAutomaticLogicStatus::Applied &&
                    enabled,
                "first occupied transition enables initialized platform") &&
         expect(exited.accepted && exited.changed &&
                    !platformState->targetActive &&
                    plateState->occupantCount == 0U &&
                    sandbox.geometryRevision == 3U,
                "final exit returns initialized platform to inactive state");
}

bool holdSourceRetriesOccupiedPlatformRestore() {
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Blocked Hold Restore");
  static_cast<void>(document.assignId(50U));
  const bool floor = addObject(
      document, cr::CreativeObjectKind::Floor, "Floor", {0.0, 0.0, 0.0},
      cr::CreativeBounds{{-8.0, 0.0, -8.0}, {8.0, 0.25, 8.0}});
  const bool spawn = addObject(document, cr::CreativeObjectKind::SpawnPoint,
                               "Player Spawn", {-4.0, 0.25, 0.0});
  const cr::CreativeDocumentCreateReceipt plate = createObject(
      document, cr::CreativeObjectKind::PressurePlate, "Retract Plate",
      {0.0, 0.25, 0.0}, std::nullopt,
      cr::CreativeBounds{{-0.5, 0.25, -0.5}, {0.5, 0.35, 0.5}});
  const cr::CreativeDocumentCreateReceipt platform = createObject(
      document, cr::CreativeObjectKind::Platform, "Wide Platform",
      {0.0, 0.25, 0.0}, std::nullopt,
      cr::CreativeBounds{{-2.0, 0.25, -2.0}, {2.0, 0.6, 2.0}});
  if (!floor || !spawn || !plate.accepted || !platform.accepted ||
      !document
           .setLogicLink({plate.objectId, platform.objectId,
                          cr::CreativeLogicLinkAction::Disable})
           .accepted) {
    return expect(false, "blocked hold restore fixture is authored");
  }

  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "blocked hold restore fixture prepares");
  }
  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  if (!activated.sandbox.has_value()) {
    return expect(false, "blocked hold restore fixture activates");
  }

  cr::CreativeRuntimeSandbox& sandbox = *activated.sandbox;
  cr::CreativeRuntimeInteractableState* plateState =
      findInteractable(sandbox, plate.objectId);
  cr::CreativeRuntimeInteractableState* platformState =
      findInteractable(sandbox, platform.objectId);
  if (plateState == nullptr || platformState == nullptr ||
      !platformState->targetActive || sandbox.geometryRevision != 0U) {
    return expect(false, "Disable target starts enabled while plate is empty");
  }

  if (!moveEntity(sandbox, {1U}, {0.0F, 0.25F, 0.0F})) {
    return expect(false, "player enters retract plate");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt retracted =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const bool retractedState =
      !platformState->targetActive && plateState->occupantCount == 1U &&
      sandbox.geometryRevision == 1U;
  if (!moveEntity(sandbox, {1U}, {1.5F, 0.25F, 0.0F})) {
    return expect(false, "player leaves plate inside retracted platform");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt blocked =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);
  const bool blockedStatePreserved =
      !platformState->targetActive && plateState->occupantCount == 1U &&
      sandbox.geometryRevision == 1U;
  if (!moveEntity(sandbox, {1U}, {-4.0F, 0.25F, 0.0F})) {
    return expect(false, "player clears retracted platform");
  }
  const cr::CreativeRuntimeAutomaticLogicReceipt restored =
      cr::updateCreativeRuntimeAutomaticLogic(sandbox);

  return expect(retracted.accepted && retracted.changed && retractedState,
                "plate entry retracts Disable target") &&
         expect(blocked.accepted && !blocked.changed &&
                    blocked.status ==
                        cr::CreativeRuntimeAutomaticLogicStatus::EffectBlocked &&
                    blocked.lastEffect ==
                        cr::CreativeRuntimeInteractionEffectStatus::
                            TargetOccupied &&
                    blocked.reasonCode ==
                        "creative_runtime_platform_enable_occupied" &&
                    cr::toString(blocked.status) == "effect_blocked" &&
                    cr::toString(blocked.lastEffect) == "target_occupied" &&
                    blockedStatePreserved,
                "occupied automatic restore is nonfatal and remains armed") &&
         expect(restored.accepted && restored.changed &&
                    restored.status ==
                        cr::CreativeRuntimeAutomaticLogicStatus::Applied &&
                    platformState->targetActive &&
                    plateState->occupantCount == 0U &&
                    sandbox.geometryRevision == 2U,
                "armed hold source restores after platform clears");
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
  const bool doorMeshRetained = std::any_of(
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
                        cr::CreativeRuntimeInteractionEffectStatus::DoorOpening &&
                    opened.geometryRevision == 0U && doorMeshRetained &&
                    sandbox.geometryRevision == 0U,
                "direct door activation requests motion without deleting geometry") &&
         expect(closed.accepted &&
                    closed.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::DoorClosing &&
                    closedMeshCount == sandbox.room.staticMeshes.size() &&
                    closedSurfaceCount == sandbox.room.spatialSurfaces.size() &&
                    closedColliderCount == sandbox.collisionSurfaces.size(),
                "closing request preserves exact geometry and collision cardinality") &&
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
                        cr::CreativeRuntimeInteractionEffectStatus::NoLinkedTarget,
                "ungrouped control reports no link instead of guessing") &&
         expect(document.revision() == authoredRevision &&
                    document.objectCount() == authoredObjectCount,
                "runtime interactable effects leave authored content untouched");
}

bool retractablePlatformPublishesAtomicallyAndRejectsOccupiedRestore() {
  cr::CreativeDocument document = playableDocument(false, 48U);
  const cr::CreativeDocumentCreateReceipt platform = createObject(
      document, cr::CreativeObjectKind::Platform, "Retractable Platform",
      {3.0, 1.0, 0.0}, std::nullopt,
      cr::CreativeBounds{{2.0, 1.0, -1.0}, {4.0, 1.35, 1.0}});
  const cr::CreativeDocumentCreateReceipt button = createObject(
      document, cr::CreativeObjectKind::Button, "Platform Button",
      {-2.0, 1.0, 0.0});
  const cr::CreativeDocumentCreateReceipt door = createObject(
      document, cr::CreativeObjectKind::Door, "Coupled Door",
      {0.0, 1.0, 4.0});
  const cr::CreativeDocumentCreateReceipt staticPlatform = createObject(
      document, cr::CreativeObjectKind::Platform, "Static Platform",
      {-3.0, 1.0, 3.0}, std::nullopt,
      cr::CreativeBounds{{-4.0, 1.0, 2.0}, {-2.0, 1.35, 4.0}});
  if (!platform.accepted || !button.accepted || !door.accepted ||
      !staticPlatform.accepted ||
      !document
           .setLogicLink({button.objectId, platform.objectId,
                          cr::CreativeLogicLinkAction::Toggle})
           .accepted ||
      !document
           .setLogicLink({button.objectId, door.objectId,
                          cr::CreativeLogicLinkAction::Toggle})
           .accepted) {
    return expect(false, "retractable platform setup creates authored circuit");
  }
  const std::uint64_t authoredRevision = document.revision();

  cr::CreativePlayPreparationResult prepared = prepare(document);
  if (!prepared.payload.has_value()) {
    return expect(false, "retractable platform prepares play payload");
  }
  cr::CreativeRuntimeSandboxActivationRequest request;
  request.sourceDocument = &document;
  request.payload = std::move(*prepared.payload);
  cr::CreativeRuntimeSandboxActivationResult activated =
      cr::activateCreativeRuntimeSandbox(std::move(request));
  if (!activated.sandbox.has_value()) {
    return expect(false, activated.receipt.reasonCode);
  }
  cr::CreativeRuntimeSandbox& sandbox = *activated.sandbox;
  cr::CreativeRuntimeInteractableState* platformState =
      findInteractable(sandbox, platform.objectId);
  cr::CreativeRuntimeInteractableState* buttonState =
      findInteractable(sandbox, button.objectId);
  cr::CreativeRuntimeInteractableState* doorState =
      findInteractable(sandbox, door.objectId);
  const cr::CreativeRuntimeInteractableState* staticPlatformState =
      findInteractable(sandbox, staticPlatform.objectId);
  if (platformState == nullptr || buttonState == nullptr ||
      doorState == nullptr || staticPlatformState != nullptr) {
    return expect(false, "retractable platform runtime states exist");
  }
  const iggy3d::EntityState* platformEntity =
      sandbox.session.state().world.findById(platformState->entity);
  if (platformEntity == nullptr) {
    return expect(false, "retractable platform runtime marker exists");
  }

  const std::size_t enabledMeshCount = sandbox.room.staticMeshes.size();
  const std::size_t enabledSurfaceCount = sandbox.room.spatialSurfaces.size();
  const std::size_t enabledColliderCount = sandbox.collisionSurfaces.size();
  const iggy3d::ReasoningGraphSummary enabledReasoning =
      sandbox.reasoningGraph;
  const std::vector<std::string> enabledMeshOrder =
      sandbox.roomStaticMeshOrder;
  const std::vector<std::string> enabledSurfaceOrder =
      sandbox.roomSpatialSurfaceOrder;
  const std::string staticPlatformMeshId =
      "creative_object_" + std::to_string(staticPlatform.objectId);
  const bool staticPlatformRemainsPhysical =
      std::any_of(sandbox.room.staticMeshes.begin(),
                  sandbox.room.staticMeshes.end(),
                  [&staticPlatformMeshId](
                      const iggy3d::RoomStaticMeshAsset& mesh) {
                    return mesh.id == staticPlatformMeshId;
                  }) &&
      std::any_of(
          sandbox.room.spatialSurfaces.begin(),
          sandbox.room.spatialSurfaces.end(),
          [&staticPlatformMeshId](const iggy3d::RoomSpatialSurface& surface) {
            return surface.sourceStaticMeshId == staticPlatformMeshId;
          });

  const cr::CreativeRuntimeInteractionEffectReceipt disabled =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, buttonState->entity);
  const bool disabledState = !platformState->targetActive;
  const bool coupledDoorOpened = doorState->targetActive;
  const bool platformMeshRemoved = std::none_of(
      sandbox.room.staticMeshes.begin(), sandbox.room.staticMeshes.end(),
      [&platformState](const iggy3d::RoomStaticMeshAsset& mesh) {
        return mesh.id == platformState->definition.roomMeshId;
      });
  if (!moveEntity(sandbox, {1U}, {3.0F, 1.1F, 0.0F})) {
    return expect(false, "player moves into retracted platform volume");
  }
  const std::size_t retractedMeshCount = sandbox.room.staticMeshes.size();
  const std::size_t retractedSurfaceCount = sandbox.room.spatialSurfaces.size();
  const std::size_t retractedColliderCount = sandbox.collisionSurfaces.size();
  const cr::CreativeRuntimeInteractionEffectReceipt occupied =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, buttonState->entity);
  const bool occupiedStateStayedDisabled = !platformState->targetActive;
  const bool occupiedDoorStayedOpen = doorState->targetActive;
  const std::uint64_t revisionAfterOccupiedReject = sandbox.geometryRevision;
  const std::size_t meshCountAfterOccupiedReject =
      sandbox.room.staticMeshes.size();
  const std::size_t surfaceCountAfterOccupiedReject =
      sandbox.room.spatialSurfaces.size();
  const std::size_t colliderCountAfterOccupiedReject =
      sandbox.collisionSurfaces.size();
  if (!moveEntity(sandbox, {1U}, {-4.0F, 0.25F, 0.0F})) {
    return expect(false, "player leaves retracted platform volume");
  }
  const cr::CreativeRuntimeInteractionEffectReceipt enabled =
      cr::applyCreativeRuntimeInteractionEffect(sandbox, buttonState->entity);

  const bool meshOrderRestored =
      sandbox.room.staticMeshes.size() == enabledMeshOrder.size() &&
      std::equal(sandbox.room.staticMeshes.begin(),
                 sandbox.room.staticMeshes.end(), enabledMeshOrder.begin(),
                 [](const iggy3d::RoomStaticMeshAsset& mesh,
                    const std::string& id) { return mesh.id == id; });
  const bool surfaceOrderRestored =
      sandbox.room.spatialSurfaces.size() == enabledSurfaceOrder.size() &&
      std::equal(sandbox.room.spatialSurfaces.begin(),
                 sandbox.room.spatialSurfaces.end(),
                 enabledSurfaceOrder.begin(),
                 [](const iggy3d::RoomSpatialSurface& surface,
                    const std::string& id) { return surface.id == id; });

  return expect(activated.receipt.scenario.platformEntityCount == 1U &&
                    platformEntity->kind == iggy3d::EntityKind::Marker &&
                    !platformEntity->active &&
                    !platformEntity->targeting.targetable &&
                    platformState->targetMeshes.size() == 1U &&
                    platformState->targetSurfaces.size() == 1U &&
                    staticPlatformRemainsPhysical,
                "only linked platform seeds while static platforms stay physical") &&
         expect(disabled.accepted && disabled.changed &&
                    disabled.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::LinksApplied &&
                    disabled.affectedTargetCount == 2U &&
                    disabled.affectedDoorCount == 1U &&
                    disabled.affectedPlatformCount == 1U &&
                    disabled.geometryRevision == 1U && platformMeshRemoved &&
                    disabledState && coupledDoorOpened &&
                    retractedMeshCount + platformState->targetMeshes.size() ==
                        enabledMeshCount &&
                    retractedSurfaceCount +
                            platformState->targetSurfaces.size() ==
                        enabledSurfaceCount &&
                    retractedColliderCount +
                            platformState->targetSurfaces.size() ==
                        enabledColliderCount,
                "one source transitions platform and door in one revision") &&
         expect(occupied.accepted && !occupied.changed &&
                    occupied.status ==
                        cr::CreativeRuntimeInteractionEffectStatus::
                            TargetOccupied &&
                    occupied.reasonCode ==
                        "creative_runtime_platform_enable_occupied" &&
                    cr::toString(occupied.status) == "target_occupied" &&
                    occupied.geometryRevision == 1U &&
                    revisionAfterOccupiedReject == 1U &&
                    occupiedStateStayedDisabled && occupiedDoorStayedOpen &&
                    meshCountAfterOccupiedReject == retractedMeshCount &&
                    surfaceCountAfterOccupiedReject == retractedSurfaceCount &&
                    colliderCountAfterOccupiedReject == retractedColliderCount,
                "occupied restore rejects without publishing partial state") &&
         expect(enabled.accepted && enabled.changed &&
                    enabled.affectedTargetCount == 2U &&
                    enabled.affectedDoorCount == 1U &&
                    enabled.affectedPlatformCount == 1U &&
                    enabled.geometryRevision == 2U &&
                    platformState->targetActive && !doorState->targetActive &&
                    meshOrderRestored &&
                    surfaceOrderRestored &&
                    sandbox.room.staticMeshes.size() == enabledMeshCount &&
                    sandbox.room.spatialSurfaces.size() == enabledSurfaceCount &&
                    sandbox.collisionSurfaces.size() == enabledColliderCount &&
                    sandbox.reasoningGraph.nodeCount ==
                        enabledReasoning.nodeCount &&
                    sandbox.reasoningGraph.edgeCount ==
                        enabledReasoning.edgeCount,
                "clear restore republishes exact geometry collision and reasoning") &&
         expect(document.revision() == authoredRevision,
                "runtime platform transitions leave authored content untouched");
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
                  holdSourceInitializesBeforeFirstRuntimeTick() &&
                  holdSourceRetriesOccupiedPlatformRestore() &&
                  authoredInteractablesOwnExplicitCircuitsAndDynamicGeometry() &&
                  retractablePlatformPublishesAtomicallyAndRejectsOccupiedRestore();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
