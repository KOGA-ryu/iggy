#include "app/iggy3d/creative/play/RuntimeSandbox.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

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
  const bool ok = pureSeedMapsPlayerAndActorPolicies() &&
                  activationOwnsCollisionSessionAndReasoning() &&
                  activationRejectsStaleAndMalformedPayloads() &&
                  sandboxFreshnessAndStopAreExplicit() &&
                  runningSnapshotDoesNotTrackLaterDocumentEdits() &&
                  authoredInteractablesOwnExplicitCircuitsAndDynamicGeometry();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
