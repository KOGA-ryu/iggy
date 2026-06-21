#include "app/CliParser.hpp"
#include "content/PackageLoader.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/diagnostics/RuntimeSummary.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/replay/CommandReplay.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"
#include "runtime/session/SessionRunner.hpp"

#include <fstream>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <string>

namespace {

struct DemoRunResult {
  bool ok = false;
  std::string summary;
  std::string diagnostic;
};

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

iggy3d::CommandRecord submittedAttack() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Attack;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {4};
  command.payload.attackDamage = 3;
  return command;
}

std::uint32_t itemCount(const iggy3d::InventoryState& inventory, iggy3d::PlayerSlotId slotId,
                        const std::string& itemId) {
  const iggy3d::PlayerInventory* player = iggy3d::findInventory(inventory, slotId);
  if (player == nullptr) {
    return 0;
  }
  for (const iggy3d::InventoryStack& stack : player->stacks) {
    if (stack.itemId == itemId) {
      return stack.count;
    }
  }
  return 0;
}

bool durableFactsMatch(const iggy3d::SessionState& expected, const iggy3d::SessionState& actual) {
  const iggy3d::EntityState* expectedKey = expected.world.findByStableName("gold_key");
  const iggy3d::EntityState* actualKey = actual.world.findByStableName("gold_key");
  return actual.currentStateHash == expected.currentStateHash && expectedKey != nullptr &&
         actualKey != nullptr && actualKey->active == expectedKey->active &&
         itemCount(actual.inventory, 0, "gold_key") == itemCount(expected.inventory, 0, "gold_key") &&
         iggy3d::objectiveComplete(actual.objectives, "collect_gold_key") ==
             iggy3d::objectiveComplete(expected.objectives, "collect_gold_key") &&
         actual.outcome == expected.outcome && actual.lifecycle == expected.lifecycle;
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

bool resetProof(const iggy3d::SessionCreateRequest& create) {
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(create);
  if (created.status != iggy3d::ResultStatus::Ok) {
    return false;
  }
  iggy3d::Session probe = std::move(created.value);
  const iggy3d::SessionCommandResult move = probe.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  if (!commandAccepted(move, 1, 1) || !runQueuedCommand(probe)) {
    return false;
  }
  const iggy3d::SessionResetResult reset = probe.resetToBaseline();
  const iggy3d::SessionState& state = probe.state();
  const iggy3d::EntityState* player = state.world.findByStableName("player");
  const iggy3d::EntityState* key = state.world.findByStableName("gold_key");
  return reset.reset && reset.baselineHash == reset.currentHash &&
         reset.currentHash == state.currentStateHash && state.nextCommandId == 1U &&
         state.commandLog.empty() && state.commandLog.nextSequence() == 1U &&
         player != nullptr &&
         iggy3d::nearlyEqual(player->transform.position, iggy3d::Vec3{0.0F, 0.0F, 0.0F}) &&
         key != nullptr && key->active && state.clock.mode == iggy3d::ClockMode::Normal &&
         state.clock.tickIndex == 0U && state.camera.activeMode == iggy3d::CameraMode::ThirdPerson &&
         itemCount(state.inventory, 0, "gold_key") == 0U &&
         !iggy3d::objectiveComplete(state.objectives, "collect_gold_key") &&
         state.transient.pendingExecutionSequences.empty();
}

bool writeTextFile(const std::filesystem::path& path, const std::string& text) {
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << text;
  return static_cast<bool>(out);
}

bool readTextFile(const std::filesystem::path& path, std::string& text) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return true;
}

DemoRunResult fail(std::string diagnostic) {
  DemoRunResult result;
  result.diagnostic = std::move(diagnostic);
  return result;
}

DemoRunResult runDemo(const iggy3d::AppConfig& config) {
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{config.packagePath.string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok) {
    return fail("package load failed");
  }

  iggy3d::SessionCreateRequest create;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(create);
  if (created.status != iggy3d::ResultStatus::Ok) {
    return fail("session create failed");
  }
  iggy3d::Session session = std::move(created.value);

  const iggy3d::SessionCommandResult interact = session.submitCommand(submittedInteract());
  if (interact.command.commandId != 1U || interact.command.sequence != 1U ||
      interact.command.admission != iggy3d::CommandAdmissionStatus::Rejected ||
      interact.command.rejection != iggy3d::CommandRejectionReason::OutOfRange ||
      !session.state().transient.pendingExecutionSequences.empty()) {
    return fail("cmd_interact_oob proof failed");
  }

  const iggy3d::SessionCommandResult moveToKey =
      session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  if (!commandAccepted(moveToKey, 2, 2) || session.state().transient.pendingExecutionSequences.size() != 1U ||
      !runQueuedCommand(session)) {
    return fail("cmd_move_to_key proof failed");
  }
  const iggy3d::EntityState* player = session.state().world.findByStableName("player");
  if (player == nullptr ||
      !iggy3d::nearlyEqual(player->transform.position, iggy3d::Vec3{2.0F, 0.0F, 0.0F})) {
    return fail("move result proof failed");
  }

  const iggy3d::SessionCommandResult retry = session.submitCommand(submittedRetry(1));
  if (!commandAccepted(retry, 3, 3) || retry.command.payload.retrySourceCommandId != 1U ||
      session.state().transient.pendingExecutionSequences.size() != 1U || !runQueuedCommand(session)) {
    return fail("cmd_retry_key proof failed");
  }
  const iggy3d::EntityState* key = session.state().world.findByStableName("gold_key");
  if (key == nullptr || key->active || itemCount(session.state().inventory, 0, "gold_key") != 1U ||
      !iggy3d::objectiveComplete(session.state().objectives, "collect_gold_key") ||
      session.state().outcome != iggy3d::SessionOutcome::DemoComplete ||
      session.state().lifecycle != iggy3d::SessionLifecycle::Playing) {
    return fail("pickup proof failed");
  }

  const iggy3d::SessionCommandResult enterTactical =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  if (!commandAccepted(enterTactical, 4, 4) || !enterTactical.executedImmediately ||
      session.state().clock.mode != iggy3d::ClockMode::Slow ||
      session.state().camera.activeMode != iggy3d::CameraMode::TacticalOverhead) {
    return fail("cmd_enter_tactical proof failed");
  }

  const iggy3d::SessionCommandResult tacticalMove =
      session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  if (!commandAccepted(tacticalMove, 5, 5) || !runQueuedCommand(session)) {
    return fail("cmd_tactical_move proof failed");
  }
  player = session.state().world.findByStableName("player");
  if (player == nullptr ||
      !iggy3d::nearlyEqual(player->transform.position, iggy3d::Vec3{2.0F, 0.0F, 1.0F})) {
    return fail("tactical move result proof failed");
  }

  const iggy3d::SessionCommandResult paused =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Pause));
  if (!commandAccepted(paused, 6, 6) || !paused.executedImmediately ||
      session.state().clock.mode != iggy3d::ClockMode::Paused ||
      session.state().camera.activeMode != iggy3d::CameraMode::TacticalOverhead) {
    return fail("cmd_pause proof failed");
  }

  const iggy3d::CommandTick tickBeforeStep = session.state().clock.tickIndex;
  const iggy3d::SessionCommandResult step =
      session.submitCommand(submittedControl(iggy3d::CommandKind::StepTacticalTick));
  if (!commandAccepted(step, 7, 7) || !step.executedImmediately ||
      session.state().clock.mode != iggy3d::ClockMode::Paused ||
      session.state().clock.stepRequested ||
      session.state().clock.tickIndex != tickBeforeStep + 1U) {
    return fail("cmd_step proof failed");
  }

  const iggy3d::SessionCommandResult resumed =
      session.submitCommand(submittedControl(iggy3d::CommandKind::Resume));
  if (!commandAccepted(resumed, 8, 8) || !resumed.executedImmediately ||
      session.state().clock.mode != iggy3d::ClockMode::Slow ||
      session.state().camera.activeMode != iggy3d::CameraMode::TacticalOverhead) {
    return fail("cmd_resume proof failed");
  }

  const iggy3d::SessionCommandResult attack = session.submitCommand(submittedAttack());
  if (!commandAccepted(attack, 9, 9) || attack.command.payload.attackDamage != 3 ||
      !runQueuedCommand(session)) {
    return fail("cmd_attack_dummy proof failed");
  }
  const iggy3d::EntityState* dummy = session.state().world.findByStableName("training_dummy");
  bool dummyDefeated = false;
  for (const iggy3d::CombatantState& combatant : session.state().combat.combatants) {
    if (dummy != nullptr && combatant.entity == dummy->id && combatant.hitPoints == 0 &&
        combatant.defeated) {
      dummyDefeated = true;
    }
  }
  if (dummy == nullptr || !dummy->active || !dummyDefeated) {
    return fail("combat result proof failed");
  }

  const iggy3d::SessionCommandResult exitTactical =
      session.submitCommand(submittedControl(iggy3d::CommandKind::ToggleTacticalMode));
  if (!commandAccepted(exitTactical, 10, 10) || !exitTactical.executedImmediately ||
      session.state().clock.mode != iggy3d::ClockMode::Normal ||
      session.state().camera.activeMode != iggy3d::CameraMode::ThirdPerson) {
    return fail("cmd_exit_tactical proof failed");
  }

  const iggy3d::SessionFinalizationResult finalized = session.finalizeDemoIfComplete();
  if (finalized.status != iggy3d::SessionFinalizationStatus::Completed ||
      session.state().lifecycle != iggy3d::SessionLifecycle::Complete) {
    return fail("finalization proof failed");
  }

  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  if (saved.status != iggy3d::SaveLoadStatus::Ok) {
    return fail("save failed");
  }

  iggy3d::Result<iggy3d::Session> freshCreated = iggy3d::Session::create(create);
  if (freshCreated.status != iggy3d::ResultStatus::Ok) {
    return fail("fresh session create failed");
  }
  iggy3d::Session loaded = std::move(freshCreated.value);
  const iggy3d::SaveCompatibilityRequest compatibility{
      saved.envelope, session.state().identity.packageId, session.state().identity.scenarioId};
  const iggy3d::LoadStateResult loadedResult =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibility);
  const bool roundtripOk =
      loadedResult.status == iggy3d::SaveLoadStatus::Ok && durableFactsMatch(session.state(), loaded.state());
  const bool resetOk = resetProof(create);
  if (!config.savePath.empty() && !writeTextFile(config.savePath, saved.encodedSaveText)) {
    return fail("save file write failed");
  }

  const iggy3d::CommandLogCounts counts = loaded.state().commandLog.counts();
  player = loaded.state().world.findByStableName("player");
  key = loaded.state().world.findByStableName("gold_key");
  if (player == nullptr || key == nullptr ||
      !iggy3d::nearlyEqual(player->transform.position, iggy3d::Vec3{2.0F, 0.0F, 1.0F}) ||
      key->active || counts.submitted != 10U || counts.accepted != 9U ||
      counts.rejected != 1U || counts.retry != 1U || counts.combat != 1U ||
      counts.control != 5U ||
      counts.movement != 2U || loaded.state().clock.mode != iggy3d::ClockMode::Normal ||
      loaded.state().camera.activeMode != iggy3d::CameraMode::ThirdPerson) {
    return fail("final durable proof failed");
  }

  std::string expectedSummary;
  if (!config.expectedSummaryPath.empty() && !readTextFile(config.expectedSummaryPath, expectedSummary)) {
    return fail("expected summary read failed");
  }

  iggy3d::CommandReplayRequest replayRequest;
  replayRequest.baseline = create;
  replayRequest.sourceCommands = session.state().commandLog.records();
  replayRequest.expectedFinalHash = session.stateHash();
  replayRequest.expectedSummaryText = expectedSummary;
  replayRequest.saveRoundtrip =
      roundtripOk ? iggy3d::RuntimeProofStatus::Pass : iggy3d::RuntimeProofStatus::Fail;
  replayRequest.resetBaseline =
      resetOk ? iggy3d::RuntimeProofStatus::Pass : iggy3d::RuntimeProofStatus::Fail;
  replayRequest.replayHash = iggy3d::RuntimeProofStatus::Pass;
  replayRequest.retryExecutedCommandId = retry.command.commandId;
  replayRequest.retryExecutedSequence = retry.command.sequence;
  const iggy3d::CommandReplayResult replay = iggy3d::replayCommands(replayRequest);
  const bool replayOk = replay.status == iggy3d::CommandReplayStatus::Matched;

  iggy3d::RuntimeSummaryInput summaryInput;
  summaryInput.state = &loaded.state();
  summaryInput.retryExecutedCommandId = retry.command.commandId;
  summaryInput.retryExecutedSequence = retry.command.sequence;
  summaryInput.saveRoundtrip =
      roundtripOk ? iggy3d::RuntimeProofStatus::Pass : iggy3d::RuntimeProofStatus::Fail;
  summaryInput.resetBaseline =
      resetOk ? iggy3d::RuntimeProofStatus::Pass : iggy3d::RuntimeProofStatus::Fail;
  summaryInput.replayHash =
      replayOk ? iggy3d::RuntimeProofStatus::Pass : iggy3d::RuntimeProofStatus::Fail;

  DemoRunResult result;
  result.ok = roundtripOk && resetOk && replayOk;
  result.summary = iggy3d::formatRuntimeSummary(iggy3d::buildRuntimeSummary(summaryInput));
  if (!expectedSummary.empty() && result.summary != expectedSummary) {
    result.ok = false;
    result.diagnostic = "expected summary mismatch";
    return result;
  }
  if (!roundtripOk) {
    result.diagnostic = "save roundtrip proof failed";
  } else if (!resetOk) {
    result.diagnostic = "reset proof failed";
  } else if (!replayOk) {
    result.diagnostic = "replay proof failed";
  }
  return result;
}

void printDiagnostics(const iggy3d::CliParseResult& parsed) {
  for (const iggy3d::Diagnostic& diagnostic : parsed.diagnostics) {
    std::cerr << diagnostic.code << "=" << diagnostic.message << '\n';
  }
}

}  // namespace

int main(int argc, const char* const* argv) {
  const iggy3d::CliParseResult parsed = iggy3d::parseCommandLine(argc, argv);
  if (parsed.status != iggy3d::AppConfigStatus::Ok) {
    printDiagnostics(parsed);
    return 2;
  }
  if (parsed.config.printHelp) {
    std::cout << iggy3d::cliHelpText() << '\n';
    return 0;
  }
  if (parsed.config.printVersion) {
    std::cout << iggy3d::cliVersionText() << '\n';
    return 0;
  }
  if (parsed.config.mode != iggy3d::AppMode::HeadlessDemo) {
    std::cerr << "app.mode=unsupported\n";
    return 2;
  }

  const DemoRunResult result = runDemo(parsed.config);
  if (!result.ok) {
    if (parsed.config.verbose) {
      std::cerr << "demo.error=" << result.diagnostic << '\n';
    }
    return 1;
  }
  std::cout << result.summary;
  return 0;
}
