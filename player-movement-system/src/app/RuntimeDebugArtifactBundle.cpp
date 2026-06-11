#include "RuntimeDebugArtifactBundle.hpp"

#include "files/TextFileStore.hpp"

namespace dev {

bool RuntimeDebugArtifactBundleResult::saved() const
{
	return rootPrepared && traceSaved && manifestSaved;
}

RuntimeDebugArtifactBundle::RuntimeDebugArtifactBundle(
    RuntimeTraceService traceService,
    RuntimeDebugManifest manifest,
    RuntimeDebugArtifactLayout layout)
    : traceService_(traceService)
    , manifest_(manifest)
    , layout_(layout)
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
	bundle.traceSaved = traceService_.saveRunTrace(bundle.tracePath, result);
	bundle.manifestSaved = TextFileStore {}.saveLines(bundle.manifestPath, manifest_.format(result, {
	    .rootPath = bundle.rootPath,
	    .manifestPath = bundle.manifestPath,
	    .tracePath = bundle.tracePath,
	    .traceSaved = bundle.traceSaved,
	}));
	return bundle;
}

} // namespace dev
