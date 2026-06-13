#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimePlayerInputCommandReport.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameStep.hpp"
#include "scene/interaction/InteractionEvent2D.hpp"

namespace iggy::runtime {

enum class RuntimePlayerInputInteractionEffectApplyFrameEvent {
	PlayerCommandAccepted,
	PlayerIntentBlocked,
	PlayerIntentRejected,
	CommandFrameQueued,
	CommandRunnerRan,
	EffectApplied,
	EffectDeferred,
	InteractionApplyFailed,
	TargetToggled,
	InspectTextRequested,
	EventEmitted,
};

struct RuntimePlayerInputInteractionEffectApplyFrameReport {
	RuntimePlayerInputGatedCommandReport playerInput;
	RuntimeInteractionEffectApplyFrameResult application;
	InteractionEventRecorder2D events;
	std::vector<RuntimePlayerInputInteractionEffectApplyFrameEvent> summaryEvents;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t appliedCount = 0;
	std::size_t deferredCount = 0;
	std::size_t noOpCount = 0;
	std::size_t failedCount = 0;
	std::size_t interactionEventCount = 0;
	std::size_t targetToggledCount = 0;
	std::size_t inspectTextRequestedCount = 0;
	std::size_t eventEmittedCount = 0;
	bool mutated = false;

	[[nodiscard]] bool hasInteractionEvents() const;
};

} // namespace iggy::runtime
