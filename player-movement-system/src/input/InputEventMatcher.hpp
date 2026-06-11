#pragma once

#include "input/RawInput.hpp"

namespace dev {

class InputEventMatcher {
public:
	[[nodiscard]] bool pressedKey(const RawInputEvent &event, int code) const;
	[[nodiscard]] bool pressedPointer(const RawInputEvent &event) const;
};

} // namespace dev
