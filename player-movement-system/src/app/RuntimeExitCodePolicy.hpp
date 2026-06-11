#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputFailurePolicy.hpp"

namespace dev {

class RuntimeExitCodePolicy {
public:
	explicit RuntimeExitCodePolicy(RuntimeOutputFailurePolicy outputFailurePolicy = RuntimeOutputFailurePolicy {});

	[[nodiscard]] int exitCodeFor(const GameLoopResult &result) const;
	[[nodiscard]] bool failed(const GameLoopResult &result) const;

private:
	RuntimeOutputFailurePolicy outputFailurePolicy_;
};

} // namespace dev
