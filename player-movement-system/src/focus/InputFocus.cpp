#include "InputFocus.hpp"

namespace dev {

InputFocus::InputFocus(const FocusState &state)
    : state_(state)
{
}

bool InputFocus::gameplayOwnsMovement() const
{
	return state_.owner == InputOwner::Gameplay && !state_.textEntryActive;
}

bool InputFocus::gameplayOwnsActions() const
{
	return state_.owner == InputOwner::Gameplay && !state_.textEntryActive;
}

} // namespace dev

