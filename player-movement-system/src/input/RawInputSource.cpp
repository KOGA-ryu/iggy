#include "RawInputSource.hpp"

namespace dev {

void QueuedRawInputSource::enqueue(const RawInputEvent &event)
{
	events_.push_back(event);
}

void QueuedRawInputSource::clear()
{
	events_.clear();
}

std::vector<RawInputEvent> QueuedRawInputSource::drain()
{
	std::vector<RawInputEvent> drained;
	drained.swap(events_);
	return drained;
}

bool QueuedRawInputSource::empty() const
{
	return events_.empty();
}

std::size_t QueuedRawInputSource::size() const
{
	return events_.size();
}

} // namespace dev
