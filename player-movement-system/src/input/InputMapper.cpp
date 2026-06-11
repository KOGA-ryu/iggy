#include "InputMapper.hpp"

namespace dev {

PlayerIntent InputMapper::mapToIntent(const RawInputEvent &event, const TileMap &map, const InputFocus &focus) const
{
	if (!focus.gameplayOwnsMovement())
		return {};

	if (event.type == RawInputType::MouseClick && event.pressed) {
		return {
			.type = PlayerIntentType::MoveTo,
			.destination = map.screenToTile(event.screenPosition),
		};
	}

	return {};
}

} // namespace dev

