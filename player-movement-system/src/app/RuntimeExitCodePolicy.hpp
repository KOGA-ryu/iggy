#pragma once

#include "app/RuntimeExitCodeMapper.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeRunFailurePolicy.hpp"

namespace dev {

class RuntimeExitCodePolicy {
public:
	explicit RuntimeExitCodePolicy(
	    RuntimeRunFailurePolicy runFailurePolicy = RuntimeRunFailurePolicy {},
	    RuntimeExitCodeMapper exitCodeMapper = RuntimeExitCodeMapper {});

	[[nodiscard]] int exitCodeFor(const GameLoopResult &result) const;

private:
	RuntimeRunFailurePolicy runFailurePolicy_;
	RuntimeExitCodeMapper exitCodeMapper_;
};

} // namespace dev
