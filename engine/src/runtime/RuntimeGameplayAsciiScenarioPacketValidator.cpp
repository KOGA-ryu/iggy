#include "runtime/RuntimeGameplayAsciiScenarioPacketValidator.hpp"

namespace iggy::runtime {
namespace {

bool BuiltInMarker(char marker)
{
	return marker == '.' || marker == '#' || marker == ' ';
}

bool MarkerKindKnown(RuntimeGameplayAsciiScenarioMarkerKind kind)
{
	return kind != RuntimeGameplayAsciiScenarioMarkerKind::Unknown;
}

bool MarkerRequiresActor(RuntimeGameplayAsciiScenarioMarkerKind kind)
{
	return kind == RuntimeGameplayAsciiScenarioMarkerKind::Actor;
}

bool MarkerDeclared(
	const RuntimeGameplayAsciiScenarioPacket &packet,
	char marker)
{
	for (const RuntimeGameplayAsciiScenarioMarkerDeclaration &declaration : packet.markers) {
		if (declaration.marker == marker)
			return true;
	}
	return false;
}

void AddIssue(
	RuntimeGameplayAsciiScenarioPacketValidationResult &result,
	RuntimeGameplayAsciiScenarioPacketIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyRows:
		++result.emptyRowsCount;
		break;
	case RuntimeGameplayAsciiScenarioPacketIssueCode::RaggedRow:
		++result.raggedRowCount;
		break;
	case RuntimeGameplayAsciiScenarioPacketIssueCode::DuplicateMarker:
		++result.duplicateMarkerCount;
		break;
	case RuntimeGameplayAsciiScenarioPacketIssueCode::UnknownMarkerKind:
		++result.unknownMarkerKindCount;
		break;
	case RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyActorId:
		++result.emptyActorIdCount;
		break;
	case RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyProfileId:
		++result.emptyProfileIdCount;
		break;
	case RuntimeGameplayAsciiScenarioPacketIssueCode::UnknownGridMarker:
		++result.unknownGridMarkerCount;
		break;
	}
	result.issues.push_back(issue);
}

void ValidateRowsShape(RuntimeGameplayAsciiScenarioPacketValidationResult &result)
{
	if (result.packet.rows.empty()) {
		RuntimeGameplayAsciiScenarioPacketIssue issue;
		issue.code = RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyRows;
		AddIssue(result, issue);
		return;
	}

	result.width = result.packet.rows.front().size();
	for (std::size_t rowIndex = 1; rowIndex < result.packet.rows.size(); ++rowIndex) {
		if (result.packet.rows[rowIndex].size() == result.width)
			continue;
		RuntimeGameplayAsciiScenarioPacketIssue issue;
		issue.code = RuntimeGameplayAsciiScenarioPacketIssueCode::RaggedRow;
		issue.rowIndex = rowIndex;
		AddIssue(result, issue);
	}
}

void ValidateMarkerDeclarations(RuntimeGameplayAsciiScenarioPacketValidationResult &result)
{
	const std::vector<RuntimeGameplayAsciiScenarioMarkerDeclaration> &markers =
		result.packet.markers;
	for (std::size_t markerIndex = 0; markerIndex < markers.size(); ++markerIndex) {
		const RuntimeGameplayAsciiScenarioMarkerDeclaration &declaration =
			markers[markerIndex];

		for (std::size_t first = 0; first < markerIndex; ++first) {
			if (markers[first].marker != declaration.marker)
				continue;
			RuntimeGameplayAsciiScenarioPacketIssue issue;
			issue.code = RuntimeGameplayAsciiScenarioPacketIssueCode::DuplicateMarker;
			issue.markerIndex = markerIndex;
			issue.firstMarkerIndex = first;
			issue.marker = declaration.marker;
			issue.declaration = declaration;
			AddIssue(result, issue);
			break;
		}

		if (!MarkerKindKnown(declaration.kind)) {
			RuntimeGameplayAsciiScenarioPacketIssue issue;
			issue.code = RuntimeGameplayAsciiScenarioPacketIssueCode::UnknownMarkerKind;
			issue.markerIndex = markerIndex;
			issue.marker = declaration.marker;
			issue.declaration = declaration;
			AddIssue(result, issue);
		}

		if (MarkerRequiresActor(declaration.kind) && declaration.actorId.empty()) {
			RuntimeGameplayAsciiScenarioPacketIssue issue;
			issue.code = RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyActorId;
			issue.markerIndex = markerIndex;
			issue.marker = declaration.marker;
			issue.declaration = declaration;
			AddIssue(result, issue);
		}

		if (MarkerRequiresActor(declaration.kind) && declaration.profileId.empty()) {
			RuntimeGameplayAsciiScenarioPacketIssue issue;
			issue.code = RuntimeGameplayAsciiScenarioPacketIssueCode::EmptyProfileId;
			issue.markerIndex = markerIndex;
			issue.marker = declaration.marker;
			issue.declaration = declaration;
			AddIssue(result, issue);
		}
	}
}

void ValidateGridMarkers(RuntimeGameplayAsciiScenarioPacketValidationResult &result)
{
	for (std::size_t rowIndex = 0; rowIndex < result.packet.rows.size(); ++rowIndex) {
		const std::string &row = result.packet.rows[rowIndex];
		for (std::size_t columnIndex = 0; columnIndex < row.size(); ++columnIndex) {
			const char marker = row[columnIndex];
			if (BuiltInMarker(marker) || MarkerDeclared(result.packet, marker))
				continue;
			RuntimeGameplayAsciiScenarioPacketIssue issue;
			issue.code = RuntimeGameplayAsciiScenarioPacketIssueCode::UnknownGridMarker;
			issue.rowIndex = rowIndex;
			issue.columnIndex = columnIndex;
			issue.marker = marker;
			AddIssue(result, issue);
		}
	}
}

} // namespace

bool RuntimeGameplayAsciiScenarioPacketValidationResult::ok() const
{
	return status == RuntimeGameplayAsciiScenarioPacketValidationStatus::Valid;
}

RuntimeGameplayAsciiScenarioPacketValidationResult
RuntimeGameplayAsciiScenarioPacketValidator::validate(
	const RuntimeGameplayAsciiScenarioPacket &packet) const
{
	RuntimeGameplayAsciiScenarioPacketValidationResult result;
	result.packet = packet;
	result.rowCount = packet.rows.size();
	result.markerCount = packet.markers.size();

	ValidateRowsShape(result);
	ValidateMarkerDeclarations(result);
	ValidateGridMarkers(result);

	result.issueCount = result.issues.size();
	result.status = result.issues.empty()
		? RuntimeGameplayAsciiScenarioPacketValidationStatus::Valid
		: RuntimeGameplayAsciiScenarioPacketValidationStatus::Invalid;
	return result;
}

} // namespace iggy::runtime
