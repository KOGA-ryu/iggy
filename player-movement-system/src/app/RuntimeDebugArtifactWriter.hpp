#pragma once

#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugManifest.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeTraceService.hpp"
#include "files/TextFileStore.hpp"

namespace dev {

struct RuntimeDebugArtifactWriteResult {
	bool traceSaved = false;
	bool manifestSaved = false;
};

class RuntimeDebugArtifactWriter {
public:
	explicit RuntimeDebugArtifactWriter(
	    RuntimeTraceService traceService = RuntimeTraceService {},
	    RuntimeDebugManifest manifest = RuntimeDebugManifest {},
	    TextFileStore textFileStore = TextFileStore {});

	[[nodiscard]] RuntimeDebugArtifactWriteResult write(
	    const RuntimeDebugArtifactPaths &paths,
	    const GameLoopResult &result) const;

private:
	RuntimeTraceService traceService_;
	RuntimeDebugManifest manifest_;
	TextFileStore textFileStore_;
};

} // namespace dev
