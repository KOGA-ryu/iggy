#include "RuntimeFrameEventDeltaCollector.hpp"

namespace dev {

void RuntimeFrameEventDeltaCollector::beginFrame(
    const SessionEventRecorder &sessionEvents,
    const InventoryEventRecorder &inventoryEvents)
{
	sessionEventOffset_ = sessionEvents.events().size();
	inventoryEventOffset_ = inventoryEvents.events().size();
}

RuntimeFrameEventDeltas RuntimeFrameEventDeltaCollector::collect(
    const SessionEventRecorder &sessionEvents,
    const InventoryEventRecorder &inventoryEvents) const
{
	return {
		.sessionEvents = streamDelta_.eventsSince(sessionEvents.events(), sessionEventOffset_),
		.inventoryEvents = streamDelta_.eventsSince(inventoryEvents.events(), inventoryEventOffset_),
	};
}

} // namespace dev
