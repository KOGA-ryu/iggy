#include "RuntimeDebugManifestPathsText.hpp"

namespace dev {

std::vector<std::string> RuntimeDebugManifestPathsText::format(const RuntimeDebugManifestContext &context) const
{
	return {
		"paths root=" + context.rootPath.string(),
		"paths manifest=" + context.manifestPath.filename().string(),
		"paths trace=" + context.tracePath.filename().string(),
	};
}

} // namespace dev
