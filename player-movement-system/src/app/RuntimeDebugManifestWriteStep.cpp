#include "RuntimeDebugManifestWriteStep.hpp"

#include "app/RuntimeDebugManifestContextBuilder.hpp"

namespace dev {

RuntimeDebugManifestWriteStep::RuntimeDebugManifestWriteStep(
    RuntimeDebugManifest manifest,
    TextFileStore textFileStore)
    : manifest_(manifest)
    , textFileStore_(textFileStore)
{
}

bool RuntimeDebugManifestWriteStep::write(
    const RuntimeDebugArtifactPaths &paths,
    const GameLoopResult &result,
    bool traceSaved) const
{
	return textFileStore_.saveLines(
	    paths.manifestPath,
	    manifest_.format(result, RuntimeDebugManifestContextBuilder {}.build(paths, traceSaved)));
}

} // namespace dev
