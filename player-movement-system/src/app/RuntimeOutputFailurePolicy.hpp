#pragma once

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeOutputFailurePolicy {
public:
	[[nodiscard]] bool failed(const RuntimeOutputResult &result) const;
};

} // namespace dev
