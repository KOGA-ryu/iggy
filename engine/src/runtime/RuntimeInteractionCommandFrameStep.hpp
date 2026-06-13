#pragma once

#include <cstddef>
#include <vector>

#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeInteractionCommandStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy::runtime {

struct RuntimeInteractionCommandFrameEntry {
	std::size_t commandIndex = 0;
	RuntimeInteractionCommandResult result;
};

struct RuntimeInteractionCommandFrameResult {
	std::vector<RuntimeInteractionCommandFrameEntry> interactions;
	std::size_t readyCount = 0;
	std::size_t blockedCount = 0;

	[[nodiscard]] bool hasInteractions() const;
};

class RuntimeInteractionCommandFrameStep {
public:
	[[nodiscard]] RuntimeInteractionCommandFrameResult evaluate(
		const RuntimeSessionState &session,
		const InteractionTarget2DRegistry &registry,
		const GameplayCommandFrame2D &frame,
		const InteractionReach2DConfig &reachConfig = {}) const;
};

} // namespace iggy::runtime
