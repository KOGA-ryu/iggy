#include "runtime/RuntimeGameplayProductPlayMode.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayProductPlayModeFrameStatus MapFrameStatus(
	RuntimeGameplayProductPlaySurfaceFrameStatus status)
{
	switch (status) {
	case RuntimeGameplayProductPlaySurfaceFrameStatus::Stepped:
		return RuntimeGameplayProductPlayModeFrameStatus::Stepped;
	case RuntimeGameplayProductPlaySurfaceFrameStatus::NoFrameAvailable:
		return RuntimeGameplayProductPlayModeFrameStatus::NoFrameAvailable;
	case RuntimeGameplayProductPlaySurfaceFrameStatus::NotLoaded:
		return RuntimeGameplayProductPlayModeFrameStatus::NotLoaded;
	}
	return RuntimeGameplayProductPlayModeFrameStatus::NotLoaded;
}

} // namespace

RuntimeGameplayProductPlayModeBuildResult RuntimeGameplayProductPlayMode::build(
	const RuntimeGameplayProductLoopBuildResult &loop) const
{
	RuntimeGameplayProductPlayModeBuildResult result;
	result.loop = loop;

	if (loop.status != RuntimeGameplayProductLoopStatus::Ready)
		return result;

	result.status = RuntimeGameplayProductPlayModeBuildStatus::Ready;
	result.state.loop = loop.state;
	result.state.hasInputFocus = true;
	return result;
}

RuntimeGameplayProductPlayModeState RuntimeGameplayProductPlayMode::withInputFocus(
	RuntimeGameplayProductPlayModeState state,
	bool hasInputFocus) const
{
	state.hasInputFocus = hasInputFocus;
	return state;
}

RuntimeGameplayProductPlayModeFrameResult RuntimeGameplayProductPlayMode::frame(
	const RuntimeGameplayProductPlayModeFrameInput &input) const
{
	RuntimeGameplayProductPlayModeFrameResult result;
	result.state = input.state;

	RuntimeGameplayProductPlaySurfaceFrameInput surfaceInput;
	surfaceInput.state = input.state.loop;
	surfaceInput.inputFrame = input.inputFrame;
	surfaceInput.hasInputFocus = input.state.hasInputFocus;
	surfaceInput.presentationCamera = input.presentationCamera;
	surfaceInput.levelRenderConfig = input.levelRenderConfig;

	result.surface =
		RuntimeGameplayProductPlaySurfaceFrame {}.build(surfaceInput);
	result.status = MapFrameStatus(result.surface.status);

	if (result.status == RuntimeGameplayProductPlayModeFrameStatus::Stepped)
		result.state.loop = result.surface.step.state;
	return result;
}

} // namespace iggy::runtime
