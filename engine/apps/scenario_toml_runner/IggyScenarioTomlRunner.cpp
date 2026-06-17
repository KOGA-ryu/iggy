#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanFinalDebugRows.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"
#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"

namespace {

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

int Usage()
{
	std::cerr << "usage: iggy_scenario_toml_runner <path>\n";
	return 1;
}

void PrintFirstTomlIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult &read)
{
	if (!read.issues.empty() && !read.issues.front().detail.empty()) {
		std::cerr << "detail: " << read.issues.front().detail << '\n';
		return;
	}
	if (!read.text.issues.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue =
			read.text.issues.front();
		std::cerr << "detail: line=" << issue.line
			<< " table=" << issue.table
			<< " key=" << issue.key;
		if (!issue.detail.empty())
			std::cerr << " " << issue.detail;
		std::cerr << '\n';
	}
}

} // namespace

int main(int argc, char **argv)
{
	if (argc != 2)
		return Usage();

	const std::filesystem::path path(argv[1]);
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	if (!read.ok()) {
		std::cerr << "read failed: status=" << ToString(read.status)
			<< " toml_status=" << ToString(read.text.status)
			<< " issues=" << (read.issues.size() + read.text.issues.size())
			<< '\n';
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
		std::cerr << "conversion failed: status=" << ToString(adapter.status)
			<< " source_plan_status="
			<< ToString(adapter.asciiSourcePlanConversion.status)
			<< " issues="
			<< (adapter.issues.size() + adapter.asciiSourcePlanConversion.issues.size())
			<< '\n';
		return 3;
	}

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	if (!run.ran()) {
		std::cerr << "run failed: status=" << ToString(run.status)
			<< " validation_issues=" << run.validation.issueCount
			<< '\n';
		return 4;
	}

	std::cout << "source_path: " << path.string() << '\n';
	std::cout << "frame_count: " << run.frameCount << '\n';
	std::cout << "accepted_command_count: "
		<< run.scenario.runner.acceptedCommandCount << '\n';
	std::cout << "picked_up_count: " << run.scenario.runner.pickedUpCount << '\n';
	std::cout << "interaction_changed: "
		<< (run.scenario.runner.interactionChanged ? "true" : "false") << '\n';
	std::cout << "npc_moved_count: " << run.npcMovedCount << '\n';
	std::cout << "final_rows:\n";

	const std::vector<std::string> rows =
		iggy::runtime::finalDebugRowsForAsciiSourcePlan(read.text.plan, run.state);
	for (const std::string &row : rows)
		std::cout << row << '\n';

	return 0;
}
