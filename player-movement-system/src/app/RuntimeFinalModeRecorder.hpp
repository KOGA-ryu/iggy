#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "session/GameSession.hpp"

namespace dev {

class RuntimeFinalModeRecorder {
public:
	void record(const GameSession &session, GameLoopResult &result) const;
};

} // namespace dev
