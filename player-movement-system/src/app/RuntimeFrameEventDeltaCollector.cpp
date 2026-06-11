#include "RuntimeFrameEventDeltaCollector.hpp"

namespace dev {

namespace {

template <typename Event>
std::vector<Event> EventsSince(const std::vector<Event> &events, std::size_t offset)
{
	if (offset >= events.size())
		return {};
	return { events.begin() + static_cast<std::ptrdiff_t>(offset), events.end() };
}

} // namespace

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
		.sessionEvents = EventsSince(sessionEvents.events(), sessionEventOffset_),
		.inventoryEvents = EventsSince(inventoryEvents.events(), inventoryEventOffset_),
	};
}

} // namespace dev
