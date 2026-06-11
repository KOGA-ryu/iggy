#include "RuntimeMovementInputContextBuilder.hpp"

namespace dev {

std::optional<RuntimeMovementInputContext> RuntimeMovementInputContextBuilder::build(const RuntimeInputContext &context) const
{
	if (context.world == nullptr || context.playerId >= context.world->players.size())
		return std::nullopt;

	FocusState focusState = focusResolver_.resolve(context.focusState, context.sessionMode);
	InputFocus focus { focusState };
	const Player &player = context.world->players[context.playerId];
	const PlayerActionGate gate { focus, context.actionContext };
	return RuntimeMovementInputContext {
		.world = context.world,
		.player = &player,
		.focusState = focusState,
		.actionContext = context.actionContext,
		.blockReason = gate.movementBlockReason(player),
	};
}

} // namespace dev
