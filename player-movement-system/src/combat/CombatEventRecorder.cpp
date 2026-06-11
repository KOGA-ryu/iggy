#include "CombatEventRecorder.hpp"

namespace dev {

void CombatEventRecorder::emit(const CombatEvent &event)
{
	events_.push_back(event);
}

void CombatEventRecorder::clear()
{
	events_.clear();
}

const std::vector<CombatEvent> &CombatEventRecorder::events() const
{
	return events_;
}

} // namespace dev

