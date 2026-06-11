#include "RuntimeSourceContext.hpp"

namespace dev {

RuntimeSourceContext::RuntimeSourceContext(GameSession &session, PlayerId inputPlayerId)
    : session_(session)
    , inputPlayerId_(inputPlayerId)
{
}

bool RuntimeSourceContext::hasActiveWorld() const
{
	return session_.hasActiveWorld();
}

bool RuntimeSourceContext::hasActivePlayer() const
{
	return hasActiveWorld() && inputPlayerId_ < session_.world().players.size();
}

SimulationWorld &RuntimeSourceContext::world() const
{
	return session_.world();
}

Player &RuntimeSourceContext::player() const
{
	return session_.world().players[inputPlayerId_];
}

} // namespace dev
