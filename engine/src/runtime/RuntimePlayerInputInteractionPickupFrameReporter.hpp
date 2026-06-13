#pragma once

#include "runtime/RuntimePlayerInputInteractionPickupFrameReport.hpp"

namespace iggy::runtime {

class RuntimePlayerInputInteractionPickupFrameReporter {
public:
	[[nodiscard]] RuntimePlayerInputInteractionPickupFrameReport report(
		const RuntimePlayerInputInteractionPickupFrameResult &result) const;
};

} // namespace iggy::runtime
