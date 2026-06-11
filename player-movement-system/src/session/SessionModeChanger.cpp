#include "SessionModeChanger.hpp"

#include "session/SessionModePolicy.hpp"

namespace dev {

bool SessionModeChanger::change(GameSessionMode &current, GameSessionMode requested) const
{
	if (!SessionModePolicy {}.canTransition(current, requested))
		return false;

	current = requested;
	return true;
}

} // namespace dev
