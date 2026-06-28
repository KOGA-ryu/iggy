#include "app/iggy3d/gameplay/TapeRunner.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/movement/MovementSystem.hpp"
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
    case ProductGameplayTapeAction::Attack:
      command.kind = CommandKind::Attack;
      if (target != nullptr) {
        command.payload.target.hasEntity = true;
        command.payload.target.entity = target->id;
      }
      command.payload.attackDamage = 3;
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

bool anyNpcTargetable(const WorldState& world) {
  for (const EntityState& entity : world.entities()) {
    if (entity.kind == EntityKind::Npc && entity.active &&
        isTargetActionSupported(entity.targeting, TargetAction::Attack)) {
      return true;
    }
  }
  return false;
}

bool anyNpcDefeated(const SessionState& state) {
  for (const EntityState& entity : state.world.entities()) {
    if (entity.kind != EntityKind::Npc) {
      continue;
    }
    for (const CombatantState& combatant : state.combat.combatants) {
      if (combatant.entity == entity.id && combatant.defeated) {
        return true;
      }
    }
  }
  return false;
}

const CombatantState* findCombatant(const CombatState& combat, EntityId entity) {
  for (const CombatantState& combatant : combat.combatants) {
    if (combatant.entity == entity) {
      return &combatant;
    }
  }
  return nullptr;
}

std::string entityIdReceiptValue(EntityId entity) {
  return isValid(entity) ? std::to_string(toUint64(entity)) : "none";
}

std::string failedTapeStepReceiptValue(std::uint64_t stepIndex) {
  // branch-gate: BG-1032
  return stepIndex == 0U ? "none" : std::to_string(stepIndex);
}

std::int32_t playerHitPoints(const SessionState& state) {
  const EntityId player = state.players.actorForSlot(0);
  const CombatantState* combatant = findCombatant(state.combat, player);
  return combatant == nullptr ? 0 : combatant->hitPoints;
}

void fillAiTapeFacts(const SessionState& before,
                     const SessionState& after,
                     std::size_t commandLogRecordBoundary,
                     ProductGameplayTapeRunResult& result) {
  const EntityId player = after.players.actorForSlot(0);
  result.aiPlayerHpBefore = playerHitPoints(before);
  result.aiPlayerHpAfter = playerHitPoints(after);
  result.aiPlayerDamaged = result.aiPlayerHpAfter < result.aiPlayerHpBefore;

  CommandSequence latestAiSequence = kInvalidCommandSequence;
  const std::vector<CommandRecord>& records = after.commandLog.records();
  for (std::size_t index = commandLogRecordBoundary; index < records.size(); ++index) {
    const CommandRecord& record = records[index];
    if (record.source != CommandSource::Ai) {
      continue;
    }
    result.aiCommandLogged = true;
    if (record.kind == CommandKind::Attack) {
      result.aiAttackLogged = true;
    }
    if (record.kind == CommandKind::Wait) {
      result.aiWaitLogged = true;
    }
    if (record.sequence >= latestAiSequence) {
      latestAiSequence = record.sequence;
      result.aiActorId = entityIdReceiptValue(record.actor);
      if (record.payload.target.hasEntity) {
        result.aiTargetId = entityIdReceiptValue(record.payload.target.entity);
      } else if (record.kind == CommandKind::Attack && isValid(player)) {
        result.aiTargetId = entityIdReceiptValue(player);
      }
    }
  }

  for (const AiActorState& actor : after.ai.actors) {
    if (result.aiActorId != entityIdReceiptValue(actor.actor)) {
      continue;
    }
    result.aiBehavior = std::string(aiBehaviorKindName(actor.behavior));
    result.aiIntent = std::string(aiIntentKindName(actor.lastIntent));
    if (result.aiTargetId == "none") {
      result.aiTargetId = entityIdReceiptValue(actor.target);
    }
    break;
  }
}

void fillLoopFacts(const Session& session, ProductGameplayTapeRunResult& result) {
  result.keyCollected =
      inventoryHasSemanticItem(session.state().inventory, 0, "key");
  result.secretDoorOpened = anySecretDoorOpened(session.state().world);
  result.treasureCollected =
      inventoryHasSemanticItem(session.state().inventory, 0, "treasure");
  result.npcTargetable = anyNpcTargetable(session.state().world);
  result.npcDefeated = anyNpcDefeated(session.state());
  result.exitObjectiveComplete = anyExitObjectiveComplete(session.state().objectives);
  result.sessionOutcome =
      std::string(productGameplayTapeSessionOutcomeName(session.state().outcome));
  result.loopComplete = result.keyCollected && result.secretDoorOpened &&
                        result.treasureCollected && result.exitObjectiveComplete &&
                        session.state().outcome == SessionOutcome::Victory;
  result.runtimeStateHash = session.stateHash();
}

const SpatialSurfaceSet* currentCollisionSurfaces(
    const ProductGameplayTapeRunRequest& request) {
  if (request.activeRoomCollision != nullptr) {
    const SpatialSurfaceSet* activeSurfaces =
        productActiveRoomCollisionSurfaces(*request.activeRoomCollision);
    if (activeSurfaces != nullptr) {
      return activeSurfaces;
    }
  }
  return request.collisionSurfaces;
}

void refreshActiveRoomCollision(const ProductGameplayTapeRunRequest& request) {
  if (request.session == nullptr || request.activeRoom == nullptr ||
      request.activeRoomCollision == nullptr || !request.activeRoom->loaded) {
    return;
  }
  *request.activeRoomCollision =
      buildProductActiveRoomCollision(*request.activeRoom, request.session->state());
}

ProductGameplayTapeRunResult fail(ProductGameplayTapeRunResult result,
                                  const Session* session,
                                  std::string status,
                                  std::uint64_t stepIndex,
                                  const ProductGameplayTapeStep* step,
                                  CommandRejectionReason rejection =
                                      CommandRejectionReason::None,
                                  MovementBlockedReason movementBlock =
                                      MovementBlockedReason::None) {
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
  result.failedMovementBlock =
      std::string(productGameplayTapeMovementBlockName(movementBlock));
  if (session != nullptr) {
    fillLoopFacts(*session, result);
  }
  return result;
}

MovementBlockedReason lastMovementBlock(const Session& session) {
  if (!session.state().transient.lastMovementResultAvailable) {
    return MovementBlockedReason::None;
  }
  return session.state().transient.lastMovementResult.blocked;
}

bool lastMovementHasPhysicsStats(const Session& session) {
  return session.state().transient.lastMovementResultAvailable &&
         session.state().transient.lastMovementResult.physicsFrameStatsAvailable;
}

void recordProductGameplayTapeMovementDebug(
    const Session& session,
    const ProductGameplayTapeRunResult& run,
    ProductAppWindowState& window) {
  const SessionTransientState& transient = session.state().transient;
  // branch-gate: BG-1120
  if (!transient.lastMovementResultAvailable) {
    return;
  }

  const MovementResult& movement = transient.lastMovementResult;
  window.gameplayMovementDebugAvailable = true;
  // branch-gate: BG-1120
  window.gameplayMovementReasonCode =
      movement.reasonCode.empty() ? movementBlockedReasonName(movement.blocked)
                                  : movement.reasonCode;
  window.gameplayMovementBlockedReason =
      movementBlockedReasonName(movement.blocked);
  // branch-gate: BG-1120
  window.gameplayMovementHitSurfaceId =
      movement.hitSurfaceId.empty() ? "none" : movement.hitSurfaceId;
  window.gameplayMovementGroundSnapApplied = movement.groundSnapApplied;
  window.gameplayMovementClamped = movement.movementClamped;
  window.gameplayMovementSlid = movement.movementSlid;
  window.gameplayMovementCollisionSweepCount = movement.collisionSweepCount;
  // branch-gate: BG-1120
  window.gameplayMovementPolicyBand =
      movement.movementPolicyBand.empty() ? "none" : movement.movementPolicyBand;
  window.gameplayMovementSlopeTravelDirection = movement.slopeTravelDirection;
  window.gameplayMovementSlopeAngleDegrees = movement.slopeAngleDegrees;
  window.gameplayMovementSpeedMultiplier = movement.speedMultiplier;
  window.gameplayMovementStartX = movement.start.x;
  window.gameplayMovementStartY = movement.start.y;
  window.gameplayMovementStartZ = movement.start.z;
  window.gameplayMovementFinalX = movement.finalPosition.x;
  window.gameplayMovementFinalY = movement.finalPosition.y;
  window.gameplayMovementFinalZ = movement.finalPosition.z;
  window.gameplayMovementHorizontalDistanceMeters =
      movement.horizontalDistanceMeters;
  window.gameplayMovementVerticalDeltaMeters = movement.verticalDeltaMeters;
  window.gameplayMovementGradePercent = movement.gradePercent;

  // branch-gate: BG-1120
  if (run.lastAction == "move") {
    window.gameplayMovementAttempted = true;
    window.gameplayMovementBlocked =
        movement.blocked != MovementBlockedReason::None;
    // branch-gate: BG-1120
    window.gameplayMovementStatus =
        window.gameplayMovementBlocked ? "blocked" : "moved";
  }
}

StatusResult tickProductGameplayTape(Session& session,
                                     const SpatialSurfaceSet* collisionSurfaces,
                                     bool usePhysicsMovePlanner) {
  // branch-gate: BG-1032
  if (usePhysicsMovePlanner) {
    return session.tickWithOptions(
        SessionTickOptions{collisionSurfaces, true});
  }
  return session.tick(collisionSurfaces);
}

void recordProductGameplayTapeParse(const ProductGameplayTapeParseResult& parsed,
                                    ProductAppWindowState& window) {
  window.gameplayTapeLoaded = parsed.ok;
  window.gameplayTapeStatus = parsed.status;
  window.gameplayTapeReasonCode = parsed.reasonCode;
  window.gameplayTapeLineCount = parsed.lineCount;
  window.gameplayTapeStepCount =
      static_cast<std::uint64_t>(parsed.tape.steps.size());
  window.gameplayTapeFailedStep = failedTapeStepReceiptValue(parsed.failedLine);
  window.gameplayTapeFailedSourceLine = parsed.failedLine;
  window.gameplayTapeFailedAction = "none";
  window.gameplayTapeFailedTarget = parsed.failedToken;
  window.gameplayTapeFailedRejection = "none";
}

void recordProductGameplayTapeRun(const ProductGameplayTapeRunResult& run,
                                  ProductAppWindowState& window) {
  window.gameplayTapeStatus = run.status;
  window.gameplayTapeReasonCode = run.reasonCode;
  window.gameplayTapeStepCount = run.stepCount;
  window.gameplayTapeExecutedStepCount = run.executedStepCount;
  window.gameplayTapeExpectedRejectedStepCount = run.expectedRejectedStepCount;
  window.gameplayTapeExpectedBlockedStepCount = run.expectedBlockedStepCount;
  window.gameplayTapeFailedStep = failedTapeStepReceiptValue(run.failedStepIndex);
  window.gameplayTapeFailedSourceLine = run.failedSourceLine;
  window.gameplayTapeFailedAction = run.failedAction;
  window.gameplayTapeFailedTarget = run.failedTarget;
  window.gameplayTapeFailedRejection = run.failedRejection;
  window.gameplayTapeFailedMovementBlock = run.failedMovementBlock;
  window.gameplayTapeLastAction = run.lastAction;
  window.gameplayTapeLastTarget = run.lastTarget;
  window.gameplayTapeLastMovementBlock = run.lastMovementBlock;
  window.gameplayTapeKeyCollected = run.keyCollected;
  window.gameplayTapeSecretDoorOpened = run.secretDoorOpened;
  window.gameplayTapeTreasureCollected = run.treasureCollected;
  window.gameplayTapeNpcTargetable = run.npcTargetable;
  window.gameplayTapeNpcDefeated = run.npcDefeated;
  window.gameplayTapeExitObjectiveComplete = run.exitObjectiveComplete;
  window.gameplayTapeLoopComplete = run.loopComplete;
  window.gameplayTapeAiCommandLogged = run.aiCommandLogged;
  window.gameplayTapeAiAttackLogged = run.aiAttackLogged;
  window.gameplayTapeAiWaitLogged = run.aiWaitLogged;
  window.gameplayTapeAiPlayerDamaged = run.aiPlayerDamaged;
  window.gameplayTapeAiPlayerHpBefore = run.aiPlayerHpBefore;
  window.gameplayTapeAiPlayerHpAfter = run.aiPlayerHpAfter;
  window.gameplayTapeAiActorId = run.aiActorId;
  window.gameplayTapeAiTargetId = run.aiTargetId;
  window.gameplayTapeAiBehavior = run.aiBehavior;
  window.gameplayTapeAiIntent = run.aiIntent;
  window.sessionOutcome = run.sessionOutcome;
  window.runtimeStateHash = run.runtimeStateHash;
  // branch-gate: BG-1032
  if (!run.ok) {
    window.status = "gameplay_tape_failed";
  }
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

  const SessionState stateBeforeRun = request.session->state();
  const std::size_t commandLogRecordBoundary =
      request.session->state().commandLog.records().size();
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

    const StatusResult tick =
        tickProductGameplayTape(*request.session,
                                currentCollisionSurfaces(request),
                                request.usePhysicsMovePlanner);
    if (tick.status != ResultStatus::Ok) {
      return fail(std::move(result),
                  request.session,
                  tick.error.code.empty() ? "gameplay_tape_tick_failed"
                                          : tick.error.code,
                  stepIndex,
                  &step);
    }
    refreshActiveRoomCollision(request);
    if (step.action == ProductGameplayTapeAction::Move) {
      const MovementBlockedReason movementBlock = lastMovementBlock(*request.session);
      result.lastMovementBlock =
          std::string(productGameplayTapeMovementBlockName(movementBlock));
      if (step.expectMovementBlock) {
        if (movementBlock == step.expectedMovementBlock) {
          ++result.expectedBlockedStepCount;
          continue;
        }
        return fail(std::move(result),
                    request.session,
                    "gameplay_tape_expected_movement_block_mismatch",
                    stepIndex,
                    &step,
                    CommandRejectionReason::None,
                    movementBlock);
      }
      if (movementBlock != MovementBlockedReason::None) {
        return fail(std::move(result),
                    request.session,
                    "gameplay_tape_movement_blocked",
                    stepIndex,
                    &step,
                    CommandRejectionReason::None,
                    movementBlock);
      }
    }
    ++result.executedStepCount;
  }

  fillLoopFacts(*request.session, result);
  fillAiTapeFacts(stateBeforeRun, request.session->state(), commandLogRecordBoundary, result);
  result.ok = true;
  result.status = "gameplay_tape_completed";
  result.reasonCode = "gameplay_tape_completed";
  result.failedStepIndex = 0;
  result.failedSourceLine = 0;
  result.failedAction = "none";
  result.failedTarget = "none";
  result.failedRejection = "none";
  result.failedMovementBlock = "none";
  return result;
}

void runProductGameplayTapeFromOptions(
    const ProductGameplayTapeOptionsRunRequest& request) {
  // branch-gate: BG-1032
  if (request.options.gameplayTapePath.empty()) {
    return;
  }

  request.window.gameplayTapeRequested = true;
  request.window.gameplayTapePath = request.options.gameplayTapePath.generic_string();
  const ProductGameplayTapeParseResult parsed =
      loadProductGameplayTapeFile(request.options.gameplayTapePath);
  recordProductGameplayTapeParse(parsed, request.window);
  // branch-gate: BG-1032
  if (!parsed.ok) {
    request.window.status = "gameplay_tape_parse_failed";
    return;
  }

  const SpatialSurfaceSet* collisionSurfaces =
      productActiveRoomCollisionSurfaces(request.window.activeRoomCollision);
  const ProductGameplayTapeRunResult run = runProductGameplayTape(
      ProductGameplayTapeRunRequest{
          // branch-gate: BG-1032
          request.activeSession.has_value() ? &*request.activeSession : nullptr,
          &parsed.tape,
          collisionSurfaces,
          &request.window.activeRoom,
          &request.window.activeRoomCollision,
          request.window.physicsMovementPlannerEnabled});
  recordProductGameplayTapeRun(run, request.window);
  // branch-gate: BG-1120
  if (request.activeSession.has_value()) {
    recordProductGameplayTapeMovementDebug(*request.activeSession,
                                           run,
                                           request.window);
  }
  recordProductPhysicsMovementPlannerTickProof(
      request.window,
      request.window.physicsMovementPlannerEnabled,
      collisionSurfaces != nullptr,
      request.activeSession.has_value() &&
          lastMovementHasPhysicsStats(*request.activeSession));
}

}  // namespace iggy3d
