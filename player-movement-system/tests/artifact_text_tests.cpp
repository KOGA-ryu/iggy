#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "app/RuntimeFramePolicyText.hpp"
#include "app/RuntimeFrameTraceHeaderText.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunSummaryText.hpp"
#include "app/RuntimeRunTraceFrameHeaderText.hpp"
#include "session/GameSessionMode.hpp"
#include "simulation/SimulationFramePolicyDescriber.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestRuntimeFramePolicyTextFormatsArtifactPolicyLines()
{
	const dev::SimulationFramePolicyDescription gameplay = dev::SimulationFramePolicyDescriber {}.describe(dev::SimulationMode::Gameplay);
	dev::RuntimeFramePolicyText formatter;

	const std::string traceLine = formatter.format("policy mode", gameplay, dev::RuntimeFramePolicyBoolStyle::Numeric);
	const std::string manifestLine = formatter.format("policy latest", gameplay, dev::RuntimeFramePolicyBoolStyle::Words);
	const std::string noneLine = formatter.formatNone("policy latest");

	Expect(traceLine == "policy mode=Gameplay acceptCommands=1 updatePlayers=1 updateEnemies=1 reason=accept live input and advance all actors", "runtime frame policy text should format trace policy line");
	Expect(manifestLine == "policy latest=Gameplay acceptCommands=true updatePlayers=true updateEnemies=true reason=accept live input and advance all actors", "runtime frame policy text should format manifest policy line");
	Expect(noneLine == "policy latest=none", "runtime frame policy text should format missing policy line");
}

void TestRuntimeFrameTraceHeaderTextFormatsFrameCounts()
{
	dev::RuntimeFrameReport report;
	report.rawInputEventsRouted = 2;
	report.sessionCommandResults.push_back({});
	report.inventoryScriptResults.push_back({});
	report.inventoryCommandResults.push_back({});
	report.movementScriptResults.push_back({});
	report.movementCommandsQueued = 3;
	report.frameEvents.emit(dev::MovementEvent { .type = dev::MovementEventType::StepCommitted });
	report.frameEvents.emit(dev::CombatEvent { .type = dev::CombatEventType::Hit });
	report.frameEvents.emit(dev::EffectRequest { .type = dev::EffectRequestType::Footstep });
	report.sessionEvents.push_back({});
	report.inventoryEvents.push_back({});

	dev::RuntimeFrameTraceHeaderText formatter;

	Expect(formatter.format(report) == "frame rawInput=2 movementInputBlocks=0 sessionResults=1 inventoryScripts=1 inventoryResults=1 movementScripts=1 movementQueued=3 movementEvents=1 combatEvents=1 effects=1 sessionEvents=1 inventoryEvents=1", "runtime frame trace header text should format all frame counts");
}

void TestRuntimeRunTraceFrameHeaderTextFormatsFrameIndex()
{
	dev::RuntimeRunTraceFrameHeaderText formatter;

	Expect(formatter.format(0) == "frame[0]", "runtime run trace frame header text should format first frame index");
	Expect(formatter.format(12) == "frame[12]", "runtime run trace frame header text should format later frame indexes");
}

void TestRuntimeRunSummaryTextFormatsTraceAndManifestSummaries()
{
	dev::GameLoopResult result;
	result.summary.framesRun = 2;
	result.summary.rawInputEventsRouted = 3;
	result.summary.movementInputBlockReasons.push_back(dev::PlayerActionBlockReason::Focus);
	result.summary.sessionCommandResults.push_back({});
	result.summary.runtimeInventoryScriptResults.push_back({});
	result.summary.inventoryCommandResults.push_back({});
	result.summary.movementCommandsQueued = 4;
	result.frameReports.push_back({});
	result.finalMode = dev::GameSessionMode::Inventory;

	dev::RuntimeRunSummaryText formatter;

	Expect(formatter.format(result, dev::RuntimeRunSummaryDetail::CountsOnly) == "run frames=2 frameReports=1 rawInput=3 movementInputBlocks=1 sessionResults=1 inventoryScripts=1 inventoryResults=1 movementScripts=0 movementQueued=4", "runtime run summary text should format trace run summary");
	Expect(formatter.format(result, dev::RuntimeRunSummaryDetail::WithFinalMode) == "run frames=2 frameReports=1 rawInput=3 movementInputBlocks=1 sessionResults=1 inventoryScripts=1 inventoryResults=1 movementScripts=0 movementQueued=4 finalMode=Inventory", "runtime run summary text should format manifest run summary");
}

} // namespace

int main()
{
	TestRuntimeFramePolicyTextFormatsArtifactPolicyLines();
	TestRuntimeFrameTraceHeaderTextFormatsFrameCounts();
	TestRuntimeRunTraceFrameHeaderTextFormatsFrameIndex();
	TestRuntimeRunSummaryTextFormatsTraceAndManifestSummaries();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
