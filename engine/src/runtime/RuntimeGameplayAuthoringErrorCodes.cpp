#include "runtime/RuntimeGameplayAuthoringErrorCodes.hpp"

namespace iggy::runtime {

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanTomlFileReadStatus status)
{
	using Status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanTomlReadStatus status)
{
	using Status = RuntimeGameplayAsciiSourcePlanTomlReadStatus;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode code)
{
	using Code = RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanTomlReadIssueCode code)
{
	using Code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanIssueCode code)
{
	using Code = RuntimeGameplayAsciiSourcePlanIssueCode;
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
	case Code::ExpectedTraceFrameRowsEmpty:
		return "expected_trace_frame_rows_empty";
	case Code::ExpectedTraceFrameRowsDimensionMismatch:
		return "expected_trace_frame_rows_dimension_mismatch";
	case Code::ExpectedInventoryStackMissingItemId:
		return "expected_inventory_stack_missing_item_id";
	case Code::ExpectedInventoryStackInvalidCount:
		return "expected_inventory_stack_invalid_count";
	case Code::ExpectedInventoryStackDuplicateItemId:
		return "expected_inventory_stack_duplicate_item_id";
	case Code::ExpectedInteractionTargetMissingId:
		return "expected_interaction_target_missing_id";
	case Code::ExpectedInteractionTargetDuplicateId:
		return "expected_interaction_target_duplicate_id";
	case Code::ExpectedInteractionTargetUnknownId:
		return "expected_interaction_target_unknown_id";
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayScenarioAuthoringAdapterStatus status)
{
	using Status = RuntimeGameplayScenarioAuthoringAdapterStatus;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayScenarioAuthoringSource source)
{
	using Source = RuntimeGameplayScenarioAuthoringSource;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayScenarioAuthoringAdapterIssueCode code)
{
	using Code = RuntimeGameplayScenarioAuthoringAdapterIssueCode;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus status)
{
	using Status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code)
{
	using Code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayProfileScenarioIssueCode code)
{
	using Code = RuntimeGameplayProfileScenarioIssueCode;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayProfileScenarioValidationStatus status)
{
	using Status = RuntimeGameplayProfileScenarioValidationStatus;
	switch (status) {
	case Status::Valid:
		return "valid";
	case Status::Invalid:
		return "invalid";
	}
	return "unknown";
}

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayScenarioIssueCode code)
{
	using Code = RuntimeGameplayScenarioIssueCode;
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

const char *runtimeGameplayAuthoringCodeText(
	RuntimeGameplayProfileScenarioRunStatus status)
{
	using Status = RuntimeGameplayProfileScenarioRunStatus;
	switch (status) {
	case Status::Ran:
		return "ran";
	case Status::ValidationFailed:
		return "validation_failed";
	}
	return "unknown";
}

} // namespace iggy::runtime
