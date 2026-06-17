#include "runtime/RuntimeGameplayAuthoringDiagnostics.hpp"

#include "runtime/RuntimeGameplayAuthoringErrorCodes.hpp"
#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"

namespace iggy::runtime {
namespace {

void SetIdIfPresent(
	RuntimeGameplayAuthoringDiagnosticEntry &entry,
	const ResourceId &id)
{
	if (entry.id.empty() && !id.empty())
		entry.id = id;
}

RuntimeGameplayAuthoringDiagnosticEntry SourcePlanEntry(
	const RuntimeGameplayAsciiSourcePlanIssue &issue)
{
	RuntimeGameplayAuthoringDiagnosticEntry entry;
	entry.layer = RuntimeGameplayAuthoringDiagnosticLayer::SourcePlan;
	entry.code = runtimeGameplayAuthoringCodeText(issue.code);
	entry.hasIndex = true;
	entry.index = issue.index;
	entry.hasRow = true;
	entry.row = issue.row;
	entry.hasColumn = true;
	entry.column = issue.column;
	entry.glyph = issue.glyph;
	SetIdIfPresent(entry, issue.id);
	return entry;
}

RuntimeGameplayAuthoringDiagnosticEntry ProfileEntry(
	const RuntimeGameplayProfileScenarioIssue &issue)
{
	RuntimeGameplayAuthoringDiagnosticEntry entry;
	entry.layer = RuntimeGameplayAuthoringDiagnosticLayer::Profile;
	entry.code = runtimeGameplayAuthoringCodeText(issue.code);
	entry.hasIndex = true;
	entry.index = issue.frameIndex;
	entry.hasActorIndex = true;
	entry.actorIndex = issue.actorIndex;
	SetIdIfPresent(entry, issue.profileId);
	SetIdIfPresent(entry, issue.npcId);
	return entry;
}

RuntimeGameplayAuthoringDiagnosticEntry ScenarioEntry(
	const RuntimeGameplayScenarioIssue &issue)
{
	RuntimeGameplayAuthoringDiagnosticEntry entry;
	entry.layer = RuntimeGameplayAuthoringDiagnosticLayer::Scenario;
	entry.code = runtimeGameplayAuthoringCodeText(issue.code);
	entry.hasIndex = true;
	entry.index = issue.frameIndex;
	entry.hasActorIndex = true;
	entry.actorIndex = issue.actorIndex;
	entry.hasControlIndex = true;
	entry.controlIndex = issue.controlIndex;
	entry.hasSubjectIndex = true;
	entry.subjectIndex = issue.subjectIndex;
	SetIdIfPresent(entry, issue.npcId);
	return entry;
}

void AppendProfileIssue(
	std::vector<RuntimeGameplayAuthoringDiagnosticEntry> &entries,
	const RuntimeGameplayProfileScenarioIssue &issue)
{
	entries.push_back(ProfileEntry(issue));
	if (issue.code == RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid)
		entries.push_back(ScenarioEntry(issue.nestedIssue));
}

void AppendReadDiagnostics(
	std::vector<RuntimeGameplayAuthoringDiagnosticEntry> &entries,
	const RuntimeGameplayAsciiSourcePlanTomlFileReadResult &read)
{
	for (const RuntimeGameplayAsciiSourcePlanTomlFileReadIssue &issue :
		read.issues) {
		RuntimeGameplayAuthoringDiagnosticEntry entry;
		entry.layer = RuntimeGameplayAuthoringDiagnosticLayer::File;
		entry.code = runtimeGameplayAuthoringCodeText(issue.code);
		entry.detail = issue.detail;
		entries.push_back(entry);
	}

	for (const RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue :
		read.text.issues) {
		RuntimeGameplayAuthoringDiagnosticEntry entry;
		entry.layer = RuntimeGameplayAuthoringDiagnosticLayer::Toml;
		entry.code = runtimeGameplayAuthoringCodeText(issue.code);
		entry.table = issue.table;
		entry.key = issue.key;
		entry.hasTableIndex = issue.hasTableIndex;
		entry.tableIndex = issue.tableIndex;
		entry.hasLine = true;
		entry.line = issue.line;
		entry.hasColumn = true;
		entry.column = issue.column;
		entry.detail = issue.detail;
		entries.push_back(entry);

		if (issue.code ==
			RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid)
			entries.push_back(SourcePlanEntry(issue.sourceIssue));
	}
}

void AppendConversionDiagnostics(
	std::vector<RuntimeGameplayAuthoringDiagnosticEntry> &entries,
	const RuntimeGameplayScenarioAuthoringAdapterResult &adapter)
{
	for (const RuntimeGameplayScenarioAuthoringAdapterIssue &issue :
		adapter.issues) {
		RuntimeGameplayAuthoringDiagnosticEntry entry;
		entry.layer = RuntimeGameplayAuthoringDiagnosticLayer::Adapter;
		entry.code = runtimeGameplayAuthoringCodeText(issue.code);
		entries.push_back(entry);
	}

	for (const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue &issue :
		adapter.asciiSourcePlanConversion.issues) {
		RuntimeGameplayAuthoringDiagnosticEntry entry;
		entry.layer = RuntimeGameplayAuthoringDiagnosticLayer::Conversion;
		entry.code = runtimeGameplayAuthoringCodeText(issue.code);
		entry.hasRow = true;
		entry.row = issue.row;
		entry.hasColumn = true;
		entry.column = issue.column;
		entry.glyph = issue.glyph;
		SetIdIfPresent(entry, issue.authoredControl.npcId);
		SetIdIfPresent(entry, issue.authoredPlayerCommand.targetId);
		SetIdIfPresent(entry, issue.profileTraitIssue.entry.profileId);
		SetIdIfPresent(entry, issue.interactionTargetIssue.target.id);
		SetIdIfPresent(entry, issue.interactionEffectIssue.entry.targetId);
		SetIdIfPresent(entry, issue.itemDropIssue.drop.id);
		entries.push_back(entry);

		if (issue.code ==
			RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid)
			entries.push_back(SourcePlanEntry(issue.sourceIssue));
		if (issue.code ==
			RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ProfileScenarioInvalid)
			AppendProfileIssue(entries, issue.profileIssue);
	}
}

void AppendValidationDiagnostics(
	std::vector<RuntimeGameplayAuthoringDiagnosticEntry> &entries,
	const RuntimeGameplayProfileScenarioValidationResult &validation)
{
	for (const RuntimeGameplayProfileScenarioIssue &issue : validation.issues)
		AppendProfileIssue(entries, issue);
}

} // namespace

const char *runtimeGameplayAuthoringDiagnosticLayerText(
	RuntimeGameplayAuthoringDiagnosticLayer layer)
{
	using Layer = RuntimeGameplayAuthoringDiagnosticLayer;
	switch (layer) {
	case Layer::File:
		return "file";
	case Layer::Toml:
		return "toml";
	case Layer::SourcePlan:
		return "source_plan";
	case Layer::Adapter:
		return "adapter";
	case Layer::Conversion:
		return "conversion";
	case Layer::Profile:
		return "profile";
	case Layer::Scenario:
		return "scenario";
	case Layer::Run:
		return "run";
	}
	return "unknown";
}

std::vector<RuntimeGameplayAuthoringDiagnosticEntry>
projectRuntimeGameplayAuthoringDiagnostics(
	const RuntimeGameplayTomlScenarioFacadeResult &result)
{
	std::vector<RuntimeGameplayAuthoringDiagnosticEntry> entries;
	AppendReadDiagnostics(entries, result.read);
	AppendConversionDiagnostics(entries, result.adapter);
	if (result.status == RuntimeGameplayTomlScenarioFacadeStatus::LintFailed
		|| result.status == RuntimeGameplayTomlScenarioFacadeStatus::RunFailed)
		AppendValidationDiagnostics(entries, result.validation);
	return entries;
}

} // namespace iggy::runtime
