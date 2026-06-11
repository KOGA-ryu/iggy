#include "RuntimeDebugArtifactBundle.hpp"

#include "files/TextFileStore.hpp"

namespace dev {

bool RuntimeDebugArtifactBundleResult::saved() const
{
	return rootPrepared && traceSaved && manifestSaved;
}

RuntimeDebugArtifactBundle::RuntimeDebugArtifactBundle(RuntimeTraceService traceService, RuntimeDebugManifest manifest)
    : traceService_(traceService)
    , manifest_(manifest)
{
}

RuntimeDebugArtifactBundleResult RuntimeDebugArtifactBundle::save(
    const std::filesystem::path &rootPath,
    const GameLoopResult &result) const
{
	RuntimeDebugArtifactBundleResult bundle {
		.rootPath = rootPath,
		.manifestPath = rootPath / "manifest.txt",
		.tracePath = rootPath / "run.trace",
	};

	std::error_code error;
	std::filesystem::create_directories(rootPath, error);
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
