#include "RuntimeDebugArtifactWriter.hpp"

namespace dev {

RuntimeDebugArtifactWriter::RuntimeDebugArtifactWriter(
    RuntimeTraceService traceService,
    RuntimeDebugManifest manifest,
    TextFileStore textFileStore)
    : traceService_(traceService)
    , manifest_(manifest)
    , textFileStore_(textFileStore)
{
}

RuntimeDebugArtifactWriteResult RuntimeDebugArtifactWriter::write(
    const RuntimeDebugArtifactPaths &paths,
    const GameLoopResult &result) const
{
	RuntimeDebugArtifactWriteResult write;
	write.traceSaved = traceService_.saveRunTrace(paths.tracePath, result);
	write.manifestSaved = textFileStore_.saveLines(paths.manifestPath, manifest_.format(result, {
	    .rootPath = paths.rootPath,
	    .manifestPath = paths.manifestPath,
	    .tracePath = paths.tracePath,
	    .traceSaved = write.traceSaved,
	}));
	return write;
}

} // namespace dev
