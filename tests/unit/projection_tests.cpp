#include "content/PackageLoader.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "runtime/ai/NpcBehaviorDebugSnapshot.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionRunner.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::SessionCreateRequest createRequestFromPackage() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{"fixtures/demos/first_room/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  return create;
}

iggy3d::Session makeSession() {
  return iggy3d::Session::create(createRequestFromPackage()).value;
}

iggy3d::CommandRecord submittedInteract() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {2};
  return command;
}

iggy3d::CommandRecord submittedMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord submittedRetry(iggy3d::CommandId sourceCommandId) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Retry;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.retrySourceCommandId = sourceCommandId;
  return command;
}

iggy3d::CommandRecord submittedControl(iggy3d::CommandKind kind) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.kind = kind;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

iggy3d::CommandRecord submittedWait() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Wait;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

bool runQueuedCommand(iggy3d::Session& session) {
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 8, true, true});
  return run.status == iggy3d::SessionRunnerStatus::Advanced && run.ticksAdvanced == 1U;
}

iggy3d::Session makePickedUpSession() {
  iggy3d::Session session = makeSession();
  (void)session.submitCommand(submittedInteract());
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedRetry(1));
  (void)runQueuedCommand(session);
  return session;
}

iggy3d::Session makeCompletedSession() {
  iggy3d::Session session = makePickedUpSession();
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::StepTacticalTick));
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::Resume));
  (void)session.submitCommand(submittedWait());
  (void)runQueuedCommand(session);
  (void)session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  (void)session.finalizeDemoIfComplete();
  return session;
}

const iggy3d::SceneItem* findSceneItem(const iggy3d::SceneProjectionResult& projection,
                                       std::string_view stableName) {
  for (const iggy3d::SceneItem& item : projection.items) {
    if (item.stableName == stableName) {
      return &item;
    }
  }
  return nullptr;
}

bool hasDebugKind(const iggy3d::DebugProjectionResult& projection,
                  iggy3d::DebugProjectionKind kind) {
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == kind) {
      return true;
    }
  }
  return false;
}

bool hasOutOfRangeRejection(const iggy3d::DebugProjectionResult& projection) {
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == iggy3d::DebugProjectionKind::CommandRejected &&
        item.commandId == 1U && item.sequence == 1U &&
        item.rejection == iggy3d::CommandRejectionReason::OutOfRange &&
        item.actor == iggy3d::EntityId{1} && item.target == iggy3d::EntityId{2}) {
      return true;
    }
  }
  return false;
}

const iggy3d::DebugProjectionItem* findNpcProjectionItem(
    const iggy3d::DebugProjectionResult& projection,
    iggy3d::EntityId actor) {
  for (const iggy3d::DebugProjectionItem& item : projection.items) {
    if (item.kind == iggy3d::DebugProjectionKind::NpcBehavior &&
        item.actor == actor) {
      return &item;
    }
  }
  return nullptr;
}

iggy3d::RuntimeDebugSnapshot okRuntimeDebugSnapshot() {
  iggy3d::RuntimeDebugSnapshot snapshot;
  snapshot.status = iggy3d::RuntimeDebugSnapshotStatus::Ok;
  snapshot.sourceTick = 3;
  snapshot.actor = {1};
  snapshot.playerPositionAvailable = true;
  snapshot.position = {1.0F, 0.0F, 2.0F};
  snapshot.grounded = true;
  snapshot.groundContact = true;
  snapshot.groundWalkable = true;
  snapshot.movementPolicyBand = "walkable";
  snapshot.slopeTravelDirection = "flat";
  snapshot.groundNormal = iggy3d::vec3UnitY();
  return snapshot;
}

iggy3d::NpcBehaviorDebugActorRow npcRow(iggy3d::EntityId actor,
                                        std::string_view stableName,
                                        std::string_view profileId,
                                        bool profileResolved,
                                        std::string_view profileStatus,
                                        iggy3d::NpcEngagementPolicy policy,
                                        iggy3d::AiBehaviorKind behavior,
                                        iggy3d::AiIntentKind intent) {
  iggy3d::NpcBehaviorDebugActorRow row;
  row.actor = actor;
  row.stableName = std::string(stableName);
  row.active = true;
  row.isNpc = true;
  row.hasAiState = true;
  row.hasCombatant = true;
  row.behaviorProfileId = std::string(profileId);
  row.profileResolved = profileResolved;
  row.profileStatus = std::string(profileStatus);
  row.engagementPolicy = policy;
  row.behavior = behavior;
  row.lastIntent = intent;
  return row;
}

iggy3d::NpcBehaviorDebugSnapshot npcDebugSnapshot() {
  iggy3d::NpcBehaviorDebugSnapshot snapshot;
  snapshot.status = iggy3d::NpcBehaviorDebugSnapshotStatus::Ok;
  snapshot.reasonCode = "npc_behavior_debug_ok";
  snapshot.sourceTick = 11;
  snapshot.npcWorldCount = 3;
  snapshot.aiActorCount = 3;
  snapshot.resolvedProfileCount = 2;
  snapshot.failedProfileCount = 1;
  snapshot.hostileCount = 1;
  snapshot.passiveCount = 1;
  snapshot.attackingCount = 1;
  snapshot.waitingCount = 1;

  iggy3d::NpcBehaviorDebugActorRow hostile =
      npcRow({2},
             "training_dummy",
             "default",
             true,
             "profile_resolved",
             iggy3d::NpcEngagementPolicy::Hostile,
             iggy3d::AiBehaviorKind::Attacking,
             iggy3d::AiIntentKind::AttackTarget);
  hostile.target = {1};
  hostile.targetStableName = "player";
  hostile.targetResolved = true;
  hostile.targetActive = true;
  hostile.targetDistanceMeters = 1.25F;
  hostile.cooldownTicksRemaining = 2;
  snapshot.actors.push_back(hostile);

  iggy3d::NpcBehaviorDebugActorRow passive =
      npcRow({3},
             "observer",
             "passive",
             true,
             "profile_resolved",
             iggy3d::NpcEngagementPolicy::Passive,
             iggy3d::AiBehaviorKind::Alert,
             iggy3d::AiIntentKind::Wait);
  passive.target = {1};
  passive.targetStableName = "player";
  passive.targetResolved = true;
  passive.targetActive = true;
  passive.targetDistanceMeters = 2.5F;
  snapshot.actors.push_back(passive);

  iggy3d::NpcBehaviorDebugActorRow ghost =
      npcRow({4},
             "ghost",
             "ghost_profile",
             false,
             "profile_missing",
             iggy3d::NpcEngagementPolicy::Hostile,
             iggy3d::AiBehaviorKind::Idle,
             iggy3d::AiIntentKind::None);
  snapshot.actors.push_back(ghost);
  return snapshot;
}

bool firstRoomProjectionContainsInitialItems() {
  const iggy3d::Session session = makeSession();
  const iggy3d::SceneProjectionResult projection = iggy3d::buildSceneProjection(session.state());
  const iggy3d::SceneItem* player = findSceneItem(projection, "player");
  const iggy3d::SceneItem* key = findSceneItem(projection, "gold_key");
  const iggy3d::SceneItem* marker = findSceneItem(projection, "tactical_marker_alpha");
  const iggy3d::SceneItem* dummy = findSceneItem(projection, "training_dummy");

  return expect(projection.items.size() == 4U, "initial projection item count") &&
         expect(player != nullptr && player->kind == iggy3d::SceneItemKind::Player,
                "player projected") &&
         expect(player != nullptr && player->owningPlayerSlot == 0U, "player owning slot") &&
         expect(player != nullptr && player->modelRef == "bean_player",
                "player bean model ref") &&
         expect(dummy != nullptr && dummy->kind == iggy3d::SceneItemKind::Npc,
                "npc projected") &&
         expect(dummy != nullptr && dummy->modelRef == "bean_npc", "npc bean model ref") &&
         expect(key != nullptr && key->kind == iggy3d::SceneItemKind::Pickup, "key projected") &&
         expect(key != nullptr && key->active && key->visible && key->interactable,
                "key active interactable") &&
         expect(key != nullptr && key->itemId == "gold_key", "key item id") &&
         expect(marker != nullptr && marker->kind == iggy3d::SceneItemKind::TacticalMarker,
                "marker projected") &&
         expect(projection.sourceStateHash == session.state().currentStateHash,
                "projection state hash copied") &&
         expect(projection.sourceTick == session.state().clock.tickIndex, "projection tick copied") &&
         expect(projection.cameraMode == iggy3d::CameraMode::ThirdPerson, "projection camera");
}

bool inactivePickupFilteringWorks() {
  const iggy3d::Session session = makePickedUpSession();
  const iggy3d::SceneProjectionResult activeOnly = iggy3d::buildSceneProjection(session.state());
  iggy3d::SceneProjectionConfig includeInactive;
  includeInactive.includeInactive = true;
  const iggy3d::SceneProjectionResult allItems =
      iggy3d::buildSceneProjection(session.state(), includeInactive);
  const iggy3d::SceneItem* hiddenKey = findSceneItem(activeOnly, "gold_key");
  const iggy3d::SceneItem* inactiveKey = findSceneItem(allItems, "gold_key");

  return expect(hiddenKey == nullptr, "inactive key hidden") &&
         expect(inactiveKey != nullptr, "inactive key included") &&
         expect(inactiveKey != nullptr && !inactiveKey->active && !inactiveKey->visible,
                "inactive key marked inactive") &&
         expect(inactiveKey != nullptr && inactiveKey->itemId == "gold_key",
                "inactive key retains item fact");
}

bool projectionDoesNotMutateRuntimeTruth() {
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::StateHashValue hashBefore = session.state().currentStateHash;
  const std::size_t logSizeBefore = session.state().commandLog.size();
  const iggy3d::CommandSequence nextSequenceBefore = session.state().commandLog.nextSequence();
  const iggy3d::CommandTick tickBefore = session.state().clock.tickIndex;

  iggy3d::SceneProjectionConfig sceneConfig;
  sceneConfig.includeInactive = true;
  const iggy3d::SceneProjectionResult scene =
      iggy3d::buildSceneProjection(session.state(), sceneConfig);
  const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());

  return expect(!scene.items.empty(), "scene projected") &&
         expect(!debug.items.empty(), "debug projected") &&
         expect(session.state().currentStateHash == hashBefore, "projection hash unchanged") &&
         expect(session.state().commandLog.size() == logSizeBefore, "projection log size unchanged") &&
         expect(session.state().commandLog.nextSequence() == nextSequenceBefore,
                "projection log cursor unchanged") &&
         expect(session.state().clock.tickIndex == tickBefore, "projection tick unchanged");
}

bool debugProjectionIncludesProofFacts() {
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());

  bool hasReach = false;
  bool hasObjective = false;
  for (const iggy3d::DebugProjectionItem& item : debug.items) {
    if (item.kind == iggy3d::DebugProjectionKind::ReachRadius &&
        item.actor == iggy3d::EntityId{1} && item.radiusMeters == 1.500F) {
      hasReach = true;
    }
    if (item.kind == iggy3d::DebugProjectionKind::ObjectiveState &&
        item.objectiveId == "collect_gold_key") {
      hasObjective = true;
    }
  }

  return expect(hasOutOfRangeRejection(debug), "debug out of range rejection") &&
         expect(hasReach, "debug reach radius") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::TargetCandidate),
                "debug target candidate") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::ClockMode), "debug clock") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::CameraMode), "debug camera") &&
         expect(hasObjective, "debug objective") &&
         expect(hasDebugKind(debug, iggy3d::DebugProjectionKind::StateHash), "debug hash") &&
         expect(debug.sourceStateHash == session.state().currentStateHash, "debug source hash");
}

bool npcDebugProjectionAppendsItemsAndHudLines() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::appendNpcBehaviorDebugSnapshot(debug, npcDebugSnapshot());

  const iggy3d::DebugProjectionItem* hostile = findNpcProjectionItem(debug, {2});
  const iggy3d::DebugProjectionItem* passive = findNpcProjectionItem(debug, {3});
  const iggy3d::DebugProjectionItem* ghost = findNpcProjectionItem(debug, {4});

  return expect(debug.items.size() == 3U, "npc projection item count") &&
         expect(hostile != nullptr, "hostile item projected") &&
         expect(hostile != nullptr && hostile->sourceTick == 11, "hostile source tick") &&
         expect(hostile != nullptr && hostile->actor == iggy3d::EntityId{2},
                "hostile actor") &&
         expect(hostile != nullptr && hostile->target == iggy3d::EntityId{1},
                "hostile target") &&
         expect(hostile != nullptr && hostile->hasScalar, "hostile scalar") &&
         expect(hostile != nullptr && hostile->scalarValue == 1.25F,
                "hostile distance scalar") &&
         expect(hostile != nullptr && hostile->labelCode == "npc.behavior",
                "hostile label") &&
         expect(hostile != nullptr &&
                    hostile->valueCode == "default:attacking:attack_target:profile_resolved",
                "hostile value") &&
         expect(passive != nullptr &&
                    passive->valueCode == "passive:alert:wait:profile_resolved",
                "passive value") &&
         expect(ghost != nullptr && !ghost->hasScalar, "ghost no scalar") &&
         expect(ghost != nullptr &&
                    ghost->valueCode == "ghost_profile:idle:none:profile_missing",
                "ghost value") &&
         expect(debug.npcBehaviorDebugHudLines.size() == 4U, "npc hud line count") &&
         expect(debug.npcBehaviorDebugHudLines[0] ==
                    "NPCS world=3 ai=3 resolved=2 failed=1 hostile=1 passive=1",
                "npc hud summary") &&
         expect(debug.npcBehaviorDebugHudLines[1] ==
                    "NPC 2 training_dummy default attacking/attack_target tgt=player cd=2",
                "hostile hud row") &&
         expect(debug.npcBehaviorDebugHudLines[2] ==
                    "NPC 3 observer passive alert/wait tgt=player cd=0",
                "passive hud row") &&
         expect(debug.npcBehaviorDebugHudLines[3] ==
                    "NPC 4 ghost ghost_profile idle/none tgt=none cd=0 "
                    "unresolved=profile_missing",
                "ghost hud row");
}

bool npcDebugProjectionIgnoresDisabledSnapshots() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::NpcBehaviorDebugSnapshot snapshot;
  snapshot.status = iggy3d::NpcBehaviorDebugSnapshotStatus::Disabled;
  snapshot.reasonCode = "npc_behavior_debug_disabled";
  iggy3d::appendNpcBehaviorDebugSnapshot(debug, snapshot);

  return expect(debug.items.empty(), "disabled npc projection items empty") &&
         expect(debug.npcBehaviorDebugHudLines.empty(),
                "disabled npc projection hud empty");
}

bool npcDebugProjectionPreservesRuntimeHudLines() {
  iggy3d::DebugProjectionResult debug;
  iggy3d::appendRuntimeDebugSnapshot(debug, okRuntimeDebugSnapshot());
  const std::vector<std::string> runtimeLinesBefore = debug.runtimeDebugHudLines;

  iggy3d::appendNpcBehaviorDebugSnapshot(debug, npcDebugSnapshot());

  return expect(!runtimeLinesBefore.empty(), "runtime hud lines present") &&
         expect(debug.runtimeDebugHudLines == runtimeLinesBefore,
                "runtime hud lines unchanged") &&
         expect(!debug.npcBehaviorDebugHudLines.empty(), "npc hud lines present");
}

bool saveLoadProjectionIsEquivalent() {
  const iggy3d::SessionCreateRequest create = createRequestFromPackage();
  const iggy3d::Session session = makeCompletedSession();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  iggy3d::Session loaded = iggy3d::Session::create(create).value;
  const iggy3d::SaveCompatibilityRequest compatibility{
      saved.envelope, session.state().identity.packageId, session.state().identity.scenarioId};
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibility);

  iggy3d::SceneProjectionConfig config;
  config.includeInactive = true;
  const iggy3d::SceneProjectionResult original =
      iggy3d::buildSceneProjection(session.state(), config);
  const iggy3d::SceneProjectionResult restored =
      iggy3d::buildSceneProjection(loaded.state(), config);
  const iggy3d::SceneItem* originalPlayer = findSceneItem(original, "player");
  const iggy3d::SceneItem* restoredPlayer = findSceneItem(restored, "player");
  const iggy3d::SceneItem* originalKey = findSceneItem(original, "gold_key");
  const iggy3d::SceneItem* restoredKey = findSceneItem(restored, "gold_key");

  return expect(saved.status == iggy3d::SaveLoadStatus::Ok, "save ok") &&
         expect(load.status == iggy3d::SaveLoadStatus::Ok, "load ok") &&
         expect(original.items.size() == restored.items.size(), "projection item count roundtrip") &&
         expect(original.sourceStateHash == restored.sourceStateHash, "projection hash roundtrip") &&
         expect(original.sourceTick == restored.sourceTick, "projection tick roundtrip") &&
         expect(originalPlayer != nullptr && restoredPlayer != nullptr, "player roundtrip exists") &&
         expect(originalPlayer != nullptr && restoredPlayer != nullptr &&
                    iggy3d::nearlyEqual(originalPlayer->transform.position,
                                        restoredPlayer->transform.position),
                "player position roundtrip") &&
         expect(originalKey != nullptr && restoredKey != nullptr, "key roundtrip exists") &&
         expect(originalKey != nullptr && restoredKey != nullptr &&
                    originalKey->active == restoredKey->active &&
                    originalKey->itemId == restoredKey->itemId,
                "key inactive fact roundtrip");
}

}  // namespace

int main() {
  bool ok = true;
  ok = firstRoomProjectionContainsInitialItems() && ok;
  ok = inactivePickupFilteringWorks() && ok;
  ok = projectionDoesNotMutateRuntimeTruth() && ok;
  ok = debugProjectionIncludesProofFacts() && ok;
  ok = npcDebugProjectionAppendsItemsAndHudLines() && ok;
  ok = npcDebugProjectionIgnoresDisabledSnapshots() && ok;
  ok = npcDebugProjectionPreservesRuntimeHudLines() && ok;
  ok = saveLoadProjectionIsEquivalent() && ok;
  return ok ? 0 : 1;
}
