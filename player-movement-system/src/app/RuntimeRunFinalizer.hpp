#pragma once

#include "app/RuntimeFinalModeRecorder.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputFinalizer.hpp"
#include "session/GameSession.hpp"

namespace dev {

class RuntimeRunFinalizer {
public:
	explicit RuntimeRunFinalizer(
	    RuntimeFinalModeRecorder finalModeRecorder = RuntimeFinalModeRecorder {},
	    RuntimeOutputFinalizer outputFinalizer = RuntimeOutputFinalizer {});

	void finalize(const GameSession &session, const RuntimeOutputSettings &settings, GameLoopResult &result) const;

private:
	RuntimeFinalModeRecorder finalModeRecorder_;
	RuntimeOutputFinalizer outputFinalizer_;
};

} // namespace dev
