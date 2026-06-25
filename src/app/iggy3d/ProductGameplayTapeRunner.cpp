#include "app/iggy3d/ProductGameplayTapeRunner.hpp"

#include <string>
#include <string_view>
#include <utility>

#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {
namespace {

CommandRecord makeTapeCommand(const Session& session,
                              ProductGameplayTapeAction action,
                              const EntityState* target) {
  CommandRecord command;
  command.playerSlot = 0;
  command.actor = session.state().players.actorForSlot(0);
  command.source = CommandSource::Script;
  switch (action) {
    case ProductGameplayTapeAction::Move:
      command.kind = CommandKind::Move;
      if (target != nullptr) {
        command.payload.target.hasPoint = true;
        command.payload.target.point = target->transform.position;
      }
      break;
    case ProductGameplayTapeAction::Interact:
      command.kind = CommandKind::Interact;
      if (target != nullptr) {
        command.payload.target.hasEntity = true;
        command.payload.target.entity = target->id;
      }
      break;
    case ProductGameplayTapeAction::Wait:
      command.kind = CommandKind::Wait;
      break;
  }
  return command;
}

bool stableNameContains(std::string_view stableName, std::string_view token) {
  return stableName.find(token) != std::string_view::npos;
}

bool inventoryHasSemanticItem(const InventoryState& inventory,
                              PlayerSlotId playerSlot,
                              std::string_view token) {
  const PlayerInventory* player = findInventory(inventory, playerSlot);
  if (player == nullptr) {
    return false;
  }
  for (const InventoryStack& stack : player->stacks) {
    if (stableNameContains(stack.itemId, token) && stack.count > 0U) {
      return true;
    }
  }
  return false;
}

bool anyExitObjectiveComplete(const ObjectiveState& objectives) {
  for (const ObjectiveRecord& objective : objectives.objectives) {
    if (objective.status == ObjectiveStatus::Complete &&
        objective.objectiveId.rfind("exit_", 0) == 0) {
      return true;
    }
  }
  return false;
}

bool anySecretDoorOpened(const WorldState& world) {
  for (const EntityState& entity : world.entities()) {
    if (entity.kind == EntityKind::Door &&
        stableNameContains(entity.stableName, "secret_door") && !entity.active) {
      return true;
    }
  }
  return false;
}

void fillLoopFacts(const Session& session, ProductGameplayTapeRunResult& result) {
  result.keyCollected =
      inventoryHasSemanticItem(session.state().inventory, 0, "key");
  result.secretDoorOpened = anySecretDoorOpened(session.state().world);
  result.treasureCollected =
      inventoryHasSemanticItem(session.state().inventory, 0, "treasure");
  result.exitObjectiveComplete = anyExitObjectiveComplete(session.state().objectives);
  result.sessionOutcome =
      std::string(productGameplayTapeSessionOutcomeName(session.state().outcome));
  result.loopComplete = result.keyCollected && result.secretDoorOpened &&
                        result.treasureCollected && result.exitObjectiveComplete &&
                        session.state().outcome == SessionOutcome::Victory;
  result.runtimeStateHash = session.stateHash();
}

ProductGameplayTapeRunResult fail(ProductGameplayTapeRunResult result,
                                  const Session* session,
                                  std::string status,
                                  std::uint64_t stepIndex,
                                  const ProductGameplayTapeStep* step,
                                  CommandRejectionReason rejection =
                                      CommandRejectionReason::None) {
  result.ok = false;
  result.status = std::move(status);
  result.reasonCode = result.status;
  result.failedStepIndex = stepIndex;
  result.failedSourceLine = step == nullptr ? 0U : step->sourceLine;
  result.failedAction =
      step == nullptr ? "none"
                      : std::string(productGameplayTapeActionName(step->action));
  result.failedTarget =
      step == nullptr || step->targetStableName.empty() ? "none"
                                                       : step->targetStableName;
  result.failedRejection =
      std::string(productGameplayTapeRejectionName(rejection));
  if (session != nullptr) {
    fillLoopFacts(*session, result);
  }
  return result;
}

}  // namespace

std::string_view productGameplayTapeSessionOutcomeName(SessionOutcome outcome) {
  switch (outcome) {
    case SessionOutcome::None:
      return "None";
    case SessionOutcome::DemoComplete:
      return "DemoComplete";
    case SessionOutcome::Victory:
      return "Victory";
    case SessionOutcome::Defeat:
      return "Defeat";
    case SessionOutcome::Failed:
      return "Failed";
  }
  return "None";
}

ProductGameplayTapeRunResult runProductGameplayTape(
    const ProductGameplayTapeRunRequest& request) {
  ProductGameplayTapeRunResult result;
  if (request.session == nullptr) {
    result.status = "gameplay_tape_session_missing";
    result.reasonCode = result.status;
    return result;
  }
  if (request.tape == nullptr) {
    return fail(std::move(result),
                request.session,
                "gameplay_tape_missing",
                0,
                nullptr);
  }
  result.stepCount = static_cast<std::uint64_t>(request.tape->steps.size());
  if (request.tape->steps.empty()) {
    return fail(std::move(result),
                request.session,
                "gameplay_tape_empty",
                0,
                nullptr);
  }

  for (std::size_t index = 0; index < request.tape->steps.size(); ++index) {
    const ProductGameplayTapeStep& step = request.tape->steps[index];
    const std::uint64_t stepIndex = static_cast<std::uint64_t>(index + 1U);
    result.lastAction = std::string(productGameplayTapeActionName(step.action));
    result.lastTarget = step.targetStableName.empty() ? "none" : step.targetStableName;

    const EntityState* target = nullptr;
    if (step.action != ProductGameplayTapeAction::Wait) {
      target = request.session->state().world.findByStableName(step.targetStableName);
      if (target == nullptr) {
        return fail(std::move(result),
                    request.session,
                    "gameplay_tape_target_missing",
                    stepIndex,
                    &step);
      }
    }

    const CommandRecord command = makeTapeCommand(*request.session, step.action, target);
    const SessionCommandResult submitted = request.session->submitCommand(command);
    if (step.expectRejection) {
      if (submitted.command.admission == CommandAdmissionStatus::Rejected &&
          submitted.command.rejection == step.expectedRejection) {
        ++result.expectedRejectedStepCount;
        continue;
      }
      return fail(std::move(result),
                  request.session,
                  "gameplay_tape_expected_rejection_mismatch",
                  stepIndex,
                  &step,
                  submitted.command.rejection);
    }

    if (submitted.command.admission != CommandAdmissionStatus::Accepted) {
      return fail(std::move(result),
                  request.session,
                  "gameplay_tape_command_rejected",
                  stepIndex,
                  &step,
                  submitted.command.rejection);
    }

    const StatusResult tick = request.session->tick(request.collisionSurfaces);
    if (tick.status != ResultStatus::Ok) {
      return fail(std::move(result),
                  request.session,
                  tick.error.code.empty() ? "gameplay_tape_tick_failed"
                                          : tick.error.code,
                  stepIndex,
                  &step);
    }
    ++result.executedStepCount;
  }

  fillLoopFacts(*request.session, result);
  result.ok = true;
  result.status = "gameplay_tape_completed";
  result.reasonCode = "gameplay_tape_completed";
  result.failedStepIndex = 0;
  result.failedSourceLine = 0;
  result.failedAction = "none";
  result.failedTarget = "none";
  result.failedRejection = "none";
  return result;
}

}  // namespace iggy3d
