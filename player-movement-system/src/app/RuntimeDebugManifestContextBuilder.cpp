#include "RuntimeDebugManifestContextBuilder.hpp"

namespace dev {

RuntimeDebugManifestContext RuntimeDebugManifestContextBuilder::build(
    const RuntimeDebugArtifactPaths &paths,
    bool traceSaved) const
{
	return {
		.rootPath = paths.rootPath,
		.manifestPath = paths.manifestPath,
		.tracePath = paths.tracePath,
		.traceSaved = traceSaved,
	};
}

} // namespace dev
