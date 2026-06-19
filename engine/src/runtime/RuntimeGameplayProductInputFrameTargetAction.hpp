#pragma once

#include <cstddef>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayProductInputAdapter.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductInputFrameTargetActionStatus {
	Unchanged,
	InteractSynthesized,
};

struct RuntimeGameplayProductInputFrameTargetActionInput {
	RuntimeGameplayProductInputFrameTargetContextResult targetContext;
};

struct RuntimeGameplayProductInputFrameTargetActionResult {
	RuntimeGameplayProductInputFrameTargetActionStatus status =
		RuntimeGameplayProductInputFrameTargetActionStatus::Unchanged;
	RuntimeGameplayProductInputFrame2D frame;
	bool hasActionEvent = false;
	std::size_t actionEventIndex = 0;
	ResourceId targetId;
};

class RuntimeGameplayProductInputFrameTargetAction {
public:
	[[nodiscard]] RuntimeGameplayProductInputFrameTargetActionResult
	synthesize(
		const RuntimeGameplayProductInputFrameTargetActionInput &input) const;
};

} // namespace iggy::runtime
