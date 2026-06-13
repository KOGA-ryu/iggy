#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimePlayerInputCommandRunner.hpp"

namespace iggy::runtime {

enum class RuntimePlayerInputCommandEvent {
	IntakeQueued,
	QueueRejected,
	IntentMapped,
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

} // namespace iggy::runtime
