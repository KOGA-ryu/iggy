#pragma once

#include <cstddef>
#include <vector>

#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeInteractionEffectApplyStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy::runtime {

struct RuntimeInteractionEffectApplyFrameEntry {
	std::size_t commandIndex = 0;
	RuntimeInteractionEffectApplyResult result;
};

enum class RuntimeInteractionEffectApplyFrameStatus {
	Applied,
	NoOp,
	Failed,
};

struct RuntimeInteractionEffectApplyFrameResult {
	RuntimeInteractionEffectApplyFrameStatus status = RuntimeInteractionEffectApplyFrameStatus::NoOp;
	InteractionTarget2DRegistry registry;
	std::vector<RuntimeInteractionEffectApplyFrameEntry> entries;
	std::size_t appliedCount = 0;
	std::size_t noOpCount = 0;
	std::size_t notReadyCount = 0;
	std::size_t failedCount = 0;
	bool mutated = false;

	[[nodiscard]] bool hasInteractions() const;
};

class RuntimeInteractionEffectApplyFrameStep {
public:
	[[nodiscard]] RuntimeInteractionEffectApplyFrameResult apply(
		const RuntimeSessionState &session,
		const InteractionTarget2DRegistry &targets,
		const InteractionEffectCatalog2D &effects,
		const GameplayCommandFrame2D &frame,
		const InteractionReach2DConfig &reachConfig = {}) const;
};

} // namespace iggy::runtime
