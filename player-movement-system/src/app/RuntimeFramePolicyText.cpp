#include "RuntimeFramePolicyText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *BoolText(bool value, RuntimeFramePolicyBoolStyle style)
{
	switch (style) {
	case RuntimeFramePolicyBoolStyle::Numeric:
		return value ? "1" : "0";
	case RuntimeFramePolicyBoolStyle::Words:
		return value ? "true" : "false";
	}
	return value ? "true" : "false";
}

} // namespace

std::string RuntimeFramePolicyText::format(
    std::string_view label,
    const SimulationFramePolicyDescription &description,
    RuntimeFramePolicyBoolStyle boolStyle) const
{
	std::ostringstream line;
	line << label << "=" << description.modeName
	     << " acceptCommands=" << BoolText(description.policy.acceptCommands, boolStyle)
	     << " updatePlayers=" << BoolText(description.policy.updatePlayers, boolStyle)
	     << " updateEnemies=" << BoolText(description.policy.updateEnemies, boolStyle)
	     << " reason=" << description.summary;
	return line.str();
}

std::string RuntimeFramePolicyText::formatNone(std::string_view label) const
{
	std::ostringstream line;
	line << label << "=none";
	return line.str();
}

} // namespace dev
