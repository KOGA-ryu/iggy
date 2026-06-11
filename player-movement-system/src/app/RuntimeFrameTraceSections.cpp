#include "RuntimeFrameTraceSections.hpp"

#include "app/RuntimeCombatText.hpp"
#include "app/RuntimeEffectText.hpp"
#include "app/RuntimeInventoryScriptText.hpp"
#include "app/RuntimeInventoryText.hpp"
#include "app/RuntimeMovementEventText.hpp"
#include "app/RuntimeMovementScriptText.hpp"
#include "app/RuntimePlayerActionText.hpp"
#include "app/RuntimeSessionText.hpp"

#include <sstream>

namespace dev {

std::vector<std::string> RuntimeFrameTraceSections::formatRuntimeSources(const RuntimeFrameReport &report) const
{
	std::vector<std::string> lines;

	for (std::size_t i = 0; i < report.sessionCommandResults.size(); ++i) {
		const SessionCommandResult &result = report.sessionCommandResults[i];
		std::ostringstream line;
		line << "sessionResult[" << i << "]";
		lines.push_back(RuntimeSessionText {}.formatResult(line.str(), result));
	}

	for (std::size_t i = 0; i < report.movementInputBlockReasons.size(); ++i) {
		std::ostringstream line;
		line << "movementInputBlock[" << i << "]";
		lines.push_back(RuntimePlayerActionText {}.formatMovementBlockReason(line.str(), report.movementInputBlockReasons[i]));
	}

	for (std::size_t i = 0; i < report.inventoryScriptResults.size(); ++i) {
		const InventoryScriptRunResult &result = report.inventoryScriptResults[i];
		std::ostringstream line;
		line << "inventoryScript[" << i << "]";
		lines.push_back(RuntimeInventoryScriptText {}.formatResult(line.str(), result));
	}

	for (std::size_t i = 0; i < report.inventoryCommandResults.size(); ++i) {
		const InventoryCommandResult &result = report.inventoryCommandResults[i];
		std::ostringstream line;
		line << "inventoryResult[" << i << "]";
		lines.push_back(RuntimeInventoryText {}.formatResult(line.str(), result));
	}

	for (std::size_t i = 0; i < report.movementScriptResults.size(); ++i) {
		const MovementScriptRunResult &result = report.movementScriptResults[i];
		std::ostringstream line;
		line << "movementScript[" << i << "]";
		lines.push_back(RuntimeMovementScriptText {}.formatResult(line.str(), result));
	}

	return lines;
}

std::vector<std::string> RuntimeFrameTraceSections::formatLifecycleEvents(const RuntimeFrameReport &report) const
{
	std::vector<std::string> lines;

	for (std::size_t i = 0; i < report.sessionEvents.size(); ++i) {
		const SessionEvent &event = report.sessionEvents[i];
		std::ostringstream line;
		line << "sessionEvent[" << i << "]";
		lines.push_back(RuntimeSessionText {}.formatEvent(line.str(), event));
	}

	for (std::size_t i = 0; i < report.inventoryEvents.size(); ++i) {
		const InventoryEvent &event = report.inventoryEvents[i];
		std::ostringstream line;
		line << "inventoryEvent[" << i << "]";
		lines.push_back(RuntimeInventoryText {}.formatEvent(line.str(), event));
	}

	return lines;
}

std::vector<std::string> RuntimeFrameTraceSections::formatSimulationEvents(const RuntimeFrameReport &report) const
{
	std::vector<std::string> lines;

	for (std::size_t i = 0; i < report.frameEvents.movementEvents().size(); ++i) {
		const MovementEvent &event = report.frameEvents.movementEvents()[i];
		std::ostringstream line;
		line << "movementEvent[" << i << "]";
		lines.push_back(RuntimeMovementEventText {}.formatEvent(line.str(), event));
	}

	for (std::size_t i = 0; i < report.frameEvents.combatEvents().size(); ++i) {
		const CombatEvent &event = report.frameEvents.combatEvents()[i];
		std::ostringstream line;
		line << "combatEvent[" << i << "]";
		lines.push_back(RuntimeCombatText {}.formatEvent(line.str(), event));
	}

	for (std::size_t i = 0; i < report.frameEvents.effectRequests().size(); ++i) {
		const EffectRequest &request = report.frameEvents.effectRequests()[i];
		std::ostringstream line;
		line << "effect[" << i << "]";
		lines.push_back(RuntimeEffectText {}.formatRequest(line.str(), request));
	}

	return lines;
}

} // namespace dev
