#pragma once

#include "app/RuntimeLoopTypes.hpp"

namespace dev {

class RuntimeSetupFailurePolicy {
public:
	[[nodiscard]] bool failed(const RuntimeSetupResult &result) const;
};

} // namespace dev
