#include "RuntimeDebugArtifactBundle.hpp"

#include "app/RuntimeDebugArtifactBundleResultBuilder.hpp"

namespace dev {

bool RuntimeDebugArtifactBundleResult::saved() const
{
	return rootPrepared && traceSaved && manifestSaved;
}

RuntimeDebugArtifactBundle::RuntimeDebugArtifactBundle(
    RuntimeDebugArtifactLayout layout,
    RuntimeDebugArtifactRootPreparer rootPreparer,
    RuntimeDebugArtifactWriter writer)
    : layout_(layout)
    , rootPreparer_(rootPreparer)
    , writer_(writer)
{
}

RuntimeDebugArtifactBundleResult RuntimeDebugArtifactBundle::save(
    const std::filesystem::path &rootPath,
    const GameLoopResult &result) const
{
	RuntimeDebugArtifactPaths paths = layout_.pathsForRoot(rootPath);
	RuntimeDebugArtifactBundleResultBuilder bundle { paths };

	if (!rootPreparer_.prepare(paths.rootPath))
		return bundle.result();

	bundle.markRootPrepared();
	bundle.recordWrite(writer_.write(paths, result));
	return bundle.result();
}

} // namespace dev
