#pragma once

#include "runtime/RuntimePolicyGameplayFrameReport.hpp"
#include "runtime/RuntimePolicyGameplayFrameStep.hpp"

namespace iggy::runtime {

class RuntimePolicyGameplayFrameReporter {
public:
	[[nodiscard]] RuntimePolicyGameplayFrameReport report(const RuntimePolicyGameplayFrameResult &result) const;
};

} // namespace iggy::runtime
