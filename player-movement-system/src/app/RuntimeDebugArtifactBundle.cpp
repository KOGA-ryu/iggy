#include "RuntimeDebugArtifactBundle.hpp"

namespace dev {

bool RuntimeDebugArtifactBundleResult::saved() const
{
	return rootPrepared && traceSaved && manifestSaved;
}

RuntimeDebugArtifactBundle::RuntimeDebugArtifactBundle(
    RuntimeDebugArtifactLayout layout,
    RuntimeDebugArtifactWriter writer)
    : layout_(layout)
    , writer_(writer)
{
}

RuntimeDebugArtifactBundleResult RuntimeDebugArtifactBundle::save(
    const std::filesystem::path &rootPath,
    const GameLoopResult &result) const
{
	RuntimeDebugArtifactPaths paths = layout_.pathsForRoot(rootPath);
	RuntimeDebugArtifactBundleResult bundle {
		.rootPath = paths.rootPath,
		.manifestPath = paths.manifestPath,
		.tracePath = paths.tracePath,
	};

	std::error_code error;
	std::filesystem::create_directories(bundle.rootPath, error);
	if (error)
		return bundle;

	bundle.rootPrepared = true;
	RuntimeDebugArtifactWriteResult write = writer_.write(paths, result);
	bundle.traceSaved = write.traceSaved;
	bundle.manifestSaved = write.manifestSaved;
	return bundle;
}

} // namespace dev
