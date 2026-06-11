#include "RuntimeInputContextBuilder.hpp"

namespace dev {

RuntimeInputContextBuilder::RuntimeInputContextBuilder(const GameSession &session)
    : session_(session)
{
}

RuntimeInputContext RuntimeInputContextBuilder::build(const RuntimeInputSettings &settings) const
{
	return {
		.world = session_.hasActiveWorld() ? &session_.world() : nullptr,
		.playerId = settings.playerId,
		.focusState = settings.focusState,
		.actionContext = settings.actionContext,
		.sessionMode = session_.mode(),
		.targetResolver = settings.targetResolver != nullptr
		    ? settings.targetResolver
		    : (session_.hasActiveWorld() ? &session_.world().targets : nullptr),
	};
}

} // namespace dev
