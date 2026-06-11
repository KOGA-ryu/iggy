#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputFailurePolicy.hpp"
#include "app/RuntimeSetupFailurePolicy.hpp"

namespace dev {

class RuntimeRunFailurePolicy {
public:
	explicit RuntimeRunFailurePolicy(
	    RuntimeSetupFailurePolicy setupFailurePolicy = RuntimeSetupFailurePolicy {},
	    RuntimeOutputFailurePolicy outputFailurePolicy = RuntimeOutputFailurePolicy {});

	[[nodiscard]] bool failed(const GameLoopResult &result) const;

private:
	RuntimeSetupFailurePolicy setupFailurePolicy_;
	RuntimeOutputFailurePolicy outputFailurePolicy_;
};

} // namespace dev
