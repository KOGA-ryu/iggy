#pragma once

#include "app/RuntimeInputTypes.hpp"
#include "input/RawInput.hpp"

namespace dev {

class RuntimeBlockedPointerInputStep {
public:
	[[nodiscard]] RuntimeInputRouteResult route(
	    const RawInputEvent &event,
	    PlayerActionBlockReason blockReason) const;
};

} // namespace dev
