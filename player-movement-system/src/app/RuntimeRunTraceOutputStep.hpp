#pragma once

#include <filesystem>

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputResultBuilder.hpp"
#include "app/RuntimeTraceService.hpp"

namespace dev {

class RuntimeRunTraceOutputStep {
public:
	explicit RuntimeRunTraceOutputStep(RuntimeTraceService traceService = RuntimeTraceService {});

	void save(
	    const std::filesystem::path &path,
	    const GameLoopResult &result,
	    RuntimeOutputResultBuilder &output) const;

private:
	RuntimeTraceService traceService_;
};

} // namespace dev
