#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunFailurePolicy.hpp"

namespace dev {

class RuntimeExitCodePolicy {
public:
	explicit RuntimeExitCodePolicy(RuntimeRunFailurePolicy runFailurePolicy = RuntimeRunFailurePolicy {});

	[[nodiscard]] int exitCodeFor(const GameLoopResult &result) const;
	[[nodiscard]] bool failed(const GameLoopResult &result) const;

private:
	RuntimeRunFailurePolicy runFailurePolicy_;
};

} // namespace dev
