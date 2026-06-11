#include "RuntimeDebugManifest.hpp"

#include "app/RuntimeDebugManifestIndexText.hpp"
#include "app/RuntimeDebugManifestPathsText.hpp"
#include "app/RuntimeDebugManifestSections.hpp"

namespace dev {

namespace {

void AppendLines(std::vector<std::string> &lines, const std::vector<std::string> &section)
{
	lines.insert(lines.end(), section.begin(), section.end());
}

} // namespace

std::vector<std::string> RuntimeDebugManifest::format(
    const GameLoopResult &result,
    const RuntimeDebugManifestContext &context) const
{
	std::vector<std::string> lines = RuntimeDebugManifestIndexText {}.format(context);

	RuntimeDebugManifestSections sections;
	AppendLines(lines, sections.formatRunStatus(result));
	AppendLines(lines, sections.formatSetup(result));
	AppendLines(lines, sections.formatRuntimeScripts(result));

	const std::vector<std::string> paths = RuntimeDebugManifestPathsText {}.format(context);
	lines.insert(lines.end(), paths.begin(), paths.end());
	return lines;
}

} // namespace dev
