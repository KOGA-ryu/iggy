#pragma once

#include "input/PlayerIntent.hpp"
#include "input/RawInput.hpp"
#include "focus/InputFocus.hpp"
#include "world/TileMap.hpp"

namespace dev {

class InputMapper {
public:
	PlayerIntent mapToIntent(const RawInputEvent &event, const TileMap &map, const InputFocus &focus) const;
};

} // namespace dev
