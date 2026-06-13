#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimePlayerInputCommandRunner.hpp"

namespace iggy::runtime {

enum class RuntimePlayerInputCommandEvent {
	IntakeQueued,
	QueueRejected,
	IntentMapped,
	IntentBlocked,
	IntentRejected,
	CommandFrameQueued,
	CommandRunnerRan,
	CommandRunnerSkipped,
};

struct RuntimePlayerInputCommandReport {
	RuntimePlayerInputCommandRunnerStatus status = RuntimePlayerInputCommandRunnerStatus::Ran;
	RuntimePlayerInputQueueResult intake;
	RuntimeQueuedCommandRunnerResult runner;
	std::vector<RuntimePlayerInputCommandEvent> events;
	std::size_t acceptedCommandCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t queuedFrameCount = 0;
	std::size_t tickResultCount = 0;
};

struct RuntimePlayerInputGatedCommandReport {
	RuntimePlayerInputCommandRunnerStatus status = RuntimePlayerInputCommandRunnerStatus::Ran;
	RuntimePlayerInputQueueGatedResult intake;
	RuntimeQueuedCommandRunnerResult runner;
	std::vector<RuntimePlayerInputCommandEvent> events;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t queuedFrameCount = 0;
	std::size_t tickResultCount = 0;
};

} // namespace iggy::runtime
