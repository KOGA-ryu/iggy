#include "content/PackageLoader.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/RendererApi.hpp"
#include "runtime/diagnostics/RuntimeSummary.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionRunner.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct CommandSnapshot {
  iggy3d::CommandId commandId = iggy3d::kInvalidCommandId;
  iggy3d::CommandSequence sequence = iggy3d::kInvalidCommandSequence;
  iggy3d::CommandKind kind = iggy3d::CommandKind::None;
  iggy3d::CommandAdmissionStatus admission = iggy3d::CommandAdmissionStatus::Pending;
  iggy3d::CommandRejectionReason rejection = iggy3d::CommandRejectionReason::None;
};

struct ScriptResult {
  bool ok = false;
  iggy3d::Session session;
  std::vector<CommandSnapshot> commands;
  iggy3d::CommandId retryCommandId = iggy3d::kInvalidCommandId;
  iggy3d::CommandSequence retrySequence = iggy3d::kInvalidCommandSequence;
  std::uint64_t rendererFrameCount = 0;
};

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

bool commandAccepted(const iggy3d::SessionCommandResult& result,
                     iggy3d::CommandId commandId,
                     iggy3d::CommandSequence sequence) {
  return result.command.commandId == commandId && result.command.sequence == sequence &&
         result.command.admission == iggy3d::CommandAdmissionStatus::Accepted &&
         result.appendedToLog;
}

CommandSnapshot snapshot(const iggy3d::SessionCommandResult& result) {
  return {result.command.commandId, result.command.sequence, result.command.kind,
          result.command.admission, result.command.rejection};
}

bool snapshotsEqual(const std::vector<CommandSnapshot>& lhs,
                    const std::vector<CommandSnapshot>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (lhs[index].commandId != rhs[index].commandId ||
        lhs[index].sequence != rhs[index].sequence || lhs[index].kind != rhs[index].kind ||
        lhs[index].admission != rhs[index].admission ||
        lhs[index].rejection != rhs[index].rejection) {
      return false;
    }
  }
  return true;
}

bool runQueuedCommand(iggy3d::Session& session) {
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 8, true, true});
  return run.status == iggy3d::SessionRunnerStatus::Advanced && run.ticksAdvanced == 1U;
}

iggy3d::FrameInput frameForState(const iggy3d::SceneProjectionResult& scene,
                                 const iggy3d::DebugProjectionResult& debug,
                                 std::uint64_t frameIndex,
                                 float deltaSeconds) {
  iggy3d::FrameInput frame;
  frame.viewport = {640U, 360U, 640.0F / 360.0F};
  frame.clock = {scene.sourceTick, frameIndex, 0.0F, deltaSeconds};
  frame.camera.mode = iggy3d::RenderCameraMode::ThirdPerson;
  if (scene.cameraMode == iggy3d::CameraMode::FirstPerson) {
    frame.camera.mode = iggy3d::RenderCameraMode::FirstPerson;
  }
  if (scene.cameraMode == iggy3d::CameraMode::TacticalOverhead) {
    frame.camera.mode = iggy3d::RenderCameraMode::TacticalOverhead;
  }
  frame.camera.worldEye = {0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.projections.scene = &scene;
  frame.projections.debug = &debug;
  return frame;
}

bool renderCurrentState(iggy3d::RendererApi& renderer,
                        const iggy3d::SessionState& state,
                        std::uint64_t frameIndex,
                        float deltaSeconds) {
  const iggy3d::StateHashValue hashBefore = state.currentStateHash;
  const std::size_t logSizeBefore = state.commandLog.size();
  const iggy3d::SceneProjectionResult scene = iggy3d::buildSceneProjection(state);
  const iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(state);
  const iggy3d::FrameInput frame = frameForState(scene, debug, frameIndex, deltaSeconds);
  const iggy3d::RenderSubmitResult result = renderer.submitFrame(frame);
  return result.outcome == iggy3d::RenderOutcome::Ok &&
         state.currentStateHash == hashBefore && state.commandLog.size() == logSizeBefore;
}

std::string summaryFor(const iggy3d::SessionState& state,
                       iggy3d::CommandId retryCommandId,
                       iggy3d::CommandSequence retrySequence) {
  iggy3d::RuntimeSummaryInput input;
  input.state = &state;
  input.retryExecutedCommandId = retryCommandId;
  input.retryExecutedSequence = retrySequence;
  input.saveRoundtrip = iggy3d::RuntimeProofStatus::Pass;
  input.resetBaseline = iggy3d::RuntimeProofStatus::Pass;
  input.replayHash = iggy3d::RuntimeProofStatus::Pass;
  return iggy3d::formatRuntimeSummary(iggy3d::buildRuntimeSummary(input));
}

ScriptResult runFullScript(bool withRenderer) {
  ScriptResult result;
  const iggy3d::SessionCreateRequest create = createRequestFromPackage();
  iggy3d::Session session = iggy3d::Session::create(create).value;
  iggy3d::RendererApi renderer =
      withRenderer ? iggy3d::createRenderer({iggy3d::RendererBackendKind::Null, {}})
                   : iggy3d::RendererApi{};
  std::uint64_t frameIndex = 1U;

  const iggy3d::SessionCommandResult interact = session.submitCommand(submittedInteract());
  result.commands.push_back(snapshot(interact));
  bool ok = expect(interact.command.commandId == 1U, "interact id") &&
            expect(interact.command.rejection == iggy3d::CommandRejectionReason::OutOfRange,
                   "interact rejected out of range");

  const iggy3d::SessionCommandResult moveToKey =
      session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  result.commands.push_back(snapshot(moveToKey));
  ok = ok && expect(commandAccepted(moveToKey, 2U, 2U), "move accepted") &&
       expect(runQueuedCommand(session), "move run");
  if (withRenderer) {
    ok = ok && expect(renderCurrentState(renderer, session.state(), frameIndex++, 1.0F / 60.0F),
                      "render after move");
    ++result.rendererFrameCount;
  }

  const iggy3d::SessionCommandResult retry = session.submitCommand(submittedRetry(1U));
  result.commands.push_back(snapshot(retry));
  result.retryCommandId = retry.command.commandId;
  result.retrySequence = retry.command.sequence;
  ok = ok && expect(commandAccepted(retry, 3U, 3U), "retry accepted") &&
       expect(runQueuedCommand(session), "retry run");
  if (withRenderer) {
    ok = ok && expect(renderCurrentState(renderer, session.state(), frameIndex++, 1.0F / 30.0F),
                      "render after retry");
    ++result.rendererFrameCount;
  }

  const iggy3d::SessionCommandResult enter =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  result.commands.push_back(snapshot(enter));
  ok = ok && expect(commandAccepted(enter, 4U, 4U), "enter tactical");

  const iggy3d::SessionCommandResult tacticalMove =
      session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  result.commands.push_back(snapshot(tacticalMove));
  ok = ok && expect(commandAccepted(tacticalMove, 5U, 5U), "tactical move") &&
       expect(runQueuedCommand(session), "tactical move run");
  if (withRenderer) {
    ok = ok && expect(renderCurrentState(renderer, session.state(), frameIndex++, 0.0F),
                      "render after tactical move");
    ++result.rendererFrameCount;
  }

  const iggy3d::SessionCommandResult pause =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  result.commands.push_back(snapshot(pause));
  const iggy3d::SessionCommandResult step =
      session.submitCommand(submittedControl(iggy3d::CommandKind::StepTacticalTick));
  result.commands.push_back(snapshot(step));
  const iggy3d::SessionCommandResult resume =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Resume));
  result.commands.push_back(snapshot(resume));
  ok = ok && expect(commandAccepted(pause, 6U, 6U), "pause") &&
       expect(commandAccepted(step, 7U, 7U), "step") &&
       expect(commandAccepted(resume, 8U, 8U), "resume");

  const iggy3d::SessionCommandResult wait = session.submitCommand(submittedWait());
  result.commands.push_back(snapshot(wait));
  ok = ok && expect(commandAccepted(wait, 9U, 9U), "wait") &&
       expect(runQueuedCommand(session), "wait run");
  if (withRenderer) {
    ok = ok && expect(renderCurrentState(renderer, session.state(), frameIndex++, 1.0F / 120.0F),
                      "render after wait");
    ++result.rendererFrameCount;
  }

  const iggy3d::SessionCommandResult exit =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  result.commands.push_back(snapshot(exit));
  const iggy3d::SessionFinalizationResult finalized = session.finalizeDemoIfComplete();
  ok = ok && expect(commandAccepted(exit, 10U, 10U), "exit tactical") &&
       expect(finalized.status == iggy3d::SessionFinalizationStatus::Completed, "finalized");

  if (withRenderer) {
    ok = ok && expect(renderCurrentState(renderer, session.state(), frameIndex++, 1.0F / 60.0F),
                      "render after final");
    ++result.rendererFrameCount;
  }

  result.ok = ok;
  result.session = std::move(session);
  return result;
}

bool runtimeOutputsAreInvariant() {
  const ScriptResult withoutRenderer = runFullScript(false);
  const ScriptResult withRenderer = runFullScript(true);
  const iggy3d::SessionState& withoutState = withoutRenderer.session.state();
  const iggy3d::SessionState& withState = withRenderer.session.state();
  const std::string withoutSummary =
      summaryFor(withoutState, withoutRenderer.retryCommandId, withoutRenderer.retrySequence);
  const std::string withSummary =
      summaryFor(withState, withRenderer.retryCommandId, withRenderer.retrySequence);

  return expect(withoutRenderer.ok && withRenderer.ok, "scripts ok") &&
         expect(withRenderer.rendererFrameCount == 5U, "renderer frame count") &&
         expect(withoutState.currentStateHash == withState.currentStateHash, "hash invariant") &&
         expect(withoutSummary == withSummary, "summary invariant") &&
         expect(snapshotsEqual(withoutRenderer.commands, withRenderer.commands),
                "command results invariant") &&
         expect(withState.lifecycle == iggy3d::SessionLifecycle::Complete, "lifecycle complete") &&
         expect(withState.outcome == iggy3d::SessionOutcome::DemoComplete, "outcome complete") &&
         expect(!withState.world.findByStableName("gold_key")->active, "key inactive") &&
         expect(iggy3d::objectiveComplete(withState.objectives, "collect_gold_key"),
                "objective complete");
}

bool rendererMetadataDoesNotChangeRuntime() {
  ScriptResult result = runFullScript(false);
  iggy3d::RendererApi renderer =
      iggy3d::createRenderer({iggy3d::RendererBackendKind::Null, {}});
  const iggy3d::StateHashValue hashBefore = result.session.state().currentStateHash;
  const std::string summaryBefore =
      summaryFor(result.session.state(), result.retryCommandId, result.retrySequence);

  bool ok = expect(renderCurrentState(renderer, result.session.state(), 101U, 0.0F),
                   "render index 101") &&
            expect(result.session.state().currentStateHash == hashBefore, "hash after index") &&
            expect(renderCurrentState(renderer, result.session.state(), 202U, 1.0F / 24.0F),
                   "render index 202") &&
            expect(result.session.state().currentStateHash == hashBefore, "hash after delta");

  iggy3d::FrameInput rejectedFrame;
  const iggy3d::RenderSubmitResult rejected = renderer.submitFrame(rejectedFrame);
  ok = ok && expect(rejected.outcome == iggy3d::RenderOutcome::InvalidFrameInput,
                    "rejected frame") &&
       expect(result.session.state().currentStateHash == hashBefore, "hash after rejected frame");

  renderer.shutdown();
  ok = ok && expect(result.session.state().currentStateHash == hashBefore, "hash after shutdown") &&
       expect(summaryFor(result.session.state(), result.retryCommandId, result.retrySequence) ==
                  summaryBefore,
              "summary after renderer metadata");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = runtimeOutputsAreInvariant() && ok;
  ok = rendererMetadataDoesNotChangeRuntime() && ok;
  return ok ? 0 : 1;
}
