#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeInteractionEffectCommandFrameStep.hpp"
#include "runtime/RuntimePlayerInputCommandReport.hpp"

namespace iggy::runtime {

enum class RuntimePlayerInputInteractionEffectFrameEvent {
	PlayerCommandAccepted,
	PlayerIntentBlocked,
	PlayerIntentRejected,
	CommandFrameQueued,
	CommandRunnerRan,
	InteractionReadyWithEffects,
	InteractionReadyWithoutEffects,
	InteractionBlocked,
	EffectsRequested,
};

struct RuntimePlayerInputInteractionEffectFrameReport {
	RuntimePlayerInputGatedCommandReport playerInput;
	RuntimeInteractionEffectCommandFrameResult interactions;
	std::vector<RuntimePlayerInputInteractionEffectFrameEvent> events;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t readyInteractionCount = 0;
	std::size_t noEffectInteractionCount = 0;
	std::size_t blockedInteractionCount = 0;
	std::size_t requestedEffectCount = 0;

	[[nodiscard]] bool hasInteractions() const;
	[[nodiscard]] bool hasRequestedEffects() const;
};

} // namespace iggy::runtime
