#pragma once

#include "app/RuntimeDebugArtifactBundle.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeTraceService.hpp"

namespace dev {

class RuntimeOutputFinalizer {
public:
	explicit RuntimeOutputFinalizer(
	    RuntimeTraceService traceService = RuntimeTraceService {},
	    RuntimeDebugArtifactBundle debugBundle = RuntimeDebugArtifactBundle {});

	void finalize(const RuntimeOutputSettings &settings, GameLoopResult &result) const;
	[[nodiscard]] static bool failed(const RuntimeOutputResult &result);

private:
	RuntimeTraceService traceService_;
	RuntimeDebugArtifactBundle debugBundle_;
};

} // namespace dev
