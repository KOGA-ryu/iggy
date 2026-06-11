#pragma once

#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeSetupFailurePolicy.hpp"

namespace dev {

class RuntimeSetupFrameGate {
public:
	explicit RuntimeSetupFrameGate(RuntimeSetupFailurePolicy failurePolicy = RuntimeSetupFailurePolicy {});

	[[nodiscard]] bool allowsFrames(const RuntimeSetupResult &setup) const;

private:
	RuntimeSetupFailurePolicy failurePolicy_;
};

} // namespace dev
