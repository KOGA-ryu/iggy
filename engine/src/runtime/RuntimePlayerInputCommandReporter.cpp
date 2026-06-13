#include "runtime/RuntimePlayerInputCommandReporter.hpp"

namespace iggy::runtime {

RuntimePlayerInputCommandReport RuntimePlayerInputCommandReporter::report(const RuntimePlayerInputCommandRunnerResult &result) const
{
	RuntimePlayerInputCommandReport report;
	report.status = result.status;
	report.intake = result.intake;
	report.runner = result.runner;
	report.acceptedCommandCount = result.intake.mapping.frame.commands.size();
	report.rejectedIntentCount = result.intake.mapping.issues.size();

	if (report.acceptedCommandCount > 0)
		report.events.push_back(RuntimePlayerInputCommandEvent::IntentMapped);
	if (report.rejectedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputCommandEvent::IntentRejected);

	if (result.intake.status == RuntimePlayerInputQueueStatus::Queued) {
		report.queuedFrameCount = 1;
		report.events.push_back(RuntimePlayerInputCommandEvent::IntakeQueued);
		report.events.push_back(RuntimePlayerInputCommandEvent::CommandFrameQueued);
	}

	if (result.status == RuntimePlayerInputCommandRunnerStatus::QueueRejected) {
		report.events.push_back(RuntimePlayerInputCommandEvent::QueueRejected);
		report.events.push_back(RuntimePlayerInputCommandEvent::CommandRunnerSkipped);
		return report;
	}

	report.tickResultCount = result.runner.runner.ticks.size();
	report.events.push_back(RuntimePlayerInputCommandEvent::CommandRunnerRan);
	return report;
}

RuntimePlayerInputGatedCommandReport RuntimePlayerInputCommandReporter::reportGated(const RuntimePlayerInputGatedCommandRunnerResult &result) const
{
	RuntimePlayerInputGatedCommandReport report;
	report.status = result.status;
	report.intake = result.intake;
	report.runner = result.runner;
	report.acceptedCommandCount = result.intake.mapping.frame.commands.size();
	report.blockedIntentCount = result.intake.mapping.gateIssues.size();
	report.rejectedIntentCount = result.intake.mapping.mapping.issues.size();

	if (report.acceptedCommandCount > 0)
		report.events.push_back(RuntimePlayerInputCommandEvent::IntentMapped);
	if (report.blockedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputCommandEvent::IntentBlocked);
	if (report.rejectedIntentCount > 0)
		report.events.push_back(RuntimePlayerInputCommandEvent::IntentRejected);

	if (result.intake.status == RuntimePlayerInputQueueStatus::Queued) {
		report.queuedFrameCount = 1;
		report.events.push_back(RuntimePlayerInputCommandEvent::IntakeQueued);
		report.events.push_back(RuntimePlayerInputCommandEvent::CommandFrameQueued);
	}

	if (result.status == RuntimePlayerInputCommandRunnerStatus::QueueRejected) {
		report.events.push_back(RuntimePlayerInputCommandEvent::QueueRejected);
		report.events.push_back(RuntimePlayerInputCommandEvent::CommandRunnerSkipped);
		return report;
	}

	report.tickResultCount = result.runner.runner.ticks.size();
	report.events.push_back(RuntimePlayerInputCommandEvent::CommandRunnerRan);
	return report;
}

} // namespace iggy::runtime
