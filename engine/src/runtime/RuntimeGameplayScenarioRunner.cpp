#include "runtime/RuntimeGameplayScenarioRunner.hpp"

#include "runtime/RuntimeGameplayScenarioReportProjection.hpp"

namespace iggy::runtime {
namespace {

std::vector<RuntimeGameplayOrchestratedFrameRunnerFrame> FramesFromScenario(
	const std::vector<RuntimeGameplayScenarioFrame> &frames)
{
	std::vector<RuntimeGameplayOrchestratedFrameRunnerFrame> runnerFrames;
	runnerFrames.reserve(frames.size());
	for (const RuntimeGameplayScenarioFrame &frame : frames) {
		runnerFrames.push_back(frame.frame);
	}
	return runnerFrames;
}

} // namespace

bool RuntimeGameplayScenarioResult::hasFrames() const
{
	return frameCount > 0;
}

bool RuntimeGameplayScenarioResult::changed() const
{
	return changedFrameCount > 0;
}

bool RuntimeGameplayScenarioResult::refreshedNpcData() const
{
	return npcOccupancyRefreshed
		|| npcInteractionRefreshed
		|| npcAiMapRefreshed
		|| npcRenderRefreshed
		|| npcVisibilityRefreshed;
}

RuntimeGameplayScenarioResult RuntimeGameplayScenarioRunner::run(const RuntimeGameplayScenario &scenario) const
{
	RuntimeGameplayScenarioResult result;
	result.scenario = scenario;

	RuntimeGameplayOrchestratedFrameRunnerInput runnerInput;
	runnerInput.initialState = scenario.initialState;
	runnerInput.frames = FramesFromScenario(scenario.frames);

	result.runner = RuntimeGameplayOrchestratedFrameRunner {}.run(runnerInput);
	result.report = RuntimeGameplayOrchestratedFrameRunnerReporter {}.report(result.runner);
	result.state = result.runner.state;
	applyRuntimeGameplayScenarioResultProjection(result, result.report);
	return result;
}

} // namespace iggy::runtime
