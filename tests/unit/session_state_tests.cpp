#include "config/RuntimeConfig.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionState.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::ScenarioEntitySeed playerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "player";
  seed.kind = iggy3d::EntityKind::Player;
  seed.transform = transformAt(0.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = false;
  return seed;
}

iggy3d::ScenarioEntitySeed goldKeySeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "gold_key";
  seed.kind = iggy3d::EntityKind::Pickup;
  seed.transform = transformAt(3.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  seed.interaction.kind = iggy3d::InteractionKind::Pickup;
  seed.interaction.primaryEffect = iggy3d::InteractionEffectKind::AddItemToInventory;
  seed.interaction.itemId = "gold_key";
  seed.interaction.itemCount = 1;
  seed.interaction.objectiveId = "collect_gold_key";
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

iggy3d::ScenarioEntitySeed markerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "tactical_marker_alpha";
  seed.kind = iggy3d::EntityKind::Marker;
  seed.transform = transformAt(2.0F, 0.0F, 1.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Move, iggy3d::TargetAction::Inspect};
  return seed;
}

iggy3d::FixtureScenarioSeed makeFirstRoomSeed() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "first_room.runtime_loop";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::CameraMode::ThirdPerson;
  seed.defaultTacticalCamera = iggy3d::CameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::PlayerSlotKind::Local, "player"});
  seed.entities = {playerSeed(), goldKeySeed(), markerSeed()};
  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = "collect_gold_key";
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "InventoryContains";
  objective.playerSlot = 0;
  objective.itemId = "gold_key";
  objective.itemCount = 1;
  objective.completeStatus = iggy3d::ObjectiveStatusSeed::Complete;
  seed.objectives.push_back(objective);
  return seed;
}

iggy3d::SessionCreateRequest makeCreateRequest() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeFirstRoomSeed();
  return request;
}

iggy3d::Session makeSession() {
  iggy3d::Result<iggy3d::Session> result = iggy3d::Session::create(makeCreateRequest());
  return result.value;
}

iggy3d::CommandRecord acceptedWait(iggy3d::CommandId commandId) {
  iggy3d::CommandRecord command;
  command.commandId = commandId;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Wait;
  command.source = iggy3d::CommandSource::Script;
  command.admission = iggy3d::CommandAdmissionStatus::Accepted;
  return command;
}

iggy3d::CommandRecord acceptedAiWait(iggy3d::CommandId commandId) {
  iggy3d::CommandRecord command;
  command.commandId = commandId;
  command.actor = {2};
  command.kind = iggy3d::CommandKind::Wait;
  command.source = iggy3d::CommandSource::Ai;
  command.admission = iggy3d::CommandAdmissionStatus::Accepted;
  return command;
}

iggy3d::CommandRecord acceptedAiMove(iggy3d::CommandId commandId) {
  iggy3d::CommandRecord command = acceptedAiWait(commandId);
  command.kind = iggy3d::CommandKind::Move;
  command.payload.target.hasPoint = true;
  command.payload.target.point = {1.0F, 0.0F, 0.0F};
  return command;
}

iggy3d::CommandRecord acceptedAiAttack(iggy3d::CommandId commandId) {
  iggy3d::CommandRecord command = acceptedAiWait(commandId);
  command.kind = iggy3d::CommandKind::Attack;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {1};
  command.payload.attackDamage = 1;
  return command;
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

bool defaultState() {
  const iggy3d::SessionState state;
  const iggy3d::Session session;
  return expect(state.lifecycle == iggy3d::SessionLifecycle::Loading, "state loading") &&
         expect(state.outcome == iggy3d::SessionOutcome::None, "state outcome") &&
         expect(state.identity.packageId.empty() && state.identity.scenarioId.empty(), "empty identity") &&
         expect(state.world.empty(), "empty world") &&
         expect(state.players.empty(), "empty players") &&
         expect(state.commandLog.empty(), "empty command log") &&
         expect(session.lifecycle() == iggy3d::SessionLifecycle::Loading, "session loading");
}

bool createFirstRoomSession() {
  const iggy3d::Session session = makeSession();
  const iggy3d::SessionState& state = session.state();
  const iggy3d::EntityState* player = state.world.findByStableName("player");
  const iggy3d::EntityState* gold = state.world.findByStableName("gold_key");
  return expect(state.identity.packageId == "iggy3d.first_room", "identity package") &&
         expect(state.identity.scenarioId == "first_room.runtime_loop", "identity scenario") &&
         expect(state.lifecycle == iggy3d::SessionLifecycle::Playing, "lifecycle playing") &&
         expect(state.outcome == iggy3d::SessionOutcome::None, "outcome none") &&
         expect(state.world.size() == 3U, "world count") &&
         expect(state.world.entities()[0].id == iggy3d::EntityId{1} &&
                    state.world.entities()[1].id == iggy3d::EntityId{2} &&
                    state.world.entities()[2].id == iggy3d::EntityId{3},
                "entity ids") &&
         expect(player != nullptr && iggy3d::nearlyEqual(player->transform.position,
                                                         iggy3d::Vec3{0.0F, 0.0F, 0.0F}),
                "player position") &&
         expect(gold != nullptr && gold->active, "gold active") &&
         expect(state.players.slotControlsActor(0, iggy3d::EntityId{1}), "player slot") &&
         expect(state.clock.mode == iggy3d::ClockMode::Normal, "clock normal") &&
         expect(state.camera.activeMode == iggy3d::CameraMode::ThirdPerson, "camera third person") &&
         expect(state.inventory.players.size() == 1U && state.inventory.players[0].stacks.empty(),
                "inventory empty") &&
         expect(state.objectives.objectives.size() == 1U &&
                    state.objectives.objectives[0].objectiveId == "collect_gold_key" &&
                    state.objectives.objectives[0].status == iggy3d::ObjectiveStatus::Active,
                "objective active") &&
         expect(state.commandLog.empty() && state.nextCommandId == 1U, "command baseline") &&
         expect(state.baseline.world.size() == 3U && state.baseline.baselineHash != 0U,
                "baseline built");
}

bool creationFailures() {
  iggy3d::SessionCreateRequest request = makeCreateRequest();
  request.seed.entities.erase(request.seed.entities.begin());
  bool ok = expect(iggy3d::Session::create(request).status == iggy3d::ResultStatus::Error,
                   "missing player fails");
  request = makeCreateRequest();
  request.seed.players.clear();
  ok = ok && expect(iggy3d::Session::create(request).status == iggy3d::ResultStatus::Error,
                    "missing binding fails");
  request = makeCreateRequest();
  request.seed.scenarioId.clear();
  ok = ok && expect(iggy3d::Session::create(request).status == iggy3d::ResultStatus::Error,
                    "empty scenario fails");
  request = makeCreateRequest();
  request.packageId.clear();
  ok = ok && expect(iggy3d::Session::create(request).status == iggy3d::ResultStatus::Error,
                    "empty package fails");
  return ok;
}

bool ownershipShape() {
  const iggy3d::Session session = makeSession();
  const iggy3d::SessionState& state = session.state();
  return expect(state.transient.events.empty(), "transient events separate") &&
         expect(state.transient.pendingExecutionSequences.empty(), "transient queue separate") &&
         expect(state.baseline.world.size() == state.world.size(), "baseline owns world") &&
         expect(state.combat.combatants.empty() && state.ai.actors.empty(), "subsystem states exist");
}

bool resetBaseline() {
  iggy3d::Session session = makeSession();
  iggy3d::SessionState& state = session.mutableStateForOwnedSystems();
  (void)state.world.updateTransform({1}, transformAt(5.0F, 0.0F, 0.0F));
  (void)state.world.setActive({2}, false);
  state.inventory.players[0].stacks.push_back({"gold_key", 1});
  state.objectives.objectives[0].status = iggy3d::ObjectiveStatus::Complete;
  (void)state.commandLog.append(acceptedWait(1));
  state.nextCommandId = 2;
  iggy3d::RuntimeEvent acceptedEvent;
  acceptedEvent.kind = iggy3d::RuntimeEventKind::CommandAccepted;
  state.transient.events.push_back(acceptedEvent);
  state.transient.metrics.ticksRun = 10;
  state.transient.pendingExecutionSequences.push_back(1);
  iggy3d::AiActorState baselineAi;
  baselineAi.actor = {3};
  baselineAi.behaviorProfileId = "passive";
  session.mutableStateForOwnedSystems().baseline.ai.actors.push_back(baselineAi);
  state.ai.actors.push_back(baselineAi);
  state.ai.actors[0].behaviorProfileId = "melee_training";
  state.currentStateHash = iggy3d::computeStateHash(state);

  const iggy3d::SessionResetResult reset = session.resetToBaseline();
  const iggy3d::SessionState& resetState = session.state();
  return expect(reset.reset, "reset true") &&
         expect(reset.baselineHash == reset.currentHash, "reset hash equality") &&
         expect(reset.currentHash == resetState.currentStateHash, "reset current hash") &&
         expect(resetState.lifecycle == iggy3d::SessionLifecycle::Playing, "reset lifecycle") &&
         expect(resetState.outcome == iggy3d::SessionOutcome::None, "reset outcome") &&
         expect(iggy3d::nearlyEqual(resetState.world.findById({1})->transform.position,
                                    iggy3d::Vec3{0.0F, 0.0F, 0.0F}),
                "reset player position") &&
         expect(resetState.world.findById({2})->active, "reset gold active") &&
         expect(resetState.inventory.players[0].stacks.empty(), "reset inventory") &&
         expect(resetState.objectives.objectives[0].status == iggy3d::ObjectiveStatus::Active,
                "reset objective") &&
         expect(resetState.clock.mode == iggy3d::ClockMode::Normal, "reset clock") &&
         expect(resetState.camera.activeMode == iggy3d::CameraMode::ThirdPerson, "reset camera") &&
         expect(resetState.ai.actors.size() == 1U &&
                    resetState.ai.actors[0].behaviorProfileId == "passive",
                "reset ai profile baseline") &&
         expect(resetState.commandLog.empty() && resetState.commandLog.nextSequence() == 1U,
                "reset command log") &&
         expect(resetState.nextCommandId == 1U, "reset command id") &&
         expect(resetState.transient.events.empty() && resetState.transient.metrics.ticksRun == 0U &&
                    resetState.transient.pendingExecutionSequences.empty(),
                "reset transient");
}

bool submitCommandLogsAndQueuesWithoutExecuting() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult interact = session.submitCommand(submittedInteract());
  bool ok = expect(interact.appendedToLog, "interact appended") &&
            expect(interact.command.commandId == 1U, "interact id") &&
            expect(interact.command.rejection == iggy3d::CommandRejectionReason::OutOfRange,
                   "interact out of range") &&
            expect(session.state().commandLog.size() == 1U, "interact logged") &&
            expect(session.state().nextCommandId == 2U, "next id after reject") &&
            expect(session.state().transient.pendingExecutionSequences.empty(),
                   "rejected not queued");

  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  ok = ok && expect(move.appendedToLog, "move appended") &&
       expect(move.command.commandId == 2U && move.command.sequence == 2U, "move id sequence") &&
       expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
              "move accepted") &&
       expect(session.state().nextCommandId == 3U, "next id after move") &&
       expect(session.state().transient.pendingExecutionSequences.size() == 1U &&
                  session.state().transient.pendingExecutionSequences[0] == move.command.sequence,
              "move queued once") &&
       expect(iggy3d::nearlyEqual(session.state().world.findById({1})->transform.position,
                                  {0.0F, 0.0F, 0.0F}),
              "submit does not execute movement");

  iggy3d::SessionState& mutableState = session.mutableStateForOwnedSystems();
  (void)mutableState.world.updateTransform({1}, transformAt(2.0F, 0.0F, 0.0F));
  const iggy3d::SessionCommandResult retry = session.submitCommand(submittedRetry(1));
  return ok && expect(retry.appendedToLog, "retry appended") &&
         expect(retry.command.commandId == 3U && retry.command.sequence == 3U, "retry id sequence") &&
         expect(retry.command.kind == iggy3d::CommandKind::Retry, "retry remains logged retry") &&
         expect(retry.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                "retry accepted after movement") &&
         expect(session.state().transient.pendingExecutionSequences.size() == 2U &&
                    session.state().transient.pendingExecutionSequences[1] == retry.command.sequence,
                "retry queued once") &&
         expect(session.state().world.findById({2})->active, "submit does not execute interaction");
}

bool loadReplacement() {
  iggy3d::Session session = makeSession();
  const iggy3d::StateHashValue before = session.stateHash();
  iggy3d::SessionState loaded = session.state();
  (void)loaded.world.updateTransform({1}, transformAt(1.0F, 0.0F, 1.0F));
  iggy3d::RuntimeEvent movedEvent;
  movedEvent.kind = iggy3d::RuntimeEventKind::Moved;
  loaded.transient.events.push_back(movedEvent);
  loaded.currentStateHash = 123;
  const iggy3d::SessionLoadResult replaced = session.replaceStateFromLoad(loaded);
  bool ok = expect(replaced.status == iggy3d::SessionLoadStatus::Ok, "load ok") &&
            expect(replaced.previousHash == before, "load previous hash") &&
            expect(session.state().transient.events.empty(), "load clears transient") &&
            expect(iggy3d::nearlyEqual(session.state().world.findById({1})->transform.position,
                                       iggy3d::Vec3{1.0F, 0.0F, 1.0F}),
                   "load replaces world") &&
            expect(session.state().currentStateHash == iggy3d::computeStateHash(session.state()),
                   "load recomputes hash");

  const iggy3d::SessionState afterValidLoad = session.state();
  loaded = afterValidLoad;
  loaded.players.clear();
  const iggy3d::SessionLoadResult rejected = session.replaceStateFromLoad(loaded);
  ok = ok && expect(rejected.status == iggy3d::SessionLoadStatus::InvalidCandidateState,
                    "load rejects invalid") &&
       expect(iggy3d::nearlyEqual(session.state().world.findById({1})->transform.position,
                                  iggy3d::Vec3{1.0F, 0.0F, 1.0F}),
              "invalid load unchanged") &&
       expect(session.state().currentStateHash == afterValidLoad.currentStateHash,
              "invalid load hash unchanged");
  return ok;
}

bool stateHashExclusionsAndCommandLogPolicy() {
  iggy3d::SessionState state = makeSession().state();
  const iggy3d::StateHashValue base = iggy3d::computeStateHash(state);
  state.currentStateHash = base + 1U;
  bool ok = expect(iggy3d::computeStateHash(state) == base, "hash excludes self");
  iggy3d::RuntimeEvent resetEvent;
  resetEvent.kind = iggy3d::RuntimeEventKind::ResetCompleted;
  state.transient.events.push_back(resetEvent);
  state.transient.metrics.ticksRun = 99;
  ok = ok && expect(iggy3d::computeStateHash(state) == base, "hash excludes transient");
  (void)state.commandLog.append(acceptedWait(1));
  ok = ok && expect(iggy3d::computeStateHash(state) != base, "hash includes command log") &&
       expect(iggy3d::formatStateHash(base).size() == 16U, "hash format width");
  return ok;
}

bool commandLogAllowsAiInvalidPlayerSlotOnlyForOwnedCommands() {
  iggy3d::CommandLog log;
  const iggy3d::CommandLogAppendResult attack = log.append(acceptedAiAttack(1));
  bool ok = expect(attack.status == iggy3d::CommandLogAppendStatus::Ok,
                   "ai attack invalid slot append") &&
            expect(attack.record.playerSlot == iggy3d::kInvalidPlayerSlotId,
                   "ai attack preserves invalid slot");

  const iggy3d::CommandLogAppendResult move = log.append(acceptedAiMove(2));
  ok = ok && expect(move.status == iggy3d::CommandLogAppendStatus::Ok,
                    "ai move invalid slot append");

  const iggy3d::CommandLogAppendResult wait = log.append(acceptedAiWait(3));
  ok = ok && expect(wait.status == iggy3d::CommandLogAppendStatus::Ok,
                    "ai wait invalid slot append");

  iggy3d::CommandRecord localWait = acceptedWait(4);
  localWait.playerSlot = iggy3d::kInvalidPlayerSlotId;
  const iggy3d::CommandLogAppendResult local = log.append(localWait);
  ok = ok && expect(local.status == iggy3d::CommandLogAppendStatus::InvalidPlayerSlot,
                    "local invalid slot still rejected");

  iggy3d::CommandRecord restoredAi = acceptedAiAttack(5);
  restoredAi.sequence = 1;
  iggy3d::CommandLog restoredLog;
  const iggy3d::CommandLogRestoreResult restored =
      restoredLog.restoreForLoad({restoredAi}, 2, 0);
  ok = ok && expect(restored.status == iggy3d::CommandLogRestoreStatus::Restored,
                    "restore ai invalid slot");

  iggy3d::CommandRecord restoredLocal = acceptedWait(6);
  restoredLocal.playerSlot = iggy3d::kInvalidPlayerSlotId;
  restoredLocal.sequence = 1;
  iggy3d::CommandLog rejectedRestoreLog;
  const iggy3d::CommandLogRestoreResult rejected =
      rejectedRestoreLog.restoreForLoad({restoredLocal}, 2, 0);
  ok = ok && expect(rejected.status == iggy3d::CommandLogRestoreStatus::InvalidAdmissionState,
                    "restore local invalid slot rejected");
  return ok;
}

}  // namespace

int main() {
  const bool ok = defaultState() && createFirstRoomSession() && creationFailures() &&
                  ownershipShape() && resetBaseline() && submitCommandLogsAndQueuesWithoutExecuting() &&
                  loadReplacement() &&
                  stateHashExclusionsAndCommandLogPolicy() &&
                  commandLogAllowsAiInvalidPlayerSlotOnlyForOwnedCommands();
  return ok ? 0 : 1;
}
