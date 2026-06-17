#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanFinalDebugRows.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"
#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"

namespace {

std::string IdText(const iggy::ResourceId &id)
{
	return std::string(id.value());
}

const char *ToString(
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus status)
{
	using Status = iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus;
	switch (status) {
	case Status::Parsed:
		return "parsed";
	case Status::EmptyPath:
		return "empty_path";
	case Status::MissingFile:
		return "missing_file";
	case Status::NonRegularFile:
		return "non_regular_file";
	case Status::OpenFailed:
		return "open_failed";
	case Status::ReadFailed:
		return "read_failed";
	case Status::TomlReadFailed:
		return "toml_read_failed";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus status)
{
	using Status = iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus;
	switch (status) {
	case Status::Parsed:
		return "parsed";
	case Status::SyntaxInvalid:
		return "syntax_invalid";
	case Status::TypeInvalid:
		return "type_invalid";
	case Status::UnsupportedSyntax:
		return "unsupported_syntax";
	case Status::SourcePlanInvalid:
		return "source_plan_invalid";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode code)
{
	using Code = iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode;
	switch (code) {
	case Code::EmptyPath:
		return "empty_path";
	case Code::MissingFile:
		return "missing_file";
	case Code::NonRegularFile:
		return "non_regular_file";
	case Code::OpenFailed:
		return "open_failed";
	case Code::ReadFailed:
		return "read_failed";
	case Code::TomlReadFailed:
		return "toml_read_failed";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode code)
{
	using Code = iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode;
	switch (code) {
	case Code::SyntaxError:
		return "syntax_error";
	case Code::MissingTable:
		return "missing_table";
	case Code::MissingRequiredKey:
		return "missing_required_key";
	case Code::WrongType:
		return "wrong_type";
	case Code::InvalidGlyphString:
		return "invalid_glyph_string";
	case Code::UnknownEnumValue:
		return "unknown_enum_value";
	case Code::UnsupportedNestedShape:
		return "unsupported_nested_shape";
	case Code::SourcePlanInvalid:
		return "source_plan_invalid";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode code)
{
	using Code = iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode;
	switch (code) {
	case Code::EmptyRows:
		return "empty_rows";
	case Code::GridDimensionMismatch:
		return "grid_dimension_mismatch";
	case Code::RaggedRow:
		return "ragged_row";
	case Code::EmptyGlyph:
		return "empty_glyph";
	case Code::DuplicateGlyph:
		return "duplicate_glyph";
	case Code::UnknownGridGlyph:
		return "unknown_grid_glyph";
	case Code::AnnotatedCellOutOfBounds:
		return "annotated_cell_out_of_bounds";
	case Code::AnnotatedCellGlyphMismatch:
		return "annotated_cell_glyph_mismatch";
	case Code::DuplicateAnnotatedCellId:
		return "duplicate_annotated_cell_id";
	case Code::InvalidRegionBounds:
		return "invalid_region_bounds";
	case Code::RegionOutOfBounds:
		return "region_out_of_bounds";
	case Code::AuthoredControlMissingNpcId:
		return "authored_control_missing_npc_id";
	case Code::AuthoredControlUnsupportedBehavior:
		return "authored_control_unsupported_behavior";
	case Code::AuthoredControlUnsupportedMoveMode:
		return "authored_control_unsupported_move_mode";
	case Code::AuthoredControlMissingTarget:
		return "authored_control_missing_target";
	case Code::AuthoredProfileMissingId:
		return "authored_profile_missing_id";
	case Code::AuthoredProfileDuplicateId:
		return "authored_profile_duplicate_id";
	case Code::AuthoredProfileInvalidTraits:
		return "authored_profile_invalid_traits";
	case Code::AuthoredInteractionTargetMissingId:
		return "authored_interaction_target_missing_id";
	case Code::AuthoredInteractionTargetDuplicateId:
		return "authored_interaction_target_duplicate_id";
	case Code::AuthoredInteractionTargetUnsupportedKind:
		return "authored_interaction_target_unsupported_kind";
	case Code::AuthoredInteractionTargetMissingPosition:
		return "authored_interaction_target_missing_position";
	case Code::AuthoredInteractionTargetPositionOutOfBounds:
		return "authored_interaction_target_position_out_of_bounds";
	case Code::AuthoredInteractionTargetInvalidRadius:
		return "authored_interaction_target_invalid_radius";
	case Code::AuthoredInteractionTargetUnsupportedEffect:
		return "authored_interaction_target_unsupported_effect";
	case Code::AuthoredItemDropMissingDropId:
		return "authored_item_drop_missing_drop_id";
	case Code::AuthoredItemDropDuplicateDropId:
		return "authored_item_drop_duplicate_drop_id";
	case Code::AuthoredItemDropMissingItemId:
		return "authored_item_drop_missing_item_id";
	case Code::AuthoredItemDropInvalidCount:
		return "authored_item_drop_invalid_count";
	case Code::AuthoredItemDropMissingPosition:
		return "authored_item_drop_missing_position";
	case Code::AuthoredItemDropPositionOutOfBounds:
		return "authored_item_drop_position_out_of_bounds";
	case Code::AuthoredItemDropInvalidPickupRadius:
		return "authored_item_drop_invalid_pickup_radius";
	case Code::AuthoredPlayerCommandUnsupportedCommand:
		return "authored_player_command_unsupported_command";
	case Code::AuthoredPlayerCommandMissingTarget:
		return "authored_player_command_missing_target";
	case Code::AuthoredPlayerCommandUnknownInteractionTarget:
		return "authored_player_command_unknown_interaction_target";
	case Code::AuthoredPlayerCommandInvalidPickupTarget:
		return "authored_player_command_invalid_pickup_target";
	case Code::ExpectedFinalRowsEmpty:
		return "expected_final_rows_empty";
	case Code::ExpectedFinalRowsDimensionMismatch:
		return "expected_final_rows_dimension_mismatch";
	case Code::EmptyActorMarkerId:
		return "empty_actor_marker_id";
	case Code::EmptyProfileMarkerId:
		return "empty_profile_marker_id";
	case Code::UnsafeNoClaims:
		return "unsafe_no_claims";
	case Code::UnsafePromotionPolicy:
		return "unsafe_promotion_policy";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus status)
{
	using Status = iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus;
	switch (status) {
	case Status::UnsupportedSource:
		return "unsupported_source";
	case Status::InvalidPacket:
		return "invalid_packet";
	case Status::Converted:
		return "converted";
	case Status::ConversionFailed:
		return "conversion_failed";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayScenarioAuthoringSource source)
{
	using Source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource;
	switch (source) {
	case Source::Unsupported:
		return "unsupported";
	case Source::ProfileScenarioDefinition:
		return "profile_scenario_definition";
	case Source::AsciiSourcePlan:
		return "ascii_source_plan";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssueCode code)
{
	using Code = iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssueCode;
	switch (code) {
	case Code::UnsupportedSource:
		return "unsupported_source";
	case Code::MissingProfileScenarioPayload:
		return "missing_profile_scenario_payload";
	case Code::MissingAsciiSourcePlanPayload:
		return "missing_ascii_source_plan_payload";
	case Code::MissingAsciiSourcePlanConversionConfig:
		return "missing_ascii_source_plan_conversion_config";
	case Code::AsciiSourcePlanConversionFailed:
		return "ascii_source_plan_conversion_failed";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus status)
{
	using Status =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus;
	switch (status) {
	case Status::Converted:
		return "converted";
	case Status::SourcePlanInvalid:
		return "source_plan_invalid";
	case Status::UnsupportedTerrainPromotion:
		return "unsupported_terrain_promotion";
	case Status::MissingFrameDefaults:
		return "missing_frame_defaults";
	case Status::ActorRegistryInvalid:
		return "actor_registry_invalid";
	case Status::ControlRegistryInvalid:
		return "control_registry_invalid";
	case Status::AuthoredControlInvalid:
		return "authored_control_invalid";
	case Status::AuthoredPlayerCommandInvalid:
		return "authored_player_command_invalid";
	case Status::PlayerStartInvalid:
		return "player_start_invalid";
	case Status::ProfileTraitCatalogInvalid:
		return "profile_trait_catalog_invalid";
	case Status::InteractionTargetRegistryInvalid:
		return "interaction_target_registry_invalid";
	case Status::InteractionEffectCatalogInvalid:
		return "interaction_effect_catalog_invalid";
	case Status::ItemDropRegistryInvalid:
		return "item_drop_registry_invalid";
	case Status::AiMapPromotionInvalid:
		return "ai_map_promotion_invalid";
	case Status::ProfileScenarioInvalid:
		return "profile_scenario_invalid";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code)
{
	using Code =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode;
	switch (code) {
	case Code::SourcePlanInvalid:
		return "source_plan_invalid";
	case Code::UnsupportedTerrainPromotion:
		return "unsupported_terrain_promotion";
	case Code::MissingFrameDefaults:
		return "missing_frame_defaults";
	case Code::ActorRegistryInvalid:
		return "actor_registry_invalid";
	case Code::ControlRegistryInvalid:
		return "control_registry_invalid";
	case Code::UnknownAuthoredControlActor:
		return "unknown_authored_control_actor";
	case Code::DuplicateAuthoredControlActor:
		return "duplicate_authored_control_actor";
	case Code::AmbiguousAuthoredPlayerCommandFrame:
		return "ambiguous_authored_player_command_frame";
	case Code::DuplicatePlayerStart:
		return "duplicate_player_start";
	case Code::ProfileTraitCatalogInvalid:
		return "profile_trait_catalog_invalid";
	case Code::InteractionTargetRegistryInvalid:
		return "interaction_target_registry_invalid";
	case Code::InteractionEffectCatalogInvalid:
		return "interaction_effect_catalog_invalid";
	case Code::ItemDropRegistryInvalid:
		return "item_drop_registry_invalid";
	case Code::AiMapPromotionInvalid:
		return "ai_map_promotion_invalid";
	case Code::ProfileScenarioInvalid:
		return "profile_scenario_invalid";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayProfileScenarioIssueCode code)
{
	using Code = iggy::runtime::RuntimeGameplayProfileScenarioIssueCode;
	switch (code) {
	case Code::EmptyScenarioId:
		return "empty_scenario_id";
	case Code::EmptyFrameId:
		return "empty_frame_id";
	case Code::MissingProfileTrait:
		return "missing_profile_trait";
	case Code::EmptyActorProfileId:
		return "empty_actor_profile_id";
	case Code::NestedScenarioInvalid:
		return "nested_scenario_invalid";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayProfileScenarioValidationStatus status)
{
	using Status = iggy::runtime::RuntimeGameplayProfileScenarioValidationStatus;
	switch (status) {
	case Status::Valid:
		return "valid";
	case Status::Invalid:
		return "invalid";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayScenarioIssueCode code)
{
	using Code = iggy::runtime::RuntimeGameplayScenarioIssueCode;
	switch (code) {
	case Code::EmptyScenarioId:
		return "empty_scenario_id";
	case Code::EmptyFrameId:
		return "empty_frame_id";
	case Code::MissingControl:
		return "missing_control";
	case Code::OrphanControl:
		return "orphan_control";
	case Code::InvalidMovementMap:
		return "invalid_movement_map";
	case Code::DuplicateTraitSubject:
		return "duplicate_trait_subject";
	case Code::OrphanTraitSubject:
		return "orphan_trait_subject";
	case Code::MissingTraitSubject:
		return "missing_trait_subject";
	}
	return "unknown";
}

const char *ToString(
	iggy::runtime::RuntimeGameplayProfileScenarioRunStatus status)
{
	using Status = iggy::runtime::RuntimeGameplayProfileScenarioRunStatus;
	switch (status) {
	case Status::Ran:
		return "ran";
	case Status::ValidationFailed:
		return "validation_failed";
	}
	return "unknown";
}

struct TraceFrameProjection {
	std::size_t index = 0;
	std::string frameId;
	std::size_t acceptedCommandCount = 0;
	std::size_t pickedUpCount = 0;
	bool interactionChanged = false;
	std::size_t npcMovedCount = 0;
	std::vector<std::string> rows;
};

struct ExpectationComparison {
	bool present = false;
	bool matched = true;
	bool checkedFinalRows = false;
	bool finalRowsMatched = true;
	bool checkedFrameCount = false;
	bool frameCountMatched = true;
	bool checkedAcceptedCommandCount = false;
	bool acceptedCommandCountMatched = true;
	bool checkedPickedUpCount = false;
	bool pickedUpCountMatched = true;
	bool checkedInteractionChanged = false;
	bool interactionChangedMatched = true;
	bool checkedNpcMovedCount = false;
	bool npcMovedCountMatched = true;
};

int Usage()
{
	std::cerr << "status:\n";
	std::cerr << "result: usage_error\n";
	std::cerr << "usage: iggy_scenario_toml_runner [--trace] [--check] [--lint] <path>\n";
	return 1;
}

void PrintSourcePlanIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanIssue &issue,
	const char *prefix)
{
	std::cerr << prefix << ": code=" << ToString(issue.code)
		<< " index=" << issue.index
		<< " row=" << issue.row
		<< " column=" << issue.column;
	if (issue.glyph != '\0')
		std::cerr << " glyph=" << issue.glyph;
	if (!issue.id.empty())
		std::cerr << " id=" << IdText(issue.id);
	std::cerr << '\n';
}

void PrintFirstTomlIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult &read)
{
	if (!read.issues.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssue &issue =
			read.issues.front();
		std::cerr << "file_issue: code=" << ToString(issue.code);
		if (!issue.path.empty())
			std::cerr << " path=" << issue.path.string();
		if (!issue.detail.empty())
			std::cerr << " detail=" << issue.detail;
		std::cerr << '\n';
	}
	if (!read.text.issues.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue =
			read.text.issues.front();
		std::cerr << "toml_issue: code=" << ToString(issue.code)
			<< " line=" << issue.line
			<< " column=" << issue.column
			<< " table=" << issue.table
			<< " key=" << issue.key;
		if (issue.hasTableIndex)
			std::cerr << " table_index=" << issue.tableIndex;
		if (!issue.detail.empty())
			std::cerr << " detail=" << issue.detail;
		std::cerr << '\n';
		if (issue.code == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid)
			PrintSourcePlanIssue(issue.sourceIssue, "source_issue");
	}
}

void PrintFirstAdapterIssue(
	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult &adapter)
{
	if (adapter.issues.empty())
		return;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssue &issue =
		adapter.issues.front();
	std::cerr << "adapter_issue: code=" << ToString(issue.code)
		<< " source=" << ToString(issue.source)
		<< " has_profile_scenario="
		<< (issue.hasProfileScenario ? "true" : "false")
		<< " has_ascii_source_plan="
		<< (issue.hasAsciiSourcePlan ? "true" : "false")
		<< " has_source_plan_config="
		<< (issue.hasAsciiSourcePlanConversionConfig ? "true" : "false")
		<< " source_plan_status="
		<< ToString(issue.asciiSourcePlanConversionStatus)
		<< '\n';
}

void PrintFirstConversionIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult
		&conversion)
{
	if (conversion.issues.empty())
		return;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue
		&issue = conversion.issues.front();
	std::cerr << "conversion_issue: code=" << ToString(issue.code)
		<< " row=" << issue.row
		<< " column=" << issue.column;
	if (issue.glyph != '\0')
		std::cerr << " glyph=" << issue.glyph;
	if (!issue.authoredControl.npcId.empty())
		std::cerr << " npc=" << IdText(issue.authoredControl.npcId);
	if (!issue.authoredPlayerCommand.targetId.empty())
		std::cerr << " player_target=" << IdText(issue.authoredPlayerCommand.targetId);
	std::cerr << '\n';

	if (issue.code == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid)
		PrintSourcePlanIssue(issue.sourceIssue, "source_issue");
	if (issue.code == iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ProfileScenarioInvalid) {
		std::cerr << "profile_issue: code=" << ToString(issue.profileIssue.code)
			<< " frame_index=" << issue.profileIssue.frameIndex
			<< " actor_index=" << issue.profileIssue.actorIndex;
		if (!issue.profileIssue.npcId.empty())
			std::cerr << " npc=" << IdText(issue.profileIssue.npcId);
		if (!issue.profileIssue.profileId.empty())
			std::cerr << " profile=" << IdText(issue.profileIssue.profileId);
		std::cerr << '\n';
		if (issue.profileIssue.code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid) {
			const iggy::runtime::RuntimeGameplayScenarioIssue &nested =
				issue.profileIssue.nestedIssue;
			std::cerr << "scenario_issue: code=" << ToString(nested.code)
				<< " frame_index=" << nested.frameIndex
				<< " actor_index=" << nested.actorIndex
				<< " control_index=" << nested.controlIndex
				<< " subject_index=" << nested.subjectIndex;
			if (!nested.npcId.empty())
				std::cerr << " npc=" << IdText(nested.npcId);
			std::cerr << '\n';
		}
	}
}

std::string FrameIdText(
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult &run,
	std::size_t frameIndex)
{
	if (frameIndex < run.definition.frames.size()) {
		const iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition &frame =
			run.definition.frames[frameIndex];
		if (frame.hasFrameId && !frame.frameId.empty())
			return IdText(frame.frameId);
	}
	return "<none>";
}

std::vector<TraceFrameProjection> TraceFrames(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan,
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult &run)
{
	std::vector<TraceFrameProjection> frames;
	frames.reserve(run.scenario.runner.frameResults.size());
	for (std::size_t index = 0; index < run.scenario.runner.frameResults.size();
		++index) {
		const iggy::runtime::RuntimeGameplayOrchestratedFrameResult &frameResult =
			run.scenario.runner.frameResults[index];
		TraceFrameProjection frame;
		frame.index = index;
		frame.frameId = FrameIdText(run, index);
		frame.acceptedCommandCount = frameResult.acceptedCommandCount;
		frame.pickedUpCount = frameResult.pickedUpCount;
		frame.interactionChanged = frameResult.interactionChanged;
		frame.npcMovedCount = frameResult.npcMovedCount;
		frame.rows =
			iggy::runtime::finalDebugRowsForAsciiSourcePlan(plan, frameResult.state);
		frames.push_back(frame);
	}
	return frames;
}

void PrintTraceFrames(const std::vector<TraceFrameProjection> &frames)
{
	std::cout << "frames:\n";
	for (const TraceFrameProjection &frame : frames) {
		std::cout << "frame_index: " << frame.index << '\n';
		std::cout << "frame_id: " << frame.frameId << '\n';
		std::cout << "accepted_command_count: "
			<< frame.acceptedCommandCount << '\n';
		std::cout << "picked_up_count: " << frame.pickedUpCount << '\n';
		std::cout << "interaction_changed: "
			<< (frame.interactionChanged ? "true" : "false") << '\n';
		std::cout << "npc_moved_count: " << frame.npcMovedCount << '\n';
		std::cout << "rows:\n";
		for (const std::string &row : frame.rows)
			std::cout << row << '\n';
	}
}

ExpectationComparison CompareExpectations(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectations &expectations,
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult &run,
	const std::vector<std::string> &finalRows)
{
	ExpectationComparison comparison;
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

const char *MatchText(bool matched)
{
	return matched ? "matched" : "mismatched";
}

void PrintExpectationComparison(const ExpectationComparison &comparison)
{
	std::cout << "expectation:\n";
	std::cout << "present: " << (comparison.present ? "true" : "false") << '\n';
	if (!comparison.present) {
		std::cout << "result: not_provided\n";
		return;
	}

	std::cout << "result: " << MatchText(comparison.matched) << '\n';
	if (comparison.checkedFinalRows)
		std::cout << "final_rows: " << MatchText(comparison.finalRowsMatched)
			<< '\n';
	if (comparison.checkedFrameCount)
		std::cout << "frame_count: " << MatchText(comparison.frameCountMatched)
			<< '\n';
	if (comparison.checkedAcceptedCommandCount)
		std::cout << "accepted_command_count: "
			<< MatchText(comparison.acceptedCommandCountMatched) << '\n';
	if (comparison.checkedPickedUpCount)
		std::cout << "picked_up_count: "
			<< MatchText(comparison.pickedUpCountMatched) << '\n';
	if (comparison.checkedInteractionChanged)
		std::cout << "interaction_changed: "
			<< MatchText(comparison.interactionChangedMatched) << '\n';
	if (comparison.checkedNpcMovedCount)
		std::cout << "npc_moved_count: "
			<< MatchText(comparison.npcMovedCountMatched) << '\n';
}

void PrintFirstProfileValidationIssue(
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult &validation)
{
	if (validation.issues.empty())
		return;

	const iggy::runtime::RuntimeGameplayProfileScenarioIssue &issue =
		validation.issues.front();
	std::cerr << "profile_issue: code=" << ToString(issue.code)
		<< " frame_index=" << issue.frameIndex
		<< " actor_index=" << issue.actorIndex;
	if (!issue.npcId.empty())
		std::cerr << " npc=" << IdText(issue.npcId);
	if (!issue.profileId.empty())
		std::cerr << " profile=" << IdText(issue.profileId);
	std::cerr << '\n';
}

} // namespace

int main(int argc, char **argv)
{
	bool trace = false;
	bool check = false;
	bool lint = false;
	const char *pathArgument = nullptr;
	for (int index = 1; index < argc; ++index) {
		const std::string_view argument(argv[index]);
		if (argument == "--trace") {
			trace = true;
		} else if (argument == "--check") {
			check = true;
		} else if (argument == "--lint") {
			lint = true;
		} else if (pathArgument == nullptr) {
			pathArgument = argv[index];
		} else {
			return Usage();
		}
	}
	if (pathArgument == nullptr) {
		return Usage();
	}

	const std::filesystem::path path(pathArgument);
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	if (!read.ok()) {
		std::cerr << "status:\n";
		std::cerr << "result: read_failed\n";
		std::cerr << "read_status: " << ToString(read.status) << '\n';
		std::cerr << "toml_status: " << ToString(read.text.status) << '\n';
		std::cerr << "issue_count: "
			<< (read.issues.size() + read.text.issues.size()) << '\n';
		PrintFirstTomlIssue(read);
		return 2;
	}

	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(packet);
	if (!adapter.ok()) {
		std::cerr << "status:\n";
		std::cerr << "result: conversion_failed\n";
		std::cerr << "adapter_status: " << ToString(adapter.status) << '\n';
		std::cerr << "source_plan_status: "
			<< ToString(adapter.asciiSourcePlanConversion.status) << '\n';
		std::cerr << "issue_count: "
			<< (adapter.issues.size() + adapter.asciiSourcePlanConversion.issues.size())
			<< '\n';
		PrintFirstAdapterIssue(adapter);
		PrintFirstConversionIssue(adapter.asciiSourcePlanConversion);
		return 3;
	}

	if (lint) {
		const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult
			validation =
				iggy::runtime::RuntimeGameplayProfileScenarioValidator {}.validate(
					adapter.profileScenario);
		if (!validation.ok()) {
			std::cerr << "status:\n";
			std::cerr << "result: lint_failed\n";
			std::cerr << "adapter_status: " << ToString(adapter.status) << '\n';
			std::cerr << "profile_status: " << ToString(validation.status) << '\n';
			std::cerr << "validation_issue_count: " << validation.issueCount
				<< '\n';
			PrintFirstProfileValidationIssue(validation);
			return 4;
		}

		std::cout << "status:\n";
		std::cout << "result: lint_ok\n";
		std::cout << "summary:\n";
		std::cout << "source_path: " << path.string() << '\n';
		std::cout << "frame_count: " << validation.frameCount << '\n';
		std::cout << "adapter_status: " << ToString(adapter.status) << '\n';
		std::cout << "profile_status: " << ToString(validation.status) << '\n';
		return 0;
	}

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	if (!run.ran()) {
		std::cerr << "status:\n";
		std::cerr << "result: run_failed\n";
		std::cerr << "run_status: " << ToString(run.status) << '\n';
		std::cerr << "validation_issue_count: " << run.validation.issueCount
			<< '\n';
		PrintFirstProfileValidationIssue(run.validation);
		return 4;
	}

	std::cout << "status:\n";
	std::cout << "result: ok\n";
	std::cout << "summary:\n";
	std::cout << "source_path: " << path.string() << '\n';
	std::cout << "frame_count: " << run.frameCount << '\n';
	std::cout << "accepted_command_count: "
		<< run.scenario.runner.acceptedCommandCount << '\n';
	std::cout << "picked_up_count: " << run.scenario.runner.pickedUpCount << '\n';
	std::cout << "interaction_changed: "
		<< (run.scenario.runner.interactionChanged ? "true" : "false") << '\n';
	std::cout << "npc_moved_count: " << run.npcMovedCount << '\n';
	if (trace)
		PrintTraceFrames(TraceFrames(read.text.plan, run));

	const std::vector<std::string> rows =
		iggy::runtime::finalDebugRowsForAsciiSourcePlan(read.text.plan, run.state);
	const ExpectationComparison comparison =
		CompareExpectations(read.text.plan.expectations, run, rows);
	PrintExpectationComparison(comparison);
	std::cout << "final_rows:\n";
	for (const std::string &row : rows)
		std::cout << row << '\n';

	if (check && (!comparison.present || !comparison.matched))
		return 5;

	return 0;
}
