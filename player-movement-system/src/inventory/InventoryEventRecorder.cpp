#include "InventoryEventRecorder.hpp"

namespace dev {

void InventoryEventRecorder::emit(const InventoryEvent &event)
{
	events_.push_back(event);
}

void InventoryEventRecorder::clear()
{
	events_.clear();
}

const std::vector<InventoryEvent> &InventoryEventRecorder::events() const
{
	return events_;
}

} // namespace dev
