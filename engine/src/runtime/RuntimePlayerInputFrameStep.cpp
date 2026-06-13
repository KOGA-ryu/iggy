#include "runtime/RuntimePlayerInputFrameStep.hpp"

namespace iggy::runtime {

RuntimePlayerInputFrameStepResult RuntimePlayerInputFrameStep::run(const RuntimePlayerInputFrameStepInput &input) const
{
	RuntimePlayerInputFrameStepResult result;
	result.command = RuntimePlayerInputCommandRunner {}.run(input.commandInput);
	result.report = RuntimePlayerInputCommandReporter {}.report(result.command);
	result.session = result.command.session;
	result.queue = result.command.queue;
	return result;
}

RuntimePlayerInputFrameStepResult RuntimePlayerInputFrameStep::run(
	const RuntimePlayerInputFrameStepInput &input,
	const physics2d::CollisionWorld2D &explicitWorld) const
{
	RuntimePlayerInputFrameStepResult result;
	result.command = RuntimePlayerInputCommandRunner {}.run(input.commandInput, explicitWorld);
	result.report = RuntimePlayerInputCommandReporter {}.report(result.command);
	result.session = result.command.session;
	result.queue = result.command.queue;
	return result;
}

} // namespace iggy::runtime
