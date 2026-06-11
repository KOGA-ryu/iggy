#pragma once

#include "session/GameSessionMode.hpp"

namespace dev {

class RuntimeSessionModeTogglePolicy {
public:
	[[nodiscard]] GameSessionMode togglePause(GameSessionMode current) const;
	[[nodiscard]] GameSessionMode toggleInventory(GameSessionMode current) const;
};

} // namespace dev
