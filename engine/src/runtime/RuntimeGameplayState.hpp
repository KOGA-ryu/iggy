#pragma once

#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeInteractionState.hpp"
#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimeSessionState.hpp"

namespace iggy::runtime {

struct RuntimeGameplayState {
	RuntimeSessionState session;
	RuntimeCommandQueueState commandQueue;
	RuntimeInteractionState interaction;
	RuntimeInventoryState inventory;
};

} // namespace iggy::runtime
