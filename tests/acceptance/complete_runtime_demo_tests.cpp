#include "content/PackageLoader.hpp"
#include "runtime/diagnostics/RuntimeSummary.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionRunner.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

struct ScriptResult {
  bool ok = false;
  iggy3d::Session session;
  iggy3d::CommandId retryCommandId = iggy3d::kInvalidCommandId;
  iggy3d::CommandSequence retrySequence = iggy3d::kInvalidCommandSequence;
  iggy3d::RuntimeProofStatus saveRoundtrip = iggy3d::RuntimeProofStatus::Unknown;
  iggy3d::RuntimeProofStatus resetBaseline = iggy3d::RuntimeProofStatus::Unknown;
};

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool contains(std::string_view text, std::string_view needle) {
  return text.find(needle) != std::string_view::npos;
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream in(path);
  std::ostringstream bytes;
  bytes << in.rdbuf();
  return bytes.str();
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

bool commandAccepted(const iggy3d::SessionCommandResult& result,
                     iggy3d::CommandId commandId,
                     iggy3d::CommandSequence sequence) {
  return result.command.commandId == commandId && result.command.sequence == sequence &&
         result.command.admission == iggy3d::CommandAdmissionStatus::Accepted &&
         result.appendedToLog;
}

bool runQueuedCommand(iggy3d::Session& session) {
  const iggy3d::SessionRunnerRunResult run =
      iggy3d::runSession(iggy3d::SessionRunnerRunRequest{&session, 8, true, true});
  return run.status == iggy3d::SessionRunnerStatus::Advanced && run.ticksAdvanced == 1U;
}

iggy3d::SessionCreateRequest createRequestFromPackage() {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{"fixtures/demos/first_room/package.iggy3d.toml"});
  iggy3d::SessionCreateRequest create;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  return create;
}

bool resetProof(const iggy3d::SessionCreateRequest& create) {
  iggy3d::Session probe = iggy3d::Session::create(create).value;
  const iggy3d::SessionCommandResult move = probe.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  if (!commandAccepted(move, 1, 1) || !runQueuedCommand(probe)) {
    return false;
  }
  const iggy3d::SessionResetResult reset = probe.resetToBaseline();
  const iggy3d::SessionState& state = probe.state();
  const iggy3d::EntityState* player = state.world.findByStableName("player");
  const iggy3d::EntityState* key = state.world.findByStableName("gold_key");
  return reset.reset && reset.baselineHash == reset.currentHash &&
         state.currentStateHash == reset.currentHash && state.nextCommandId == 1U &&
         state.commandLog.empty() && state.commandLog.nextSequence() == 1U &&
         player != nullptr &&
         iggy3d::nearlyEqual(player->transform.position, iggy3d::Vec3{0.0F, 0.0F, 0.0F}) &&
         key != nullptr && key->active && state.clock.mode == iggy3d::ClockMode::Normal &&
         state.camera.activeMode == iggy3d::CameraMode::ThirdPerson &&
         goldKeyCount(state.inventory) == 0U &&
         !iggy3d::objectiveComplete(state.objectives, "collect_gold_key") &&
         state.transient.pendingExecutionSequences.empty();
}

ScriptResult runFullScript() {
  ScriptResult result;
  const iggy3d::SessionCreateRequest create = createRequestFromPackage();
  iggy3d::Session session = iggy3d::Session::create(create).value;

  const iggy3d::SessionCommandResult interact = session.submitCommand(submittedInteract());
  bool ok = expect(interact.command.commandId == 1U && interact.command.sequence == 1U,
                   "cmd_interact_oob id") &&
            expect(interact.command.admission == iggy3d::CommandAdmissionStatus::Rejected,
                   "cmd_interact_oob rejected") &&
            expect(interact.command.rejection == iggy3d::CommandRejectionReason::OutOfRange,
                   "cmd_interact_oob out of range") &&
            expect(session.state().transient.pendingExecutionSequences.empty(), "rejected not queued");

  const iggy3d::SessionCommandResult moveToKey =
      session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  ok = ok && expect(commandAccepted(moveToKey, 2, 2), "cmd_move_to_key accepted") &&
       expect(runQueuedCommand(session), "cmd_move_to_key run");

  const iggy3d::SessionCommandResult retry = session.submitCommand(submittedRetry(1));
  ok = ok && expect(commandAccepted(retry, 3, 3), "cmd_retry_key accepted") &&
       expect(runQueuedCommand(session), "cmd_retry_key run") &&
       expect(goldKeyCount(session.state().inventory) == 1U, "gold key acquired") &&
       expect(!session.state().world.findByStableName("gold_key")->active, "gold key inactive") &&
       expect(iggy3d::objectiveComplete(session.state().objectives, "collect_gold_key"),
              "objective complete") &&
       expect(session.state().outcome == iggy3d::SessionOutcome::DemoComplete, "outcome complete") &&
       expect(session.state().lifecycle == iggy3d::SessionLifecycle::Playing,
              "lifecycle still playing");

  const iggy3d::SessionCommandResult enter =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  ok = ok && expect(commandAccepted(enter, 4, 4), "cmd_enter_tactical accepted") &&
       expect(enter.executedImmediately, "cmd_enter_tactical immediate") &&
       expect(session.state().clock.mode == iggy3d::ClockMode::Slow, "clock slow") &&
       expect(session.state().camera.activeMode == iggy3d::CameraMode::TacticalOverhead,
              "camera tactical");

  const iggy3d::SessionCommandResult tacticalMove =
      session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  ok = ok && expect(commandAccepted(tacticalMove, 5, 5), "cmd_tactical_move accepted") &&
       expect(runQueuedCommand(session), "cmd_tactical_move run") &&
       expect(iggy3d::nearlyEqual(session.state().world.findByStableName("player")->transform.position,
                                  {2.0F, 0.0F, 1.0F}),
              "tactical move position");

  const iggy3d::SessionCommandResult pause =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  ok = ok && expect(commandAccepted(pause, 6, 6), "cmd_pause accepted") &&
       expect(pause.executedImmediately, "cmd_pause immediate") &&
       expect(session.state().clock.mode == iggy3d::ClockMode::Paused, "clock paused");

  const iggy3d::CommandTick tickBeforeStep = session.state().clock.tickIndex;
  const iggy3d::SessionCommandResult step =
      session.submitCommand(submittedControl(iggy3d::CommandKind::StepTacticalTick));
  ok = ok && expect(commandAccepted(step, 7, 7), "cmd_step accepted") &&
       expect(step.executedImmediately, "cmd_step immediate") &&
       expect(session.state().clock.mode == iggy3d::ClockMode::Paused, "step remains paused") &&
       expect(session.state().clock.tickIndex == tickBeforeStep + 1U, "step advanced once");

  const iggy3d::SessionCommandResult resume =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Resume));
  ok = ok && expect(commandAccepted(resume, 8, 8), "cmd_resume accepted") &&
       expect(resume.executedImmediately, "cmd_resume immediate") &&
       expect(session.state().clock.mode == iggy3d::ClockMode::Slow, "resume slow");

  const iggy3d::SessionCommandResult wait = session.submitCommand(submittedWait());
  ok = ok && expect(commandAccepted(wait, 9, 9), "cmd_wait accepted") &&
       expect(runQueuedCommand(session), "cmd_wait run");

  const iggy3d::SessionCommandResult exit =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  ok = ok && expect(commandAccepted(exit, 10, 10), "cmd_exit_tactical accepted") &&
       expect(exit.executedImmediately, "cmd_exit_tactical immediate") &&
       expect(session.state().clock.mode == iggy3d::ClockMode::Normal, "clock normal") &&
       expect(session.state().camera.activeMode == iggy3d::CameraMode::ThirdPerson, "camera realtime");

  const iggy3d::SessionFinalizationResult finalized = session.finalizeDemoIfComplete();
  ok = ok && expect(finalized.status == iggy3d::SessionFinalizationStatus::Completed,
                    "finalized") &&
       expect(session.state().lifecycle == iggy3d::SessionLifecycle::Complete, "lifecycle complete");

  const iggy3d::CommandLogCounts counts = session.state().commandLog.counts();
  ok = ok && expect(session.state().clock.tickIndex == 5U, "final tick") &&
       expect(counts.submitted == 10U, "submitted count") &&
       expect(counts.accepted == 9U, "accepted count") &&
       expect(counts.rejected == 1U, "rejected count") &&
       expect(counts.retry == 1U, "retry count") &&
       expect(counts.control == 5U, "control count") &&
       expect(counts.movement == 2U, "movement count");

  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  iggy3d::Session loaded = iggy3d::Session::create(create).value;
  const iggy3d::SaveCompatibilityRequest compatibility{
      saved.envelope, session.state().identity.packageId, session.state().identity.scenarioId};
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibility);
  const bool roundtrip =
      saved.status == iggy3d::SaveLoadStatus::Ok && load.status == iggy3d::SaveLoadStatus::Ok &&
      loaded.stateHash() == session.stateHash() && goldKeyCount(loaded.state().inventory) == 1U &&
      loaded.state().lifecycle == iggy3d::SessionLifecycle::Complete;

  result.ok = ok && roundtrip;
  result.session = std::move(loaded);
  result.retryCommandId = retry.command.commandId;
  result.retrySequence = retry.command.sequence;
  result.saveRoundtrip =
      roundtrip ? iggy3d::RuntimeProofStatus::Pass : iggy3d::RuntimeProofStatus::Fail;
  result.resetBaseline = resetProof(create) ? iggy3d::RuntimeProofStatus::Pass
                                            : iggy3d::RuntimeProofStatus::Fail;
  result.ok = result.ok && result.resetBaseline == iggy3d::RuntimeProofStatus::Pass;
  return result;
}

bool summaryMatchesFullScript() {
  ScriptResult script = runFullScript();
  bool ok = expect(script.ok, "script ok");
  iggy3d::RuntimeSummaryInput input;
  input.state = &script.session.state();
  input.retryExecutedCommandId = script.retryCommandId;
  input.retryExecutedSequence = script.retrySequence;
  input.saveRoundtrip = script.saveRoundtrip;
  input.resetBaseline = script.resetBaseline;
  input.replayHash = iggy3d::RuntimeProofStatus::Unknown;
  const std::string summary = iggy3d::formatRuntimeSummary(iggy3d::buildRuntimeSummary(input));
  const std::string expected =
      "scenario=iggy3d.first_room:first_room.runtime_loop\n"
      "lifecycle=Complete\n"
      "outcome=DemoComplete\n"
      "final_tick=5\n"
      "player.position=(2.000,0.000,1.000)\n"
      "inventory.player0=gold_key:1\n"
      "gold_key.active=false\n"
      "tactical_marker_alpha.active=true\n"
      "objective.collect_gold_key=Complete\n"
      "clock.mode=Normal\n"
      "camera.mode=ThirdPerson\n"
      "camera.previousRealtime=ThirdPerson\n"
      "commands.submitted=10\n"
      "commands.accepted=9\n"
      "commands.rejected=1\n"
      "commands.retry=1\n"
      "first_rejection=OutOfRange\n"
      "retry.original_rejected_command_id=1\n"
      "retry.retry_command_id=3\n"
      "retry.sourceCommandId=3\n"
      "retry.retrySourceCommandId=1\n"
      "retry.executed.command_id=3\n"
      "retry.executed.sequence=3\n"
      "save.roundtrip=pass\n"
      "reset.baseline=pass\n"
      "replay.hash=unknown\n"
      "state_hash=" +
      iggy3d::formatStateHash(script.session.state().currentStateHash) + "\n";
  ok = ok && expect(summary == expected, "summary exact");
  return ok;
}

bool headlessBinaryExecutes() {
#ifdef IGGY3D_HEADLESS_DEMO_PATH
  const std::filesystem::path outPath =
      std::filesystem::temp_directory_path() / "iggy3d_headless_demo_summary.out";
  const std::string command = std::string("\"") + IGGY3D_HEADLESS_DEMO_PATH +
                              "\" --fixture fixtures/demos/first_room/package.iggy3d.toml > \"" +
                              outPath.string() + "\"";
  const int status = std::system(command.c_str());
  const std::string output = readFile(outPath);
  std::filesystem::remove(outPath);
  return expect(status == 0, "headless status") &&
         expect(contains(output, "lifecycle=Complete\n"), "headless lifecycle") &&
         expect(contains(output, "commands.submitted=10\n"), "headless submitted") &&
         expect(contains(output, "save.roundtrip=pass\n"), "headless save") &&
         expect(contains(output, "reset.baseline=pass\n"), "headless reset") &&
         expect(contains(output, "replay.hash=pass\n"), "headless replay");
#else
  return true;
#endif
}

}  // namespace

int main() {
  bool ok = true;
  ok = summaryMatchesFullScript() && ok;
  ok = headlessBinaryExecutes() && ok;
  return ok ? 0 : 1;
}
