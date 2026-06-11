#pragma once

#include <vector>

#include "app/RuntimeLoopTypes.hpp"
#include "inventory/InventoryEvent.hpp"
#include "session/SessionEvent.hpp"

namespace dev {

class RuntimeFrameCompletionReportRecorder {
public:
	void record(
	    std::vector<SessionEvent> sessionEvents,
	    std::vector<InventoryEvent> inventoryEvents,
	    RuntimeFrameReport frame,
	    GameLoopResult &result) const;
};

} // namespace dev
