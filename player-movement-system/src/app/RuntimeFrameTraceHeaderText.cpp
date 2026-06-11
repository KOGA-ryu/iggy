#include "RuntimeFrameTraceHeaderText.hpp"

#include <sstream>

namespace dev {

std::string RuntimeFrameTraceHeaderText::format(const RuntimeFrameReport &report) const
{
	std::ostringstream line;
	line << "frame rawInput=" << report.rawInputEventsRouted
	     << " sessionResults=" << report.sessionCommandResults.size()
	     << " inventoryScripts=" << report.inventoryScriptResults.size()
	     << " inventoryResults=" << report.inventoryCommandResults.size()
	     << " movementScripts=" << report.movementScriptResults.size()
	     << " movementQueued=" << report.movementCommandsQueued
	     << " movementEvents=" << report.frameEvents.movementEvents().size()
	     << " combatEvents=" << report.frameEvents.combatEvents().size()
	     << " effects=" << report.frameEvents.effectRequests().size()
	     << " sessionEvents=" << report.sessionEvents.size()
	     << " inventoryEvents=" << report.inventoryEvents.size();
	return line.str();
}

} // namespace dev
