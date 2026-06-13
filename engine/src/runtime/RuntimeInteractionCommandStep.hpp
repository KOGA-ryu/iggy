#pragma once

#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionPlan2D.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy::runtime {

enum class RuntimeInteractionCommandStatus {
	Ready,
	NotInteractCommand,
	MissingTargetId,
	TargetNotFound,
	TargetDisabled,
	OutOfRange,
	MissingPlayer,
};

struct RuntimeInteractionCommandResult {
	RuntimeInteractionCommandStatus status = RuntimeInteractionCommandStatus::NotInteractCommand;
	GameplayCommand2D command;
	InteractionPlan2DResult plan;

	[[nodiscard]] bool ready() const;
};

class RuntimeInteractionCommandStep {
public:
	[[nodiscard]] RuntimeInteractionCommandResult evaluate(
		const RuntimeSessionState &session,
		const InteractionTarget2DRegistry &registry,
		const GameplayCommand2D &command,
		const InteractionReach2DConfig &reachConfig = {}) const;
};

} // namespace iggy::runtime
