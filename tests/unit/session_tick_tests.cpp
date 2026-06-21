#include "runtime/session/Session.hpp"

#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"

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

iggy3d::Session makeSession() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = makeFirstRoomSeed();
  return iggy3d::Session::create(request).value;
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

std::uint32_t goldKeyCount(const iggy3d::InventoryState& inventory) {
  const iggy3d::PlayerInventory* player = iggy3d::findInventory(inventory, 0);
  if (player == nullptr) {
    return 0;
  }
  for (const iggy3d::InventoryStack& stack : player->stacks) {
    if (stack.itemId == "gold_key") {
      return stack.count;
    }
  }
  return 0;
}

bool tickMovesThenRetryPicksUpKeyExactlyOnce() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult rejectedInteract = session.submitCommand(submittedInteract());
  bool ok = expect(rejectedInteract.command.commandId == 1U, "rejected interact id") &&
            expect(rejectedInteract.command.rejection == iggy3d::CommandRejectionReason::OutOfRange,
                   "rejected out of range") &&
            expect(session.state().transient.pendingExecutionSequences.empty(),
                   "rejected not queued");

  const iggy3d::SessionCommandResult move =
      session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  ok = ok && expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "move accepted") &&
       expect(session.state().transient.pendingExecutionSequences.size() == 1U, "move queued");

  ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "move tick ok") &&
       expect(session.state().transient.pendingExecutionSequences.empty(), "move queue consumed") &&
       expect(iggy3d::nearlyEqual(session.state().world.findById({1})->transform.position,
                                  {2.0F, 0.0F, 0.0F}),
              "player moved") &&
       expect(session.state().clock.tickIndex == 1U, "tick advanced after move");

  const iggy3d::SessionCommandResult retry = session.submitCommand(submittedRetry(1));
  ok = ok && expect(retry.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                    "retry accepted") &&
       expect(retry.command.kind == iggy3d::CommandKind::Retry, "retry logged as retry") &&
       expect(session.state().transient.pendingExecutionSequences.size() == 1U, "retry queued");

  ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "retry tick ok") &&
       expect(session.state().transient.pendingExecutionSequences.empty(), "retry queue consumed") &&
       expect(goldKeyCount(session.state().inventory) == 1U, "one key acquired") &&
       expect(!session.state().world.findById({2})->active, "key inactive") &&
       expect(iggy3d::objectiveComplete(session.state().objectives, "collect_gold_key"),
              "objective complete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::DemoComplete,
              "demo complete outcome") &&
       expect(session.state().lifecycle == iggy3d::SessionLifecycle::Playing,
              "lifecycle remains playing");

  ok = ok && expect(session.tick().status == iggy3d::ResultStatus::Ok, "idle tick ok") &&
       expect(goldKeyCount(session.state().inventory) == 1U, "idle no duplicate key") &&
       expect(session.state().transient.pendingExecutionSequences.empty(), "idle queue empty");
  return ok;
}

}  // namespace

int main() {
  const bool ok = tickMovesThenRetryPicksUpKeyExactlyOnce();
  return ok ? 0 : 1;
}
