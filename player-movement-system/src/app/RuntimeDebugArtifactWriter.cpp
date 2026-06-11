#include "RuntimeDebugArtifactWriter.hpp"

namespace dev {

RuntimeDebugArtifactWriter::RuntimeDebugArtifactWriter(
    RuntimeDebugTraceWriteStep traceWrite,
    RuntimeDebugManifestWriteStep manifestWrite)
    : traceWrite_(traceWrite)
    , manifestWrite_(manifestWrite)
{
}

RuntimeDebugArtifactWriteResult RuntimeDebugArtifactWriter::write(
    const RuntimeDebugArtifactPaths &paths,
    const GameLoopResult &result) const
{
	RuntimeDebugArtifactWriteResult write;
	write.traceSaved = traceWrite_.write(paths, result);
	write.manifestSaved = manifestWrite_.write(paths, result, write.traceSaved);
	return write;
}

} // namespace dev
