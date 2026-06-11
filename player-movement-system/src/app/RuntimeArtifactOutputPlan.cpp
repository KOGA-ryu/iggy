#include "RuntimeArtifactOutputPlan.hpp"

namespace dev {

std::vector<RuntimeArtifactOutputRequest> RuntimeArtifactOutputPlan::build(const RuntimeOutputSettings &settings) const
{
	std::vector<RuntimeArtifactOutputRequest> requests;
	if (settings.runTracePath.has_value()) {
		requests.push_back({
		    .kind = RuntimeArtifactOutputKind::RunTrace,
		    .path = *settings.runTracePath,
		});
	}

	if (settings.debugBundlePath.has_value()) {
		requests.push_back({
		    .kind = RuntimeArtifactOutputKind::DebugBundle,
		    .path = *settings.debugBundlePath,
		});
	}

	return requests;
}

} // namespace dev
