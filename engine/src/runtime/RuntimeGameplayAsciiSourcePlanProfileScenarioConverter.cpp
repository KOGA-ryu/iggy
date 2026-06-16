#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"

namespace iggy::runtime {
namespace {

void AddIssue(
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid:
		++result.sourcePlanIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::MissingFrameDefaults:
		++result.missingFrameDefaultsCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ProfileScenarioInvalid:
		++result.profileScenarioIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::UnsupportedTerrainPromotion:
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ActorRegistryInvalid:
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ControlRegistryInvalid:
		break;
	}
	result.issues.push_back(issue);
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue SourceIssue(
	const RuntimeGameplayAsciiSourcePlanIssue &sourceIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid;
	issue.sourceIssue = sourceIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue MissingFrameDefaultsIssue()
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::MissingFrameDefaults;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue ProfileIssue(
	const RuntimeGameplayProfileScenarioIssue &profileIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ProfileScenarioInvalid;
	issue.profileIssue = profileIssue;
	return issue;
}

RuntimeGameplayProfileScenarioDefinition BuildDefinition(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config)
{
	RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = sourcePlan.hasSourceId;
	definition.scenarioId = sourcePlan.sourceId;
	definition.profileTraits = config.profileTraits;
	definition.frames.push_back(config.defaultFrame);
	return definition;
}

} // namespace

bool RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult::ok() const
{
	return status == RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::Converted;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult
RuntimeGameplayAsciiSourcePlanProfileScenarioConverter::convert(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config) const
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result;
	result.sourcePlan = sourcePlan;
	result.config = config;
	result.sourceValidation = RuntimeGameplayAsciiSourcePlanValidator {}.validate(sourcePlan);

	if (!result.sourceValidation.ok()) {
		for (const RuntimeGameplayAsciiSourcePlanIssue &issue : result.sourceValidation.issues) {
			AddIssue(result, SourceIssue(issue));
		}
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::SourcePlanInvalid;
		return result;
	}

	if (!config.hasDefaultFrame) {
		AddIssue(result, MissingFrameDefaultsIssue());
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::MissingFrameDefaults;
		return result;
	}

	result.definition = BuildDefinition(sourcePlan, config);
	result.profileValidation = RuntimeGameplayProfileScenarioValidator {}.validate(result.definition);
	if (!result.profileValidation.ok()) {
		for (const RuntimeGameplayProfileScenarioIssue &issue : result.profileValidation.issues) {
			AddIssue(result, ProfileIssue(issue));
		}
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ProfileScenarioInvalid;
		return result;
	}

	result.converted = true;
	result.issueCount = result.issues.size();
	result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::Converted;
	return result;
}

} // namespace iggy::runtime
