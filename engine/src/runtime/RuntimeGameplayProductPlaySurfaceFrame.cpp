#include "runtime/RuntimeGameplayProductPlaySurfaceFrame.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayProductPresentationFrameResult BuildPresentation(
	const RuntimeGameplayProductLoopState &state,
	CameraState camera,
	const LevelRenderFrame2DConfig &config)
{
	RuntimeGameplayProductPresentationFrameInput input;
	input.state = state;
	input.presentationCamera = camera;
	input.levelRenderConfig = config;
	return RuntimeGameplayProductPresentationFrame {}.build(input);
}

RuntimeGameplayProductLoopStepResult NoFrameStep(
	const RuntimeGameplayProductLoopState &state)
{
	RuntimeGameplayProductLoopStepResult result;
	result.status = RuntimeGameplayProductLoopStepStatus::NoFrameAvailable;
	result.state = state;
	result.frameIndex = state.nextFrameIndex;
	return result;
}

RuntimeGameplayProductLoopStepInput StepInput(
	RuntimeGameplayProductLoopState state,
	const PlayerInputBinding2DResult &binding)
{
	RuntimeGameplayProductLoopStepInput input;
	input.state = state;
	input.playerIntents = binding.intents;
	input.hasPlayerInputContextOverride = true;
	input.playerInputContextOverride = binding.inputContext;
	return input;
}

RuntimeGameplayProductInputFrame2D FocusedInputFrame(
	const RuntimeGameplayProductPlaySurfaceFrameInput &input)
{
	if (input.hasInputFocus)
		return input.inputFrame;

	RuntimeGameplayProductInputFrame2D frame;
	frame.bindingContext = input.inputFrame.bindingContext;
	return frame;
}

} // namespace

RuntimeGameplayProductPlaySurfaceFrameResult
RuntimeGameplayProductPlaySurfaceFrame::build(
	const RuntimeGameplayProductPlaySurfaceFrameInput &input) const
{
	RuntimeGameplayProductPlaySurfaceFrameResult result;

	if (!input.state.loaded) {
		result.status = RuntimeGameplayProductPlaySurfaceFrameStatus::NotLoaded;
		result.ignoredInputEventCount = input.inputFrame.events.size();
		result.presentation = BuildPresentation(
			input.state,
			input.presentationCamera,
			input.levelRenderConfig);
		return result;
	}

	if (input.state.nextFrameIndex >= input.state.scenario.frames.size()) {
		result.status =
			RuntimeGameplayProductPlaySurfaceFrameStatus::NoFrameAvailable;
		result.ignoredInputEventCount = input.inputFrame.events.size();
		result.step = NoFrameStep(input.state);
		result.presentation = BuildPresentation(
			input.state,
			input.presentationCamera,
			input.levelRenderConfig);
		return result;
	}

	const RuntimeGameplayProductInputFrame2D adapterInput =
		FocusedInputFrame(input);
	if (!input.hasInputFocus)
		result.ignoredInputEventCount = input.inputFrame.events.size();

	result.inputAdapter =
		RuntimeGameplayProductInputAdapter {}.map(adapterInput);
	result.playerBinding = PlayerInputBinding2D {}.bind(
		result.inputAdapter.bindingContext,
		result.inputAdapter.actions);
	result.step = RuntimeGameplayProductLoop {}.step(
		StepInput(input.state, result.playerBinding));
	result.status = result.step.status ==
			RuntimeGameplayProductLoopStepStatus::Stepped
		? RuntimeGameplayProductPlaySurfaceFrameStatus::Stepped
		: RuntimeGameplayProductPlaySurfaceFrameStatus::NoFrameAvailable;
	result.presentation = BuildPresentation(
		result.step.state,
		input.presentationCamera,
		input.levelRenderConfig);
	return result;
}

} // namespace iggy::runtime
