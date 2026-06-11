#pragma once

#include "session/GameSessionMode.hpp"

namespace dev {

class SessionModeChanger {
public:
	[[nodiscard]] bool change(GameSessionMode &current, GameSessionMode requested) const;
};

} // namespace dev
