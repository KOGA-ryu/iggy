#pragma once

#include "runtime/RuntimePlayerInputCommandReport.hpp"
#include "runtime/RuntimePlayerInputCommandReporter.hpp"
#include "runtime/RuntimePlayerInputCommandRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimePlayerInputFrameStepInput {
	RuntimePlayerInputCommandRunnerInput commandInput;
};

struct RuntimePlayerInputFrameStepResult {
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
	RuntimePlayerInputCommandRunnerResult command;
	RuntimePlayerInputCommandReport report;
};

struct RuntimePlayerInputGatedFrameStepInput {
	RuntimePlayerInputGatedCommandRunnerInput commandInput;
};

struct RuntimePlayerInputGatedFrameStepResult {
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
	RuntimePlayerInputGatedCommandRunnerResult command;
	RuntimePlayerInputGatedCommandReport report;
};

class RuntimePlayerInputFrameStep {
public:
	[[nodiscard]] RuntimePlayerInputFrameStepResult run(const RuntimePlayerInputFrameStepInput &input) const;

	[[nodiscard]] RuntimePlayerInputFrameStepResult run(
		const RuntimePlayerInputFrameStepInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;

	[[nodiscard]] RuntimePlayerInputGatedFrameStepResult runGated(const RuntimePlayerInputGatedFrameStepInput &input) const;

	[[nodiscard]] RuntimePlayerInputGatedFrameStepResult runGated(
		const RuntimePlayerInputGatedFrameStepInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
