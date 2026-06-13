#pragma once

#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameReport.hpp"

namespace iggy::runtime {

class RuntimePlayerInputInteractionEffectApplyFrameReporter {
public:
	[[nodiscard]] RuntimePlayerInputInteractionEffectApplyFrameReport report(
		const RuntimePlayerInputInteractionEffectApplyFrameResult &result) const;
};

} // namespace iggy::runtime
