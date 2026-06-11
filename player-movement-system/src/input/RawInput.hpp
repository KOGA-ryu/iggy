#pragma once

#include "world/Point.hpp"

namespace dev {

enum class RawInputType {
	MouseClick,
	KeyPress,
	ControllerButton,
	ControllerAxis,
	TouchTap,
};

struct RawInputEvent {
	RawInputType type;
	Point screenPosition;
	int code = 0;
	bool pressed = false;
};

} // namespace dev

