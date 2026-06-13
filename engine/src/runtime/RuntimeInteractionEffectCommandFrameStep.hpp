#pragma once

#include <cstddef>
#include <vector>

#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeInteractionEffectCommandStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy::runtime {

struct RuntimeInteractionEffectCommandFrameEntry {
	std::size_t commandIndex = 0;
	RuntimeInteractionEffectCommandResult result;
};

struct RuntimeInteractionEffectCommandFrameResult {
	std::vector<RuntimeInteractionEffectCommandFrameEntry> interactions;
	std::size_t readyCount = 0;
	std::size_t noEffectCount = 0;
	std::size_t blockedCount = 0;
	std::size_t requestedEffectCount = 0;

	[[nodiscard]] bool hasInteractions() const;
};

class RuntimeInteractionEffectCommandFrameStep {
public:
	[[nodiscard]] RuntimeInteractionEffectCommandFrameResult evaluate(
		const RuntimeSessionState &session,
		const InteractionTarget2DRegistry &targets,
		const InteractionEffectCatalog2D &effects,
		const GameplayCommandFrame2D &frame,
		const InteractionReach2DConfig &reachConfig = {}) const;
};

} // namespace iggy::runtime
