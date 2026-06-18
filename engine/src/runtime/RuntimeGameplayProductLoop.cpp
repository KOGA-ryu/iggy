#include "runtime/RuntimeGameplayProductLoop.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayOrchestratedFrameInput InputFromFrame(
	RuntimeGameplayOrchestratedFrameRunnerFrame frame,
	const RuntimeGameplayState &state,
	const std::vector<PlayerInputIntent2D> &playerIntents)
{
	RuntimeGameplayOrchestratedFrameInput input;
	frame.playerFrame.state = state;
	frame.playerFrame.playerIntents = playerIntents;
	input.playerFrame = frame.playerFrame;
	input.subjects = frame.subjects;
	input.pools = frame.pools;
	input.aiMap = frame.aiMap;
	input.controlConfig = frame.controlConfig;
	input.controlOverrides = frame.controlOverrides;
	input.movementMap = frame.movementMap;
	input.movementConfig = frame.movementConfig;
	input.previousOccupancy = frame.previousOccupancy;
	input.interactionTargets = frame.interactionTargets;
	input.refreshAiMap = frame.refreshAiMap;
	input.refreshConfig = frame.refreshConfig;
	return input;
}

RuntimeGameplayProductLoopStepResult StepWithFrame(
	const RuntimeGameplayProductLoopStepInput &input,
	const RuntimeGameplayOrchestratedFrameRunnerFrame &frame)
{
	RuntimeGameplayProductLoopStepResult result;
	result.frameIndex = input.state.nextFrameIndex;
	result.frame = RuntimeGameplayOrchestratedFrameStep {}.run(
		InputFromFrame(frame, input.state.currentState, input.playerIntents));
	result.state = input.state;
	result.state.currentState = result.frame.state;
	++result.state.nextFrameIndex;
	result.status = RuntimeGameplayProductLoopStepStatus::Stepped;
	return result;
}

RuntimeGameplayProductLoopStepResult StepWithFrame(
	const RuntimeGameplayProductLoopStepInput &input,
	const RuntimeGameplayOrchestratedFrameRunnerFrame &frame,
	const physics2d::CollisionWorld2D &explicitWorld)
{
	RuntimeGameplayProductLoopStepResult result;
	result.frameIndex = input.state.nextFrameIndex;
	result.frame = RuntimeGameplayOrchestratedFrameStep {}.run(
		InputFromFrame(frame, input.state.currentState, input.playerIntents),
		explicitWorld);
	result.state = input.state;
	result.state.currentState = result.frame.state;
	++result.state.nextFrameIndex;
	result.status = RuntimeGameplayProductLoopStepStatus::Stepped;
	return result;
}

RuntimeGameplayProductLoopStepResult InitialStepResult(
	const RuntimeGameplayProductLoopStepInput &input)
{
	RuntimeGameplayProductLoopStepResult result;
	result.state = input.state;
	result.frameIndex = input.state.nextFrameIndex;
	return result;
}

bool StepUnavailable(RuntimeGameplayProductLoopStepResult &result)
{
	if (!result.state.loaded) {
		result.status = RuntimeGameplayProductLoopStepStatus::NotLoaded;
		return true;
	}
	if (result.state.nextFrameIndex >= result.state.scenario.frames.size()) {
		result.status = RuntimeGameplayProductLoopStepStatus::NoFrameAvailable;
		return true;
	}
	return false;
}

} // namespace

RuntimeGameplayProductLoopBuildResult RuntimeGameplayProductLoop::build(
	const RuntimeGameplayProductScenarioLoadResult &load) const
{
	RuntimeGameplayProductLoopBuildResult result;
	result.load = load;
	if (!load.ok()) {
		result.status = RuntimeGameplayProductLoopStatus::LoadFailed;
		return result;
	}

	result.state.loaded = true;
	result.state.inputPath = load.inputPath;
	result.state.sourcePath = load.sourcePath;
	result.state.hasPackage = load.hasPackage;
	result.state.packageRoot = load.packageRoot;
	result.state.manifestPath = load.manifestPath;
	result.state.mainScenarioPath = load.mainScenarioPath;
	result.state.packageManifest = load.packageManifest;
	result.state.definition = load.definition;
	result.state.scenario = load.validation.build.scenario;
	result.state.initialState = load.initialState;
	result.state.currentState = load.initialState;
	result.state.nextFrameIndex = 0;
	result.status = RuntimeGameplayProductLoopStatus::Ready;
	return result;
}

RuntimeGameplayProductLoopStepResult RuntimeGameplayProductLoop::step(
	const RuntimeGameplayProductLoopStepInput &input) const
{
	RuntimeGameplayProductLoopStepResult guarded = InitialStepResult(input);
	if (StepUnavailable(guarded))
		return guarded;
	return StepWithFrame(
		input,
		input.state.scenario.frames[input.state.nextFrameIndex].frame);
}

RuntimeGameplayProductLoopStepResult RuntimeGameplayProductLoop::step(
	const RuntimeGameplayProductLoopStepInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimeGameplayProductLoopStepResult guarded = InitialStepResult(input);
	if (StepUnavailable(guarded))
		return guarded;
	return StepWithFrame(
		input,
		input.state.scenario.frames[input.state.nextFrameIndex].frame,
		explicitWorld);
}

} // namespace iggy::runtime
