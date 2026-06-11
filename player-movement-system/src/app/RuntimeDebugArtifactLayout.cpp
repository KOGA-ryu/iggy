#include "RuntimeDebugArtifactLayout.hpp"

namespace dev {

RuntimeDebugArtifactPaths RuntimeDebugArtifactLayout::pathsForRoot(const std::filesystem::path &rootPath) const
{
	return {
		.rootPath = rootPath,
		.manifestPath = rootPath / "manifest.txt",
		.tracePath = rootPath / "run.trace",
	};
}

} // namespace dev
