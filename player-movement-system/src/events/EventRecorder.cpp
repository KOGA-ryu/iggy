#include "EventRecorder.hpp"

namespace dev {

void EventRecorder::emit(const MovementEvent &event)
{
	events_.push_back(event);
}

void EventRecorder::clear()
{
	events_.clear();
}

const std::vector<MovementEvent> &EventRecorder::events() const
{
	return events_;
}

} // namespace dev

