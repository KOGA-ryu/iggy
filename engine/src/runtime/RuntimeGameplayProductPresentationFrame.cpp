#include "runtime/RuntimeGameplayProductPresentationFrame.hpp"

namespace iggy::runtime {

RuntimeGameplayProductPresentationFrameResult
RuntimeGameplayProductPresentationFrame::build(
	const RuntimeGameplayProductPresentationFrameInput &input) const
{
	RuntimeGameplayProductPresentationFrameResult result;
	result.presentationCamera = input.presentationCamera;

	if (!input.state.loaded) {
		result.status = RuntimeGameplayProductPresentationFrameStatus::NotLoaded;
		return result;
	}

	result.levelFrame = LevelRenderFrame2D {}.build(
		input.state.currentState.session.level,
		input.presentationCamera,
		input.levelRenderConfig);
	result.status = RuntimeGameplayProductPresentationFrameStatus::Rendered;
	return result;
}

} // namespace iggy::runtime
