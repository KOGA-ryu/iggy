#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"

#include "runtime/RuntimeGameplayAuthoringDiagnostics.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanFinalDebugRows.hpp"

namespace iggy::runtime {
namespace {

std::string IdText(const ResourceId &id)
{
	return std::string(id.value());
}

std::string FrameIdText(
	const RuntimeGameplayProfileScenarioRunResult &run,
	std::size_t frameIndex)
{
	if (frameIndex < run.definition.frames.size()) {
		const RuntimeGameplayProfileScenarioFrameDefinition &frame =
			run.definition.frames[frameIndex];
		if (frame.hasFrameId && !frame.frameId.empty())
			return IdText(frame.frameId);
	}
	return "<none>";
}

std::vector<RuntimeGameplayTomlScenarioTraceFrame> TraceFrames(
	const RuntimeGameplayAsciiSourcePlan &plan,
	const RuntimeGameplayProfileScenarioRunResult &run)
{
	std::vector<RuntimeGameplayTomlScenarioTraceFrame> frames;
	frames.reserve(run.scenario.runner.frameResults.size());
	for (std::size_t index = 0; index < run.scenario.runner.frameResults.size();
		++index) {
		const RuntimeGameplayOrchestratedFrameResult &frameResult =
			run.scenario.runner.frameResults[index];
		RuntimeGameplayTomlScenarioTraceFrame frame;
		frame.index = index;
		frame.frameId = FrameIdText(run, index);
		frame.acceptedCommandCount = frameResult.acceptedCommandCount;
		frame.pickedUpCount = frameResult.pickedUpCount;
		frame.interactionChanged = frameResult.interactionChanged;
		frame.npcMovedCount = frameResult.npcMovedCount;
		frame.rows = finalDebugRowsForAsciiSourcePlan(plan, frameResult.state);
		frames.push_back(frame);
	}
	return frames;
}

RuntimeGameplayTomlScenarioExpectationComparison CompareExpectations(
	const RuntimeGameplayAsciiSourcePlanExpectations &expectations,
	const RuntimeGameplayProfileScenarioRunResult &run,
	const std::vector<std::string> &finalRows)
{
	RuntimeGameplayTomlScenarioExpectationComparison comparison;
	comparison.present = expectations.hasAny();
	if (!comparison.present)
		return comparison;

	if (expectations.hasFinalRows) {
		comparison.checkedFinalRows = true;
		comparison.finalRowsMatched = expectations.finalRows == finalRows;
		comparison.matched = comparison.matched && comparison.finalRowsMatched;
	}
	if (expectations.hasFrameCount) {
		comparison.checkedFrameCount = true;
		comparison.frameCountMatched = expectations.frameCount == run.frameCount;
		comparison.matched = comparison.matched && comparison.frameCountMatched;
	}
	if (expectations.hasAcceptedCommandCount) {
		comparison.checkedAcceptedCommandCount = true;
		comparison.acceptedCommandCountMatched =
			expectations.acceptedCommandCount ==
			run.scenario.runner.acceptedCommandCount;
		comparison.matched =
			comparison.matched && comparison.acceptedCommandCountMatched;
	}
	if (expectations.hasPickedUpCount) {
		comparison.checkedPickedUpCount = true;
		comparison.pickedUpCountMatched =
			expectations.pickedUpCount == run.scenario.runner.pickedUpCount;
		comparison.matched = comparison.matched && comparison.pickedUpCountMatched;
	}
	if (expectations.hasInteractionChanged) {
		comparison.checkedInteractionChanged = true;
		comparison.interactionChangedMatched =
			expectations.interactionChanged ==
			run.scenario.runner.interactionChanged;
		comparison.matched =
			comparison.matched && comparison.interactionChangedMatched;
	}
	if (expectations.hasNpcMovedCount) {
		comparison.checkedNpcMovedCount = true;
		comparison.npcMovedCountMatched =
			expectations.npcMovedCount == run.npcMovedCount;
		comparison.matched = comparison.matched && comparison.npcMovedCountMatched;
	}

	return comparison;
}

bool ShouldLint(const RuntimeGameplayTomlScenarioFacadeConfig &config)
{
	return config.mode == RuntimeGameplayTomlScenarioFacadeMode::Lint
		|| config.lintOnly;
}

bool ShouldCheck(const RuntimeGameplayTomlScenarioFacadeConfig &config)
{
	return config.mode == RuntimeGameplayTomlScenarioFacadeMode::Check;
}

bool ShouldTrace(const RuntimeGameplayTomlScenarioFacadeConfig &config)
{
	return config.mode == RuntimeGameplayTomlScenarioFacadeMode::Trace
		|| config.captureTraceFrames;
}

} // namespace

bool RuntimeGameplayTomlScenarioFacadeResult::ok() const
{
	return status == RuntimeGameplayTomlScenarioFacadeStatus::Ran
		|| status == RuntimeGameplayTomlScenarioFacadeStatus::LintOk
		|| status == RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed;
}

bool RuntimeGameplayTomlScenarioFacadeResult::ran() const
{
	return status == RuntimeGameplayTomlScenarioFacadeStatus::Ran
		|| status == RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed
		|| status == RuntimeGameplayTomlScenarioFacadeStatus::CheckFailed;
}

bool RuntimeGameplayTomlScenarioFacadeResult::linted() const
{
	return status == RuntimeGameplayTomlScenarioFacadeStatus::LintOk
		|| status == RuntimeGameplayTomlScenarioFacadeStatus::LintFailed;
}

bool RuntimeGameplayTomlScenarioFacadeResult::checked() const
{
	return status == RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed
		|| status == RuntimeGameplayTomlScenarioFacadeStatus::CheckFailed;
}

RuntimeGameplayTomlScenarioFacadeResult RuntimeGameplayTomlScenarioFacade::execute(
	const std::filesystem::path &path,
	const RuntimeGameplayTomlScenarioFacadeConfig &config) const
{
	RuntimeGameplayTomlScenarioFacadeResult result;
	result.path = path;
	result.config = config;

	result.read = RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	if (!result.read.ok()) {
		result.status = RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed;
		result.diagnostics = projectRuntimeGameplayAuthoringDiagnostics(result);
		return result;
	}

	RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = result.read.text.plan;

	result.adapter = RuntimeGameplayScenarioAuthoringAdapter {}.convert(packet);
	if (!result.adapter.ok()) {
		result.status = RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed;
		result.diagnostics = projectRuntimeGameplayAuthoringDiagnostics(result);
		return result;
	}

	if (ShouldLint(config)) {
		result.validation = RuntimeGameplayProfileScenarioValidator {}.validate(
			result.adapter.profileScenario);
		result.status = result.validation.ok()
			? RuntimeGameplayTomlScenarioFacadeStatus::LintOk
			: RuntimeGameplayTomlScenarioFacadeStatus::LintFailed;
		result.diagnostics = projectRuntimeGameplayAuthoringDiagnostics(result);
		return result;
	}

	result.run = RuntimeGameplayProfileScenarioRunner {}.run(
		result.adapter.profileScenario);
	result.validation = result.run.validation;
	if (!result.run.ran()) {
		result.status = RuntimeGameplayTomlScenarioFacadeStatus::RunFailed;
		result.diagnostics = projectRuntimeGameplayAuthoringDiagnostics(result);
		return result;
	}

	result.finalRows =
		finalDebugRowsForAsciiSourcePlan(result.read.text.plan, result.run.state);
	result.runSummary = projectRuntimeGameplayTomlScenarioRunSummary(
		result.path,
		result.run,
		result.finalRows);
	if (ShouldTrace(config))
		result.traceFrames = TraceFrames(result.read.text.plan, result.run);
	result.expectationComparison = CompareExpectations(
		result.read.text.plan.expectations,
		result.run,
		result.finalRows);
	if (ShouldCheck(config)) {
		result.status = result.expectationComparison.present
			&& result.expectationComparison.matched
			? RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed
			: RuntimeGameplayTomlScenarioFacadeStatus::CheckFailed;
		return result;
	}
	result.status = RuntimeGameplayTomlScenarioFacadeStatus::Ran;
	return result;
}

} // namespace iggy::runtime
