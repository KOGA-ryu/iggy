#include "SessionEventRecorder.hpp"

namespace dev {

void SessionEventRecorder::emit(const SessionEvent &event)
{
	events_.push_back(event);
}

void SessionEventRecorder::clear()
{
	events_.clear();
}

const std::vector<SessionEvent> &SessionEventRecorder::events() const
{
	return events_;
}

} // namespace dev
