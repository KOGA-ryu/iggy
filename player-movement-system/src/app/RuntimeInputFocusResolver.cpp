#include "RuntimeInputFocusResolver.hpp"

namespace dev {

FocusState RuntimeInputFocusResolver::resolve(FocusState focusState, GameSessionMode sessionMode) const
{
	if (sessionMode == GameSessionMode::Paused)
		focusState.owner = InputOwner::Menu;
	else if (sessionMode == GameSessionMode::Inventory)
		focusState.owner = InputOwner::Inventory;
	return focusState;
}

} // namespace dev
