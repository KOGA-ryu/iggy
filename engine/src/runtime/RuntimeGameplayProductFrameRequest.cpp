#include "runtime/RuntimeGameplayProductFrameRequest.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayProductFrameRequestStatus MapStatus(
	RuntimeGameplayProductPlayModeFrameStatus status)
{
	switch (status) {
	case RuntimeGameplayProductPlayModeFrameStatus::Stepped:
		return RuntimeGameplayProductFrameRequestStatus::Stepped;
	case RuntimeGameplayProductPlayModeFrameStatus::NoFrameAvailable:
		return RuntimeGameplayProductFrameRequestStatus::NoFrameAvailable;
	case RuntimeGameplayProductPlayModeFrameStatus::NotLoaded:
		return RuntimeGameplayProductFrameRequestStatus::NotLoaded;
	}
	return RuntimeGameplayProductFrameRequestStatus::NotLoaded;
}

} // namespace

RuntimeGameplayProductFrameRequestResult RuntimeGameplayProductFrameRequest::run(
	const RuntimeGameplayProductFrameRequestInput &input) const
{
	RuntimeGameplayProductFrameRequestResult result;
	result.inputEventCount = input.inputFrame.events.size();
	result.presentationCamera =
		RuntimeGameplayProductPresentationCamera {}.build(
			input.state,
			input.presentationCamera);

	RuntimeGameplayProductPlayModeFrameInput frameInput;
	frameInput.state = input.state;
	frameInput.inputFrame = input.inputFrame;
	frameInput.presentationCamera = result.presentationCamera.presentationCamera;
	frameInput.levelRenderConfig = result.presentationCamera.levelRenderConfig;

	result.frame = RuntimeGameplayProductPlayMode {}.frame(frameInput);
	result.status = MapStatus(result.frame.status);
	result.state = result.frame.state;
	result.ignoredInputEventCount =
		result.frame.surface.ignoredInputEventCount;
	return result;
}

} // namespace iggy::runtime
