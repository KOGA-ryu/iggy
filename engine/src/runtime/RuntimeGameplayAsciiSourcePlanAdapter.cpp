#include "runtime/RuntimeGameplayAsciiSourcePlanAdapter.hpp"

namespace iggy::runtime {
namespace {

const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *FindLegendEntry(
	const RuntimeGameplayAsciiSourcePlan &plan,
	char glyph)
{
	for (const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry : plan.legend) {
		if (entry.glyph == glyph) {
			return &entry;
		}
	}
	return nullptr;
}

void AddIssue(
	RuntimeGameplayAsciiSourcePlanAdapterResult &result,
	RuntimeGameplayAsciiSourcePlanAdapterIssue issue)
{
	result.issues.push_back(issue);
	result.issueCount = result.issues.size();
}

bool AddMarker(
	RuntimeGameplayAsciiSourcePlanAdapterResult &result,
	RuntimeGameplayAsciiScenarioMarkerDeclaration marker,
	std::size_t sourceIndex,
	std::size_t row,
	std::size_t column)
{
	for (std::size_t index = 0; index < result.packet.markers.size(); ++index) {
		if (result.packet.markers[index].marker == marker.marker) {
			RuntimeGameplayAsciiSourcePlanAdapterIssue issue;
			issue.code =
				RuntimeGameplayAsciiSourcePlanAdapterIssueCode::DuplicateScenarioMarkerGlyph;
			issue.index = sourceIndex;
			issue.firstIndex = index;
			issue.row = row;
			issue.column = column;
			issue.glyph = marker.marker;
			AddIssue(result, issue);
			return false;
		}
	}

	result.packet.markers.push_back(marker);
	result.markerCount = result.packet.markers.size();
	return true;
}

void ConvertLegendMappings(RuntimeGameplayAsciiSourcePlanAdapterResult &result)
{
	for (std::size_t index = 0; index < result.sourcePlan.legend.size(); ++index) {
		const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry =
			result.sourcePlan.legend[index];
		if (!entry.mapsToScenarioMarker ||
			entry.scenarioMarkerKind == RuntimeGameplayAsciiScenarioMarkerKind::Actor) {
			continue;
		}

		RuntimeGameplayAsciiScenarioMarkerDeclaration marker;
		marker.marker = entry.glyph;
		marker.kind = entry.scenarioMarkerKind;
		marker.actorId = entry.targetMarkerId;
		marker.profileId = entry.targetProfileId;
		marker.present = true;
		AddMarker(result, marker, index, 0, 0);
	}
}

void ConvertAnnotatedActorCells(RuntimeGameplayAsciiSourcePlanAdapterResult &result)
{
	for (std::size_t index = 0; index < result.sourcePlan.annotatedCells.size(); ++index) {
		const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell =
			result.sourcePlan.annotatedCells[index];
		const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *entry =
			FindLegendEntry(result.sourcePlan, cell.glyph);
		if (entry == nullptr ||
			!entry->mapsToScenarioMarker ||
			entry->scenarioMarkerKind != RuntimeGameplayAsciiScenarioMarkerKind::Actor) {
			if (!cell.markerId.empty() || !cell.profileId.empty()) {
				RuntimeGameplayAsciiSourcePlanAdapterIssue issue;
				issue.code =
					RuntimeGameplayAsciiSourcePlanAdapterIssueCode::UnsupportedAnnotatedCellMapping;
				issue.index = index;
				issue.row = cell.row;
				issue.column = cell.column;
				issue.glyph = cell.glyph;
				AddIssue(result, issue);
			}
			continue;
		}

		if (cell.markerId.empty()) {
			RuntimeGameplayAsciiSourcePlanAdapterIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanAdapterIssueCode::MissingActorMarkerId;
			issue.index = index;
			issue.row = cell.row;
			issue.column = cell.column;
			issue.glyph = cell.glyph;
			AddIssue(result, issue);
			continue;
		}

		if (cell.profileId.empty()) {
			RuntimeGameplayAsciiSourcePlanAdapterIssue issue;
			issue.code = RuntimeGameplayAsciiSourcePlanAdapterIssueCode::MissingProfileId;
			issue.index = index;
			issue.row = cell.row;
			issue.column = cell.column;
			issue.glyph = cell.glyph;
			AddIssue(result, issue);
			continue;
		}

		RuntimeGameplayAsciiScenarioMarkerDeclaration marker;
		marker.marker = cell.glyph;
		marker.kind = RuntimeGameplayAsciiScenarioMarkerKind::Actor;
		marker.actorId = cell.markerId;
		marker.profileId = cell.profileId;
		marker.present = true;
		AddMarker(result, marker, index, cell.row, cell.column);
	}
}

} // namespace

bool RuntimeGameplayAsciiSourcePlanAdapterResult::converted() const
{
	return status == RuntimeGameplayAsciiSourcePlanAdapterStatus::Converted;
}

RuntimeGameplayAsciiSourcePlanAdapterResult
RuntimeGameplayAsciiSourcePlanToPacketAdapter::convert(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan) const
{
	RuntimeGameplayAsciiSourcePlanAdapterResult result;
	result.sourcePlan = sourcePlan;
	result.validation =
		RuntimeGameplayAsciiSourcePlanValidator {}.validate(sourcePlan);

	if (!result.validation.ok()) {
		RuntimeGameplayAsciiSourcePlanAdapterIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanAdapterIssueCode::SourcePlanInvalid;
		AddIssue(result, issue);
		result.status = RuntimeGameplayAsciiSourcePlanAdapterStatus::SourcePlanInvalid;
		return result;
	}

	result.packet.hasSourceId = sourcePlan.hasSourceId;
	result.packet.sourceId = sourcePlan.sourceId;
	result.packet.rows = sourcePlan.grid.rows;
	result.rowCount = result.packet.rows.size();

	ConvertLegendMappings(result);
	ConvertAnnotatedActorCells(result);

	result.status = result.issues.empty()
		? RuntimeGameplayAsciiSourcePlanAdapterStatus::Converted
		: RuntimeGameplayAsciiSourcePlanAdapterStatus::Unmappable;
	result.issueCount = result.issues.size();
	result.markerCount = result.packet.markers.size();
	return result;
}

} // namespace iggy::runtime
