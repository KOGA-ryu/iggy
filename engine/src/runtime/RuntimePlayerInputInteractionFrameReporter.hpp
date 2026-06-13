#pragma once

#include "runtime/RuntimePlayerInputInteractionFrameReport.hpp"
#include "runtime/RuntimePlayerInputInteractionFrameStep.hpp"

namespace iggy::runtime {

class RuntimePlayerInputInteractionFrameReporter {
public:
	[[nodiscard]] RuntimePlayerInputInteractionFrameReport report(const RuntimePlayerInputInteractionFrameResult &result) const;
};

} // namespace iggy::runtime
