#include "InputMapper.hpp"

#include "input/InputEventMatcher.hpp"

namespace dev {

PlayerIntent InputMapper::mapToIntent(const RawInputEvent &event, const TileMap &map, const InputFocus &focus) const
{
	if (!focus.gameplayOwnsMovement())
		return {};

	if (InputEventMatcher {}.pressedPointer(event)) {
		return {
			.type = PlayerIntentType::MoveTo,
			.destination = map.screenToTile(event.screenPosition),
		};
	}

	return {};
}

} // namespace dev
