#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeInteractionCommandFrameStep.hpp"
#include "runtime/RuntimePlayerInputCommandReport.hpp"

namespace iggy::runtime {

enum class RuntimePlayerInputInteractionFrameEvent {
	PlayerCommandAccepted,
	PlayerIntentBlocked,
	PlayerIntentRejected,
	CommandFrameQueued,
	CommandRunnerRan,
	InteractionReady,
	InteractionBlocked,
};

struct RuntimePlayerInputInteractionFrameReport {
	RuntimePlayerInputGatedCommandReport playerInput;
	RuntimeInteractionCommandFrameResult interactions;
	std::vector<RuntimePlayerInputInteractionFrameEvent> events;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t readyInteractionCount = 0;
	std::size_t blockedInteractionCount = 0;

	[[nodiscard]] bool hasInteractions() const;
};

} // namespace iggy::runtime
