#include "runtime/RuntimeGameplayProductPresentationFrame.hpp"

#include "servers/render/RenderCommandList2DComposer.hpp"

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
	result.actorCommands = RuntimeGameplayProductActorRenderCommands {}.build(
		input.state.currentState,
		input.actorRenderConfig);
	render::RenderCommandList2DComposer {}.append(
		result.levelFrame.commands,
		result.actorCommands.commands);
	result.status = RuntimeGameplayProductPresentationFrameStatus::Rendered;
	return result;
}

} // namespace iggy::runtime
