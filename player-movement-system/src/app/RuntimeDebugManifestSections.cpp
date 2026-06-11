#include "RuntimeDebugManifestSections.hpp"

#include "app/RuntimeDebugManifestSetupText.hpp"
#include "app/RuntimeFramePolicyText.hpp"
#include "app/RuntimeInventoryScriptText.hpp"
#include "app/RuntimeMovementScriptText.hpp"
#include "app/RuntimeRunSummaryText.hpp"

namespace dev {

std::vector<std::string> RuntimeDebugManifestSections::formatRunStatus(const GameLoopResult &result) const
{
	std::vector<std::string> lines;
	lines.push_back(RuntimeRunSummaryText {}.format(result, RuntimeRunSummaryDetail::WithFinalMode));

	if (result.frameReports.empty()) {
		lines.push_back(RuntimeFramePolicyText {}.formatNone("policy latest"));
	} else {
		lines.push_back(RuntimeFramePolicyText {}.format(
		    "policy latest",
		    result.frameReports.back().framePolicy,
		    RuntimeFramePolicyBoolStyle::Words));
	}

	return lines;
}

std::vector<std::string> RuntimeDebugManifestSections::formatSetup(const GameLoopResult &result) const
{
	std::vector<std::string> lines;
	lines.push_back(RuntimeDebugManifestSetupText {}.format(result.setup));

	if (result.setup.movementScriptRan) {
		lines.push_back(RuntimeMovementScriptText {}.formatResult("setup movementScript", result.setup.movementScriptResult));
	}

	if (result.setup.inventoryScriptRan) {
		lines.push_back(RuntimeInventoryScriptText {}.formatResult("setup inventoryScript", result.setup.inventoryScriptResult));
	}

	return lines;
}

std::vector<std::string> RuntimeDebugManifestSections::formatRuntimeScripts(const GameLoopResult &result) const
{
	return {
		RuntimeInventoryScriptText {}.formatAggregate(
		    "runtime inventoryScripts",
		    result.summary.runtimeInventoryScriptResults),
		RuntimeMovementScriptText {}.formatAggregate(
		    "runtime movementScripts",
		    result.summary.runtimeMovementScriptResults),
	};
}

} // namespace dev
