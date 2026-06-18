#include "runtime/RuntimeGameplayProductInputContext.hpp"

#include "scene/player/PlayerAgentState.hpp"

namespace iggy::runtime {

RuntimeGameplayProductInputContextResult RuntimeGameplayProductInputContext::build(
	const RuntimeGameplayProductPlayModeState &state) const
{
	RuntimeGameplayProductInputContextResult result;
	if (!state.loop.loaded)
		return result;

	if (!state.loop.currentState.session.hasPlayer) {
		result.status =
			RuntimeGameplayProductInputContextStatus::LoadedWithoutPlayer;
		return result;
	}

	result.status = RuntimeGameplayProductInputContextStatus::Projected;
	result.bindingContext.hasCurrentPlayerTile = true;
	result.bindingContext.currentPlayerTile =
		playerTile(state.loop.currentState.session.player);
	return result;
}

} // namespace iggy::runtime
