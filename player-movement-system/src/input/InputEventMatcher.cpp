#include "InputEventMatcher.hpp"

namespace dev {

bool InputEventMatcher::pressedKey(const RawInputEvent &event, int code) const
{
	return event.type == RawInputType::KeyPress && event.pressed && event.code == code;
}

bool InputEventMatcher::pressedPointer(const RawInputEvent &event) const
{
	return (event.type == RawInputType::MouseClick || event.type == RawInputType::TouchTap) && event.pressed;
}

} // namespace dev
