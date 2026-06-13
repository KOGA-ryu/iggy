#pragma once

#include "runtime/RuntimePlayerInputInteractionEffectFrameReport.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectFrameStep.hpp"

namespace iggy::runtime {

class RuntimePlayerInputInteractionEffectFrameReporter {
public:
	[[nodiscard]] RuntimePlayerInputInteractionEffectFrameReport report(
		const RuntimePlayerInputInteractionEffectFrameResult &result) const;
};

} // namespace iggy::runtime
