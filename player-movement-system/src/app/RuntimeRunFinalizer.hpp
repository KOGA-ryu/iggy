#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputFinalizer.hpp"
#include "session/GameSession.hpp"

namespace dev {

class RuntimeRunFinalizer {
public:
	explicit RuntimeRunFinalizer(RuntimeOutputFinalizer outputFinalizer = RuntimeOutputFinalizer {});

	void finalize(const GameSession &session, const RuntimeOutputSettings &settings, GameLoopResult &result) const;

private:
	RuntimeOutputFinalizer outputFinalizer_;
};

} // namespace dev
