#pragma once

#include "runtime/RuntimeGameplayFrameReport.hpp"
#include "runtime/RuntimeGameplayFrameStep.hpp"

namespace iggy::runtime {

class RuntimeGameplayFrameReporter {
public:
	[[nodiscard]] RuntimeGameplayFrameReport report(const RuntimeGameplayFrameResult &result) const;
};

} // namespace iggy::runtime
