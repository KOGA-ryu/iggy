#pragma once

#include "focus/InputFocus.hpp"
#include "session/GameSessionMode.hpp"

namespace dev {

class RuntimeInputFocusResolver {
public:
	[[nodiscard]] FocusState resolve(FocusState focusState, GameSessionMode sessionMode) const;
};

} // namespace dev
