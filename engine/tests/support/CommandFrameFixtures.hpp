#pragma once

#include <initializer_list>

#include "runtime/GameplayCommand2D.hpp"

namespace iggy::test {

inline runtime::GameplayCommandFrame2D CommandFrame(std::initializer_list<runtime::GameplayCommand2D> commands)
{
	runtime::GameplayCommandFrame2D frame;
	frame.commands.insert(frame.commands.end(), commands.begin(), commands.end());
	return frame;
}

} // namespace iggy::test
