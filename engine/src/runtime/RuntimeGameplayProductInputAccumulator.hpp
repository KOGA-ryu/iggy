#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayProductInputAdapter.hpp"

namespace iggy::runtime {

struct RuntimeGameplayProductInputAccumulatorState {
	std::vector<RuntimeGameplayProductInputControl2D> heldControls;
	std::vector<RuntimeGameplayProductInputEvent2D> pendingOneShotEvents;
};

struct RuntimeGameplayProductInputAccumulatorRecordResult {
	RuntimeGameplayProductInputAccumulatorState state;
	std::size_t heldControlCount = 0;
	std::size_t pendingOneShotCount = 0;
	bool changed = false;
};

struct RuntimeGameplayProductInputAccumulatorFrameInput {
	RuntimeGameplayProductInputAccumulatorState state;
	PlayerInputBindingContext2D bindingContext;
};

struct RuntimeGameplayProductInputAccumulatorFrameResult {
	RuntimeGameplayProductInputAccumulatorState state;
	RuntimeGameplayProductInputFrame2D frame;
	std::size_t heldEventCount = 0;
	std::size_t oneShotEventCount = 0;
	std::size_t eventCount = 0;
};

class RuntimeGameplayProductInputAccumulator {
public:
	[[nodiscard]] RuntimeGameplayProductInputAccumulatorRecordResult record(
		const RuntimeGameplayProductInputAccumulatorState &state,
		const RuntimeGameplayProductInputEvent2D &event) const;

	[[nodiscard]] RuntimeGameplayProductInputAccumulatorFrameResult buildFrame(
		const RuntimeGameplayProductInputAccumulatorFrameInput &input) const;

	[[nodiscard]] RuntimeGameplayProductInputAccumulatorState clear(
		const RuntimeGameplayProductInputAccumulatorState &state) const;
};

} // namespace iggy::runtime
