#include "content/PackageLoader.hpp"
#include "runtime/session/SessionRunner.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

iggy3d::Session makeSession() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{"fixtures/demos/first_room/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  return iggy3d::Session::create(create).value;
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

iggy3d::CommandRecord submittedWait() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Wait;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

iggy3d::CommandRecord submittedControl(iggy3d::CommandKind kind) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.kind = kind;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

bool runUntilIdleExecutesQueuedMovement() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult move = session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 4, true, true});
  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  return expect(move.command.admission == iggy3d::CommandAdmissionStatus::Accepted, "move accepted") &&
         expect(run.status == iggy3d::SessionRunnerStatus::Advanced, "runner advanced") &&
         expect(run.ticksAdvanced == 1U, "runner one tick") &&
         expect(session.state().transient.pendingExecutionSequences.empty(), "runner queue empty") &&
         expect(player != nullptr &&
                    iggy3d::nearlyEqual(player->transform.position, {2.0F, 0.0F, 0.0F}),
                "runner moved player");
}

bool pausedAutoRunAndStepBehavior() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult enter =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  const iggy3d::SessionCommandResult pause =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  const iggy3d::CommandTick pausedTick = session.state().clock.tickIndex;
  const iggy3d::SessionRunnerRunResult idle =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 4, true, true});
  const bool idlePreservedTick = session.state().clock.tickIndex == pausedTick;
  const iggy3d::SessionRunnerRunResult stepped = iggy3d::stepPausedOnce(session);
  return expect(enter.command.admission == iggy3d::CommandAdmissionStatus::Accepted, "enter accepted") &&
         expect(pause.command.admission == iggy3d::CommandAdmissionStatus::Accepted, "pause accepted") &&
         expect(session.state().clock.mode == iggy3d::ClockMode::Paused, "clock paused") &&
         expect(idle.status == iggy3d::SessionRunnerStatus::Idle, "paused idle") &&
         expect(idle.ticksAdvanced == 0U, "paused no auto advance") &&
         expect(idlePreservedTick, "paused tick unchanged") &&
         expect(stepped.status == iggy3d::SessionRunnerStatus::Advanced, "step advanced") &&
         expect(stepped.ticksAdvanced == 1U, "step one tick") &&
         expect(session.state().clock.tickIndex == pausedTick + 1U, "step tick index") &&
         expect(session.state().clock.mode == iggy3d::ClockMode::Paused, "step remains paused") &&
         expect(!session.state().clock.stepRequested, "step request consumed");
}

bool maxTicksExceededAndStopWhenComplete() {
  iggy3d::Session session = makeSession();
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  const iggy3d::SessionRunnerRunResult exceeded =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 0, true, true});

  iggy3d::Session complete = makeSession();
  iggy3d::SessionState& state = complete.mutableStateForOwnedSystems();
  state.lifecycle = iggy3d::SessionLifecycle::Complete;
  state.outcome = iggy3d::SessionOutcome::DemoComplete;
  const iggy3d::SessionRunnerRunResult stopped =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&complete, 4, true, true});
  return expect(exceeded.status == iggy3d::SessionRunnerStatus::MaxTicksExceeded,
                "max ticks exceeded") &&
         expect(stopped.status == iggy3d::SessionRunnerStatus::Complete, "stop complete");
}

bool immediateControlCommandExecutionAndPausedLegality() {
  iggy3d::Session session = makeSession();
  const iggy3d::SessionCommandResult enter =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  const iggy3d::SessionCommandResult pause =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  const iggy3d::SessionCommandResult wait = session.submitCommand(submittedWait());
  const iggy3d::SessionCommandResult toggle =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  const iggy3d::CommandTick tickBeforeStep = session.state().clock.tickIndex;
  const iggy3d::SessionCommandResult step =
      session.submitCommand(submittedControl(iggy3d::CommandKind::StepTacticalTick));
  const iggy3d::SessionCommandResult resume =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Resume));

  return expect(enter.executedImmediately, "enter immediate") &&
         expect(session.state().camera.previousRealtimeMode == iggy3d::CameraMode::ThirdPerson,
                "previous realtime") &&
         expect(pause.executedImmediately, "pause immediate") &&
         expect(wait.command.admission == iggy3d::CommandAdmissionStatus::Rejected &&
                    wait.command.rejection == iggy3d::CommandRejectionReason::SessionPaused,
                "wait rejected paused") &&
         expect(toggle.command.admission == iggy3d::CommandAdmissionStatus::Rejected &&
                    toggle.command.rejection == iggy3d::CommandRejectionReason::SessionPaused,
                "toggle rejected paused") &&
         expect(step.command.admission == iggy3d::CommandAdmissionStatus::Accepted &&
                    step.executedImmediately,
                "step accepted immediate") &&
         expect(session.state().clock.tickIndex == tickBeforeStep + 1U, "step command tick") &&
         expect(resume.command.admission == iggy3d::CommandAdmissionStatus::Accepted &&
                    resume.executedImmediately,
                "resume accepted immediate") &&
         expect(session.state().clock.mode == iggy3d::ClockMode::Slow, "resume slow") &&
         expect(session.state().camera.activeMode == iggy3d::CameraMode::TacticalOverhead,
                "resume tactical camera");
}

}  // namespace

int main() {
  bool ok = true;
  ok = runUntilIdleExecutesQueuedMovement() && ok;
  ok = pausedAutoRunAndStepBehavior() && ok;
  ok = maxTicksExceededAndStopWhenComplete() && ok;
  ok = immediateControlCommandExecutionAndPausedLegality() && ok;
  return ok ? 0 : 1;
}
