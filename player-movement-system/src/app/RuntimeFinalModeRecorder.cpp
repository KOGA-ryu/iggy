#include "RuntimeFinalModeRecorder.hpp"

namespace dev {

void RuntimeFinalModeRecorder::record(const GameSession &session, GameLoopResult &result) const
{
	result.finalMode = session.mode();
}

} // namespace dev
