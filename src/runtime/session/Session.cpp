#include "runtime/session/Session.hpp"

#include <utility>
#include <vector>

#include "core/math/Vec3.hpp"
#include "runtime/ability/AbilitySystem.hpp"
#include "runtime/camera/CameraModePolicy.hpp"
#include "runtime/clock/Clock.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"
#include "runtime/session/SessionInternal.hpp"
#include "runtime/session/SessionRunner.hpp"
#include "runtime/session/SessionTick.hpp"

namespace iggy3d {

namespace {

bool queuesForTickExecution(CommandKind kind) {
  return kind == CommandKind::Move || kind == CommandKind::Interact ||
         kind == CommandKind::Inspect || kind == CommandKind::Attack ||
         kind == CommandKind::CastAbility || kind == CommandKind::Wait ||
         kind == CommandKind::Retry;
}

bool sequencePending(const std::vector<CommandSequence>& pending, CommandSequence sequence) {
  for (CommandSequence candidate : pending) {
    if (candidate == sequence) {
      return true;
    }
  }
  return false;
}

bool pendingAbilityCastCommand(const SessionState& state) {
  for (const CommandRecord& record : state.commandLog.records()) {
    if (record.kind == CommandKind::CastAbility &&
        record.admission == CommandAdmissionStatus::Accepted &&
        sequencePending(state.transient.pendingExecutionSequences, record.sequence)) {
      return true;
    }
  }
  return false;
}

AbilityId abilityIdForCommand(CommandAbilityKind ability) {
  switch (ability) {
    case CommandAbilityKind::ArcaneBolt:
      return AbilityId::ArcaneBolt;
    case CommandAbilityKind::None:
      break;
  }
  return AbilityId::None;
}

CommandRejectionReason rejectionForAbilityCastStatus(AbilityCastStatus status) {
  switch (status) {
    case AbilityCastStatus::Accepted:
      return CommandRejectionReason::None;
    case AbilityCastStatus::ProjectileSlotBusy:
      return CommandRejectionReason::AbilitySlotBusy;
    case AbilityCastStatus::OnCooldown:
      return CommandRejectionReason::AbilityOnCooldown;
    case AbilityCastStatus::InsufficientResource:
      return CommandRejectionReason::AbilityInsufficientResource;
    case AbilityCastStatus::InvalidAbility:
    case AbilityCastStatus::InvalidCaster:
    case AbilityCastStatus::InvalidOrigin:
    case AbilityCastStatus::InvalidDirection:
    case AbilityCastStatus::MissingCollisionSurfaces:
      break;
  }
  return CommandRejectionReason::InvalidCommand;
}

AbilityCastRequest abilityCastRequestFromCommand(const SessionState& state,
                                                 const CommandRecord& command,
                                                 const SpatialSurfaceSet& surfaces) {
  AbilityCastRequest request;
  request.ability = abilityIdForCommand(command.payload.ability);
  request.caster = command.actor;
  const EntityState* actor = state.world.findById(command.actor);
  request.originMeters = actor == nullptr
                             ? Vec3{}
                             : actor->transform.position + Vec3{0.0F, 1.65F, 0.0F};
  request.direction = command.payload.abilityDirection;
  request.collisionSurfaces = &surfaces;
  request.sourceCommandId = command.commandId;
  request.currentTick = state.clock.tickIndex;
  return request;
}

CommandAdmissionResult applyAbilityRuntimeAdmission(const SessionState& state,
                                                    const CommandAdmissionResult& admission) {
  if (admission.command.kind != CommandKind::CastAbility ||
      admission.command.admission != CommandAdmissionStatus::Accepted) {
    return admission;
  }
  if (pendingAbilityCastCommand(state)) {
    return rejectCommand(admission.command, CommandRejectionReason::AbilitySlotBusy);
  }

  const SpatialSurfaceSet emptySurfaces;
  const AbilityCastRequest request =
      abilityCastRequestFromCommand(state, admission.command, emptySurfaces);
  const AbilityCastResult inspected =
      inspectAbilityCast(state.abilities, state.transient.abilityRuntime, request);
  if (inspected.accepted) {
    return admission;
  }
  return rejectCommand(admission.command, rejectionForAbilityCastStatus(inspected.status));
}

bool executesImmediately(CommandKind kind) {
  return kind == CommandKind::ToggleTacticalMode || kind == CommandKind::Pause ||
         kind == CommandKind::Resume || kind == CommandKind::StepTacticalTick;
}

void removeExecutedSequences(std::vector<CommandSequence>& pending,
                             const std::vector<CommandSequence>& executed) {
  std::vector<CommandSequence> remaining;
  remaining.reserve(pending.size());
  for (CommandSequence sequence : pending) {
    if (!sequencePending(executed, sequence)) {
      remaining.push_back(sequence);
    }
  }
  pending = std::move(remaining);
}

std::vector<CommandRecord> pendingAcceptedCommands(const SessionState& state) {
  std::vector<CommandRecord> commands;
  for (const CommandRecord& record : state.commandLog.records()) {
    if (sequencePending(state.transient.pendingExecutionSequences, record.sequence) &&
        record.admission == CommandAdmissionStatus::Accepted && queuesForTickExecution(record.kind)) {
      commands.push_back(record);
    }
  }
  return commands;
}

bool tickSucceeded(SessionTickStatus status) {
  return status == SessionTickStatus::Stepped || status == SessionTickStatus::NoWork;
}

void markDirtyAndHash(SessionState& state) {
  state.transient.summaryDirty = true;
  state.transient.stateHashDirty = false;
  state.currentStateHash = computeStateHash(state);
}

bool applyCameraPolicy(SessionState& state) {
  const CameraModePolicyResult camera =
      applyClockModeToCamera(CameraModePolicyRequest{state.camera, state.clock.mode});
  if (camera.status != CameraPolicyStatus::Ok) {
    return false;
  }
  state.camera = camera.camera;
  state.transient.cameraInputClearRequested =
      state.transient.cameraInputClearRequested || camera.camera.inputClearRequested;
  return true;
}

bool applyControlCommand(Session& session, SessionState& state, CommandKind kind) {
  if (kind == CommandKind::ToggleTacticalMode) {
    ClockDecision clock;
    if (state.clock.mode == ClockMode::Normal) {
      clock = enterSlow(state.clock, state.config.slowTimeScale);
    } else if (state.clock.mode == ClockMode::Slow) {
      clock = exitSlow(state.clock);
    } else {
      return false;
    }
    if (clock.status != ClockStatus::Ok) {
      return false;
    }
    state.clock = clock.state;
    return applyCameraPolicy(state);
  }

  if (kind == CommandKind::Pause) {
    const ClockDecision clock = pause(state.clock);
    if (clock.status != ClockStatus::Ok) {
      return false;
    }
    state.clock = clock.state;
    return applyCameraPolicy(state);
  }

  if (kind == CommandKind::Resume) {
    const ClockDecision clock = resume(state.clock);
    if (clock.status != ClockStatus::Ok) {
      return false;
    }
    state.clock = clock.state;
    return applyCameraPolicy(state);
  }

  if (kind == CommandKind::StepTacticalTick) {
    const ClockDecision requested = requestStep(state.clock);
    if (requested.status != ClockStatus::Ok) {
      return false;
    }
    state.clock = requested.state;
    return session.stepOneTick().status == ResultStatus::Ok;
  }

  return false;
}

PhysicsSpatialSurfaceColliderBakeResult bakeSessionTickSurfaceColliders(
    const SpatialSurfaceSet* collisionSurfaces) {
  if (collisionSurfaces == nullptr) {
    return {};
  }

  PhysicsSpatialSurfaceColliderBakeRequest bakeRequest;
  bakeRequest.surfaces = collisionSurfaces;
  return bakePhysicsAabbCollidersFromSpatialSurfaces(bakeRequest);
}

}  // namespace

namespace session_detail {

SessionCommandResult appendCommandThroughAdmission(Session& session,
                                                   SessionState& state,
                                                   const CommandRecord& command) {
  CommandRecord candidate = command;
  candidate.commandId = state.nextCommandId;
  candidate.sequence = kInvalidCommandSequence;
  candidate.issuedTick = state.clock.tickIndex;
  candidate.scheduledTick = state.clock.tickIndex;

  CommandAdmissionResult admission;
  if (state.lifecycle != SessionLifecycle::Playing) {
    admission = rejectCommand(candidate, CommandRejectionReason::SessionNotPlaying);
  } else {
    CommandAdmissionContext context{&state.world, &state.players, &state.clock,
                                    &state.commandLog, &state.config, &state.combat,
                                    &state.inventory};
    admission = applyAbilityRuntimeAdmission(
        state, admitCommand(context, CommandAdmissionRequest{candidate}));
  }

  const CommandLogAppendResult append = state.commandLog.append(admission.command);
  SessionCommandResult result;
  result.command = append.status == CommandLogAppendStatus::Ok ? append.record : admission.command;
  result.admission = admission;
  result.appendStatus = append.status;
  result.appendedToLog = append.status == CommandLogAppendStatus::Ok;
  if (append.status == CommandLogAppendStatus::Ok) {
    state.nextCommandId = candidate.commandId + 1U;
    if (append.record.admission == CommandAdmissionStatus::Accepted) {
      if (executesImmediately(append.record.kind)) {
        result.executedImmediately = applyControlCommand(session, state, append.record.kind);
      } else if (queuesForTickExecution(append.record.kind)) {
        state.transient.pendingExecutionSequences.push_back(append.record.sequence);
      }
    }
    markDirtyAndHash(state);
  }
  return result;
}

}  // namespace session_detail

const SessionState& Session::state() const {
  return state_;
}

SessionState& Session::mutableStateForOwnedSystems() {
  return state_;
}

void Session::setReasoningGraph(ReasoningGraph graph) {
  // Set-once carry: the session takes ownership of the caller-built graph. Off StateHash/SaveCodec,
  // so this never shifts a receipt or hash (A3 zero-behavior-change contract).
  state_.reasoningGraph = std::move(graph);
}

SessionLifecycle Session::lifecycle() const {
  return state_.lifecycle;
}

SessionOutcome Session::outcome() const {
  return state_.outcome;
}

std::uint64_t Session::stateHash() const {
  return state_.currentStateHash;
}

SessionCommandResult Session::submitCommand(const CommandRecord& command) {
  return session_detail::appendCommandThroughAdmission(*this, state_, command);
}

StatusResult Session::tick(const SpatialSurfaceSet* collisionSurfaces) {
  return tickWithOptions(SessionTickOptions{collisionSurfaces, false});
}

StatusResult Session::tickWithOptions(const SessionTickOptions& options) {
  // Bake once for this tick. A successful empty bake is clear-capable open
  // space; an absent/failed bake stays Unknown for sense callers.
  const PhysicsSpatialSurfaceColliderBakeResult tickSurfaceBake =
      bakeSessionTickSurfaceColliders(options.collisionSurfaces);
  session_detail::enqueueNpcBehaviorCommands(*this, state_, tickSurfaceBake);
  std::vector<CommandRecord> commands = pendingAcceptedCommands(state_);
  const SessionTickResult tick =
      runSessionTick(SessionTickInput{&state_,
                                      std::move(commands),
                                      options.collisionSurfaces,
                                      false,
                                      options.usePhysicsMovePlanner,
                                      options.collisionSurfaces == nullptr
                                          ? nullptr
                                          : &tickSurfaceBake});
  removeExecutedSequences(state_.transient.pendingExecutionSequences, tick.executedSequences);

  markDirtyAndHash(state_);

  if (tickSucceeded(tick.status)) {
    return session_detail::statusOk();
  }
  if (tick.status == SessionTickStatus::BlockedByPausedClock) {
    return session_detail::statusError("session.tick_paused", "session tick blocked by paused clock");
  }
  if (tick.status == SessionTickStatus::SessionNotPlayable) {
    return session_detail::statusError("session.tick_not_playable", "session is not playable");
  }
  return session_detail::statusError("session.tick_invalid_state", "session tick found invalid runtime state");
}

StatusResult Session::stepOneTick(const SpatialSurfaceSet* collisionSurfaces) {
  return stepOneTickWithOptions(SessionTickOptions{collisionSurfaces, false});
}

StatusResult Session::stepOneTickWithOptions(const SessionTickOptions& options) {
  if (state_.clock.mode != ClockMode::Paused || !state_.clock.stepRequested) {
    return session_detail::statusError("session.step_requires_paused", "paused step was not requested");
  }

  const ClockDecision consumed = consumeStep(state_.clock);
  if (consumed.status != ClockStatus::Ok || !consumed.consumedStep) {
    return session_detail::statusError("session.step_invalid_clock", "paused step could not be consumed");
  }
  state_.clock = consumed.state;

  std::vector<CommandRecord> commands = pendingAcceptedCommands(state_);
  if (commands.empty() && !abilityRuntimeHasActiveProjectile(state_.transient.abilityRuntime) &&
      !abilityStateHasPendingRecharge(state_.abilities)) {
    state_.clock = advanceTick(state_.clock);
    ++state_.transient.metrics.ticksRun;
    markDirtyAndHash(state_);
    return session_detail::statusOk();
  }

  const PhysicsSpatialSurfaceColliderBakeResult tickSurfaceBake =
      bakeSessionTickSurfaceColliders(options.collisionSurfaces);
  const SessionTickResult tick =
      runSessionTick(SessionTickInput{&state_,
                                      std::move(commands),
                                      options.collisionSurfaces,
                                      true,
                                      options.usePhysicsMovePlanner,
                                      options.collisionSurfaces == nullptr
                                          ? nullptr
                                          : &tickSurfaceBake});
  removeExecutedSequences(state_.transient.pendingExecutionSequences, tick.executedSequences);
  markDirtyAndHash(state_);

  if (tickSucceeded(tick.status)) {
    return session_detail::statusOk();
  }
  return session_detail::statusError("session.step_invalid_state", "paused step found invalid runtime state");
}

StatusResult Session::runUntilIdle(std::uint32_t maxTicks,
                                   const SpatialSurfaceSet* collisionSurfaces) {
  return runUntilIdleWithOptions(maxTicks, SessionTickOptions{collisionSurfaces, false});
}

StatusResult Session::runUntilIdleWithOptions(std::uint32_t maxTicks,
                                              const SessionTickOptions& options) {
  SessionRunnerRunResult run =
      runSession(SessionRunnerRunRequest{this,
                                         maxTicks,
                                         true,
                                         true,
                                         options.collisionSurfaces,
                                         options.usePhysicsMovePlanner});
  if (run.status == SessionRunnerStatus::Failed) {
    return session_detail::statusError("session.runner_failed", run.diagnostic);
  }
  if (run.status == SessionRunnerStatus::MaxTicksExceeded) {
    return session_detail::statusError("session.runner_max_ticks", run.diagnostic);
  }
  return session_detail::statusOk();
}

SessionFinalizationResult Session::finalizeDemoIfComplete() {
  SessionFinalizationResult result;
  result.previousLifecycle = state_.lifecycle;
  result.lifecycle = state_.lifecycle;
  result.outcome = state_.outcome;
  result.stateHash = state_.currentStateHash;

  if (state_.lifecycle == SessionLifecycle::Complete && state_.outcome == SessionOutcome::DemoComplete) {
    result.status = SessionFinalizationStatus::AlreadyComplete;
    return result;
  }
  if (state_.lifecycle != SessionLifecycle::Playing) {
    result.status = SessionFinalizationStatus::Failed;
    return result;
  }
  if (state_.outcome != SessionOutcome::DemoComplete || state_.clock.mode != ClockMode::Normal ||
      !isRealtimeCameraMode(state_.camera.activeMode) ||
      !state_.transient.pendingExecutionSequences.empty() ||
      abilityRuntimeHasActiveProjectile(state_.transient.abilityRuntime)) {
    result.status = SessionFinalizationStatus::NotReady;
    return result;
  }

  state_.lifecycle = SessionLifecycle::Complete;
  markDirtyAndHash(state_);
  result.status = SessionFinalizationStatus::Completed;
  result.lifecycle = state_.lifecycle;
  result.outcome = state_.outcome;
  result.stateHash = state_.currentStateHash;
  return result;
}

}  // namespace iggy3d
