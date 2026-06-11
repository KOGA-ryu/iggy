#pragma once

#include <cstddef>
#include <vector>

#include "app/RuntimeEventStreamDelta.hpp"
#include "inventory/InventoryEvent.hpp"
#include "inventory/InventoryEventRecorder.hpp"
#include "session/SessionEvent.hpp"
#include "session/SessionEventRecorder.hpp"

namespace dev {

struct RuntimeFrameEventDeltas {
	std::vector<SessionEvent> sessionEvents;
	std::vector<InventoryEvent> inventoryEvents;
};

class RuntimeFrameEventDeltaCollector {
public:
	void beginFrame(
	    const SessionEventRecorder &sessionEvents,
	    const InventoryEventRecorder &inventoryEvents);

	[[nodiscard]] RuntimeFrameEventDeltas collect(
	    const SessionEventRecorder &sessionEvents,
	    const InventoryEventRecorder &inventoryEvents) const;

private:
	std::size_t sessionEventOffset_ = 0;
	std::size_t inventoryEventOffset_ = 0;
	RuntimeEventStreamDelta streamDelta_;
};

} // namespace dev
