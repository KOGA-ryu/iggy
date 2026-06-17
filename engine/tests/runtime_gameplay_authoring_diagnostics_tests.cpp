#include "runtime/RuntimeGameplayAuthoringDiagnostics.hpp"
#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

struct TempTomlFile {
	std::filesystem::path path;

	TempTomlFile(const char *label, const std::string &text)
	{
		static int counter = 0;
		path = std::filesystem::temp_directory_path() /
			("iggy_authoring_diagnostics_" + std::string(label) + "_" +
				std::to_string(++counter) + ".toml");
		std::ofstream stream(path);
		stream << text;
	}

	~TempTomlFile()
	{
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
	}
};

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name;
}

const iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry *FindEntry(
	const std::vector<iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry>
		&entries,
	iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer layer,
	const char *code)
{
	for (const iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry &entry :
		entries) {
		if (entry.layer == layer && entry.code == code)
			return &entry;
	}
	return nullptr;
}

std::string UnknownControlActorToml()
{
	return
		"format_id = \"iggy:ascii-source-plan\"\n"
		"version = 1\n"
		"source_id = \"scenario:unknown-control-actor\"\n"
		"\n"
		"[grid]\n"
		"width = 7\n"
		"height = 4\n"
		"background = \".\"\n"
		"rows = [\n"
		"  \"#######\",\n"
		"  \"#A...@#\",\n"
		"  \"#.....#\",\n"
		"  \"#######\",\n"
		"]\n"
		"\n"
		"[[legend]]\n"
		"glyph = \"A\"\n"
		"kind = \"actor\"\n"
		"maps_to_scenario_marker = true\n"
		"scenario_marker_kind = \"actor\"\n"
		"\n"
		"[[legend]]\n"
		"glyph = \"@\"\n"
		"kind = \"player_start\"\n"
		"maps_to_scenario_marker = true\n"
		"scenario_marker_kind = \"player_start\"\n"
		"\n"
		"[[profiles]]\n"
		"id = \"profile:guard\"\n"
		"strength = 10\n"
		"dexterity = 10\n"
		"constitution = 10\n"
		"intelligence = 10\n"
		"wisdom = 10\n"
		"charisma = 10\n"
		"\n"
		"[[cells]]\n"
		"id = \"cell:guard\"\n"
		"row = 1\n"
		"column = 1\n"
		"glyph = \"A\"\n"
		"local_tile = { x = 1, y = 1 }\n"
		"local_position = { x = 1.5, y = 1.5 }\n"
		"cell_bounds = { min_x = 1.0, min_y = 1.0, max_x = 2.0, max_y = 2.0 }\n"
		"marker_id = \"npc:guard\"\n"
		"profile_id = \"profile:guard\"\n"
		"\n"
		"[[frame_controls]]\n"
		"frame_id = \"frame:unknown-control-actor\"\n"
		"npc = \"npc:missing\"\n"
		"behavior = \"seeking\"\n"
		"move_mode = \"walk\"\n"
		"target = { x = 2.5, y = 1.5 }\n";
}

void TestReadDiagnosticsProjectTomlAndSourcePlanEntries()
{
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("semantic_invalid_guard_room.toml"));

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed,
		"semantic fixture should fail during read");
	const iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry *toml =
		FindEntry(
			result.diagnostics,
			iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::Toml,
			"source_plan_invalid");
	const iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry *source =
		FindEntry(
			result.diagnostics,
			iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::SourcePlan,
			"annotated_cell_glyph_mismatch");
	Expect(toml != nullptr,
		"semantic fixture should project TOML source-plan diagnostic");
	Expect(source != nullptr,
		"semantic fixture should project mirrored source-plan diagnostic");
	if (toml != nullptr && source != nullptr) {
		Expect(toml->layer ==
			iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::Toml,
			"first read diagnostic should be TOML layer");
		Expect(toml->code == "source_plan_invalid",
			"TOML diagnostic should use stable source-plan-invalid code");
		Expect(toml->table == "cells" && toml->hasTableIndex &&
			toml->tableIndex == 0,
			"TOML diagnostic should preserve table location");
		Expect(toml->hasLine && toml->line == 36,
			"TOML diagnostic should preserve line");

		Expect(source->layer ==
			iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::SourcePlan,
			"second read diagnostic should be source-plan layer");
		Expect(source->code == "annotated_cell_glyph_mismatch",
			"source-plan diagnostic should use stable glyph mismatch code");
		Expect(source->hasIndex && source->index == 0,
			"source-plan diagnostic should preserve index");
		Expect(source->hasRow && source->row == 1 && source->hasColumn &&
			source->column == 1 && source->glyph == 'A',
			"source-plan diagnostic should preserve row, column, and glyph");
	}
}

void TestConversionDiagnosticsProjectAdapterAndConversionEntries()
{
	TempTomlFile unknown("unknown_control", UnknownControlActorToml());
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(unknown.path);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed,
		"unknown control should fail during conversion");
	const iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry *adapter =
		FindEntry(
			result.diagnostics,
			iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::Adapter,
			"ascii_source_plan_conversion_failed");
	const iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry *conversion =
		FindEntry(
			result.diagnostics,
			iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::Conversion,
			"unknown_authored_control_actor");
	Expect(adapter != nullptr,
		"conversion failure should project adapter entry");
	Expect(conversion != nullptr,
		"conversion failure should project conversion entry");
	if (adapter != nullptr && conversion != nullptr) {
		Expect(conversion->id.value() == "npc:missing",
			"conversion diagnostic should preserve relevant id");
	}
}

void TestProfileDiagnosticsProjectNestedProfileEntry()
{
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
			FixturePath("valid_guard_room.toml"));

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed,
		"missing profile catalog should fail during conversion");
	const iggy::runtime::RuntimeGameplayAuthoringDiagnosticEntry *profile =
		FindEntry(
			result.diagnostics,
			iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::Profile,
			"missing_profile_trait");
	Expect(profile != nullptr,
		"profile failure should project nested profile entry");
	if (profile != nullptr) {
		Expect(profile->layer ==
			iggy::runtime::RuntimeGameplayAuthoringDiagnosticLayer::Profile,
			"nested profile diagnostic should be profile layer");
		Expect(profile->code == "missing_profile_trait",
			"profile diagnostic should preserve stable code");
		Expect(profile->hasIndex && profile->index == 0,
			"profile diagnostic should preserve frame index");
		Expect(profile->hasActorIndex && profile->actorIndex == 0,
			"profile diagnostic should preserve actor index");
		Expect(profile->id.value() == "profile:guard",
			"profile diagnostic should preserve profile id as primary id");
	}
}

} // namespace

int main()
{
	TestReadDiagnosticsProjectTomlAndSourcePlanEntries();
	TestConversionDiagnosticsProjectAdapterAndConversionEntries();
	TestProfileDiagnosticsProjectNestedProfileEntry();

	if (Failures != 0)
		return 1;
	return 0;
}
