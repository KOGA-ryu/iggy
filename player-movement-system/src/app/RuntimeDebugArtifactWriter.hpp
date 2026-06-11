#pragma once

#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugManifestWriteStep.hpp"
#include "app/RuntimeDebugTraceWriteStep.hpp"
#include "app/RuntimeLoopTypes.hpp"

namespace dev {

struct RuntimeDebugArtifactWriteResult {
	bool traceSaved = false;
	bool manifestSaved = false;
};

class RuntimeDebugArtifactWriter {
public:
	explicit RuntimeDebugArtifactWriter(
	    RuntimeDebugTraceWriteStep traceWrite = RuntimeDebugTraceWriteStep {},
	    RuntimeDebugManifestWriteStep manifestWrite = RuntimeDebugManifestWriteStep {});

	[[nodiscard]] RuntimeDebugArtifactWriteResult write(
	    const RuntimeDebugArtifactPaths &paths,
	    const GameLoopResult &result) const;

private:
	RuntimeDebugTraceWriteStep traceWrite_;
	RuntimeDebugManifestWriteStep manifestWrite_;
};

} // namespace dev
