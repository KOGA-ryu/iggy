#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputFailurePolicy.hpp"
#include "app/RuntimeSetupFailurePolicy.hpp"

namespace dev {

class RuntimeExitCodePolicy {
public:
	explicit RuntimeExitCodePolicy(
	    RuntimeSetupFailurePolicy setupFailurePolicy = RuntimeSetupFailurePolicy {},
	    RuntimeOutputFailurePolicy outputFailurePolicy = RuntimeOutputFailurePolicy {});

	[[nodiscard]] int exitCodeFor(const GameLoopResult &result) const;
	[[nodiscard]] bool failed(const GameLoopResult &result) const;

private:
	RuntimeSetupFailurePolicy setupFailurePolicy_;
	RuntimeOutputFailurePolicy outputFailurePolicy_;
};

} // namespace dev
