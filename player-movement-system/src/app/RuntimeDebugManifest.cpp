#include "RuntimeDebugManifest.hpp"

#include "app/RuntimeFramePolicyText.hpp"
#include "app/RuntimeRunSummaryText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *BoolText(bool value)
{
	return value ? "true" : "false";
}

} // namespace

std::vector<std::string> RuntimeDebugManifest::format(
    const GameLoopResult &result,
    const RuntimeDebugManifestContext &context) const
{
	std::vector<std::string> lines;
	lines.push_back("bundle version=1");
	lines.push_back("trace=run.trace saved=" + std::string { BoolText(context.traceSaved) });

	lines.push_back(RuntimeRunSummaryText {}.format(result, RuntimeRunSummaryDetail::WithFinalMode));

	if (result.frameReports.empty()) {
		lines.push_back(RuntimeFramePolicyText {}.formatNone("policy latest"));
	} else {
		lines.push_back(RuntimeFramePolicyText {}.format(
		    "policy latest",
		    result.frameReports.back().framePolicy,
		    RuntimeFramePolicyBoolStyle::Words));
	}

	{
		std::ostringstream setup;
		setup << "setup startupScriptRan=" << BoolText(result.setup.startupScriptRan)
		      << " inventoryScriptRan=" << BoolText(result.setup.inventoryScriptRan);
		lines.push_back(setup.str());
	}

	lines.push_back("paths root=" + context.rootPath.string());
	lines.push_back("paths manifest=" + context.manifestPath.filename().string());
	lines.push_back("paths trace=" + context.tracePath.filename().string());
	return lines;
}

} // namespace dev
