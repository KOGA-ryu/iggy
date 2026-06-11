#pragma once

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeExitCodePolicy {
public:
	[[nodiscard]] int exitCodeFor(const GameLoopResult &result) const;
	[[nodiscard]] bool failed(const GameLoopResult &result) const;
};

} // namespace dev
