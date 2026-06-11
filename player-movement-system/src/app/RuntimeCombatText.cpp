#include "RuntimeCombatText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(CombatEventType type)
{
	switch (type) {
	case CombatEventType::Hit:
		return "Hit";
	case CombatEventType::Defeated:
		return "Defeated";
	case CombatEventType::Rejected:
		return "Rejected";
	}
	return "Unknown";
}

} // namespace

std::string RuntimeCombatText::formatEvent(std::string_view label, const CombatEvent &event) const
{
	std::ostringstream line;
	line << label << " type=" << ToString(event.type)
	     << " damage=" << event.damage
	     << " remainingHp=" << event.remainingHitPoints;
	return line.str();
}

} // namespace dev
