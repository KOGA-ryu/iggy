#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"
#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"
#include "scene/debug/SceneAsciiCanvas2D.hpp"
#include "scene/level/TileCoord.hpp"
#include "support/LevelMapFixtures.hpp"

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_gameplay_ascii_source_plan_toml_file_reader_tests_tmp";
}

std::filesystem::path TempPath(const char *name)
{
	return TempRoot() / name;
}

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name;
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

void CleanupTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
}

void WriteText(const std::filesystem::path &path, const std::string &text)
{
	std::ofstream stream(path, std::ios::binary | std::ios::trunc);
	stream << text;
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssue &issue :
		result.issues) {
		if (issue.code == code) {
			return true;
		}
	}
	return false;
}

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::NpcTraitSet Traits()
{
	iggy::NpcTraitSet traits;
	traits.strength = 10;
	return traits;
}

iggy::NpcAiProfileTraitCatalog Catalog(std::vector<iggy::NpcAiProfileTraitEntry> entries)
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build(entries);
	Expect(result.built, "file reader fixture acceptance profile catalog should build");
	return result.catalog;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition DefaultFrame()
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:fixture");
	frame.movementMap = iggy::test::MapFromRows({
		"#######",
		"#.....#",
		"#.....#",
		"#######",
	});
	frame.movementMap.id = Id("level:fixture-default");
	return frame;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig ConverterConfig()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config;
	config.hasDefaultFrame = true;
	config.defaultFrame = DefaultFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });
	return config;
}

char GlyphForActor(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan,
	const iggy::ResourceId &npcId)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell :
		plan.annotatedCells) {
		if (cell.markerId == npcId)
			return cell.glyph;
	}
	return 'N';
}

char GlyphForItemDrop(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan,
	const iggy::ResourceId &dropId)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &drop :
		plan.authoredItemDrops) {
		if (drop.dropId == dropId && drop.glyph != '\0')
			return drop.glyph;
	}
	return 'i';
}

bool IsAuthoredItemDropGlyph(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan,
	char glyph)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &drop :
		plan.authoredItemDrops) {
		if (drop.glyph == glyph && glyph != '\0')
			return true;
	}
	return false;
}

char BaseDebugGlyphForSource(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan,
	char glyph)
{
	if (IsAuthoredItemDropGlyph(plan, glyph))
		return '.';

	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry :
		plan.legend) {
		if (entry.glyph != glyph)
			continue;
		if (entry.kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor
			|| entry.scenarioMarkerKind == iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor) {
			return '.';
		}
		if (entry.kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart
			|| entry.scenarioMarkerKind == iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart) {
			return '.';
		}
	}
	return glyph;
}

std::vector<std::string> FinalDebugRows(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan &plan,
	const iggy::runtime::RuntimeGameplayState &state)
{
	iggy::SceneAsciiCanvas2D canvas =
		iggy::makeSceneAsciiCanvas2D(plan.grid.width, plan.grid.height, ' ');

	for (std::size_t row = 0; row < plan.grid.rows.size(); ++row) {
		const std::string &text = plan.grid.rows[row];
		for (std::size_t column = 0; column < text.size(); ++column) {
			canvas = iggy::setSceneAsciiCanvas2DPoint(
				canvas,
				column,
				row,
				BaseDebugGlyphForSource(plan, text[column])).canvas;
		}
	}

	for (const iggy::NpcActorState2D &actor : state.npcActors.actors) {
		if (!actor.present)
			continue;
		const iggy::TileCoord tile = iggy::tileForPoint(actor.position);
		if (tile.x < 0 || tile.y < 0)
			continue;
		canvas = iggy::setSceneAsciiCanvas2DPoint(
			canvas,
			static_cast<std::size_t>(tile.x),
			static_cast<std::size_t>(tile.y),
			GlyphForActor(plan, actor.npcId)).canvas;
	}

	for (const iggy::LevelItemDrop2D &drop : state.inventory.drops.drops) {
		if (!drop.enabled)
			continue;
		const iggy::TileCoord tile = iggy::tileForPoint(drop.position);
		if (tile.x < 0 || tile.y < 0)
			continue;
		canvas = iggy::setSceneAsciiCanvas2DPoint(
			canvas,
			static_cast<std::size_t>(tile.x),
			static_cast<std::size_t>(tile.y),
			GlyphForItemDrop(plan, drop.id)).canvas;
	}

	if (state.session.hasPlayer) {
		const iggy::TileCoord tile = iggy::playerTile(state.session.player);
		if (tile.x >= 0 && tile.y >= 0) {
			canvas = iggy::setSceneAsciiCanvas2DPoint(
				canvas,
				static_cast<std::size_t>(tile.x),
				static_cast<std::size_t>(tile.y),
				'@').canvas;
		}
	}

	return iggy::renderSceneAsciiCanvas2DRows(canvas);
}

std::string ValidToml()
{
	return R"toml(
format_id = "iggy:ascii-source-plan"
version = 1
source_id = "scenario:file-reader"

[grid]
width = 3
height = 3
background = "."
rows = [
  "###",
  "#.#",
  "###",
]

[no_claims]
runtime_truth = false
gameplay_execution = false
file_parsing = false
profile_scenario_conversion = false

[promotion]
ready = false
runtime_execution = false
file_parsing = false
profile_scenario_conversion = false
)toml";
}

void TestEmptyPathDoesNotCallTextReader()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read({});

	Expect(!result.ok(), "empty path file read should not be ok");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::EmptyPath, "empty path should report EmptyPath");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::EmptyPath), "empty path should add empty path issue");
	Expect(result.text.input.empty(), "empty path should not call nested text reader");
	Expect(result.bytesRead == 0, "empty path should read zero bytes");
}

void TestMissingPathReportsMissingFile()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("missing.toml");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.path == path, "missing file result should preserve path");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::MissingFile, "missing path should report MissingFile");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::MissingFile), "missing path should add missing file issue");
	Expect(result.text.input.empty(), "missing path should not call nested text reader");
}

void TestDirectoryReportsNonRegularFile()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("directory");
	std::filesystem::create_directories(path);

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::NonRegularFile, "directory path should report NonRegularFile");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::NonRegularFile), "directory path should add non-regular issue");
	Expect(result.text.input.empty(), "directory path should not call nested text reader");
}

void TestValidFileReadsAndParses()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("valid.toml");
	const std::string text = ValidToml();
	WriteText(path, text);

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.ok(), "valid TOML file should parse");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::Parsed, "valid TOML file should report Parsed");
	Expect(result.path == path, "valid TOML file result should preserve path");
	Expect(result.bytesRead == text.size(), "valid TOML file should report bytes read");
	Expect(result.text.input == text, "nested text reader should own file contents");
	Expect(result.text.ok(), "nested text reader should parse valid file contents");
	Expect(result.text.plan.sourceId == iggy::ResourceId("scenario:file-reader"), "nested plan should preserve source id");
}

void TestInvalidTomlFilePreservesNestedDiagnostics()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("invalid.toml");
	WriteText(path, "not toml");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::TomlReadFailed, "invalid TOML file should report TomlReadFailed");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::TomlReadFailed), "invalid TOML file should add wrapper failure issue");
	Expect(result.bytesRead > 0, "invalid TOML file should report bytes read");
	Expect(!result.text.ok(), "nested text reader should preserve parse failure");
	Expect(!result.text.issues.empty(), "nested text reader should preserve diagnostics");
}

void TestValidFixtureReadsAndParses()
{
	const std::filesystem::path path = FixturePath("valid_guard_room.toml");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.ok(), "valid guard room fixture should parse");
	Expect(result.path == path, "valid fixture result should preserve path");
	Expect(result.bytesRead > 0, "valid fixture should report nonzero bytes read");
	Expect(result.text.ok(), "valid fixture nested text reader should parse");
	Expect(result.text.plan.sourceId == iggy::ResourceId("scenario:guard-room"), "valid fixture should preserve source id");
	Expect(result.text.plan.grid.rows.size() == 4, "valid fixture should preserve grid rows");
	Expect(result.text.plan.legend.size() == 2, "valid fixture should preserve legend entries");
	Expect(result.text.plan.annotatedCells.size() == 1, "valid fixture should preserve annotated cell");
	Expect(result.text.plan.annotatedCells.front().markerId == iggy::ResourceId("npc:guard"), "valid fixture should preserve actor marker id");
}

void TestCorruptFixturePreservesNestedParserDiagnostics()
{
	const std::filesystem::path path = FixturePath("corrupt_guard_room.toml");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::TomlReadFailed, "corrupt fixture should report TomlReadFailed");
	Expect(result.path == path, "corrupt fixture result should preserve path");
	Expect(result.bytesRead > 0, "corrupt fixture should report nonzero bytes read");
	Expect(!result.text.ok(), "corrupt fixture nested text reader should fail");
	Expect(!result.text.issues.empty(), "corrupt fixture should preserve nested parser issue");
	Expect(result.text.status != iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "corrupt fixture should fail before source validation");
}

void TestSemanticInvalidFixturePreservesNestedSourceValidation()
{
	const std::filesystem::path path = FixturePath("semantic_invalid_guard_room.toml");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::TomlReadFailed, "semantic invalid fixture should report TomlReadFailed");
	Expect(result.path == path, "semantic invalid fixture result should preserve path");
	Expect(result.bytesRead > 0, "semantic invalid fixture should report nonzero bytes read");
	Expect(!result.text.ok(), "semantic invalid fixture nested text reader should fail");
	Expect(result.text.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "semantic invalid fixture should preserve nested source validation status");
	Expect(result.text.sourcePlanIssueCount > 0, "semantic invalid fixture should preserve source validation issue count");
}

void TestValidFixtureConvertsToValidatedProfileScenario()
{
	const std::filesystem::path path = FixturePath("valid_guard_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config =
		ConverterConfig();

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult conversion =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {}.convert(
			read.text.plan,
			config);
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult validation =
		iggy::runtime::RuntimeGameplayProfileScenarioValidator {}.validate(conversion.definition);

	Expect(read.ok(), "fixture acceptance should read valid fixture");
	Expect(conversion.ok(), "fixture source plan should convert to profile scenario");
	Expect(conversion.converted, "fixture conversion should mark converted");
	Expect(validation.ok(), "fixture converted profile scenario should validate");
	Expect(conversion.definition.scenarioId == Id("scenario:guard-room"), "fixture conversion should preserve scenario id");
	Expect(conversion.promotedMap.width == 7 && conversion.promotedMap.height == 4, "fixture conversion should promote map dimensions");
	Expect(conversion.promotedMap.tileAt(0, 0) != nullptr && !conversion.promotedMap.tileAt(0, 0)->walkable, "fixture wall should promote as blocked");
	Expect(conversion.promotedMap.tileAt(1, 1) != nullptr && conversion.promotedMap.tileAt(1, 1)->walkable, "fixture actor glyph should promote as walkable");
	Expect(conversion.definition.frames.size() == 1, "fixture conversion should publish default frame");
	if (!conversion.definition.frames.empty()) {
		Expect(conversion.definition.frames[0].movementMap.width == conversion.promotedMap.width, "fixture frame movement map should use promoted map width");
		Expect(conversion.definition.frames[0].movementMap.height == conversion.promotedMap.height, "fixture frame movement map should use promoted map height");
	}
	Expect(conversion.definition.initialState.npcActors.actors.size() == 1, "fixture conversion should promote one actor");
	Expect(conversion.definition.initialState.npcControls.entries.size() == 1, "fixture conversion should create one default control");
	Expect(conversion.definition.initialState.session.hasPlayer, "fixture conversion should promote player start");
	Expect(conversion.definition.initialState.session.player.position.x == 5.5F && conversion.definition.initialState.session.player.position.y == 1.5F, "fixture conversion should promote player at tile center");
	if (!conversion.definition.initialState.npcActors.actors.empty()) {
		const iggy::NpcActorState2D &actor =
			conversion.definition.initialState.npcActors.actors.front();
		Expect(actor.npcId == Id("npc:guard"), "fixture actor should preserve npc id");
		Expect(actor.aiProfileId == Id("profile:guard"), "fixture actor should preserve profile id");
		Expect(actor.position.x == 1.5F && actor.position.y == 1.5F, "fixture actor should preserve local position");
	}
	Expect(read.text.plan.sourceId == Id("scenario:guard-room"), "file reader should only expose source plan for converter");
}

void TestValidFixtureFeedsAuthoringAdapterThroughParsedSourcePlan()
{
	const std::filesystem::path path = FixturePath("valid_guard_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig adapterConfig;
	adapterConfig.hasAsciiSourcePlanProfileScenarioConfig = true;
	adapterConfig.asciiSourcePlanProfileScenario = ConverterConfig();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(
			packet,
			adapterConfig);
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult validation =
		iggy::runtime::RuntimeGameplayProfileScenarioValidator {}.validate(
			adapter.profileScenario);

	Expect(read.ok(), "authoring adapter fixture acceptance should read valid fixture");
	Expect(packet.asciiSourcePlan.sourceId == Id("scenario:guard-room"), "caller should pass parsed source plan into authoring adapter");
	Expect(adapter.ok(), "authoring adapter should convert parsed source plan from fixture");
	Expect(adapter.asciiSourcePlanConversion.ok(), "authoring adapter should preserve nested source-plan conversion result");
	Expect(validation.ok(), "authoring adapter converted fixture profile scenario should validate");
	Expect(adapter.profileScenario.scenarioId == Id("scenario:guard-room"), "authoring adapter should publish fixture scenario id");
	Expect(adapter.profileScenario.initialState.npcActors.actors.size() == 1, "authoring adapter should publish fixture actor");
	Expect(adapter.profileScenario.initialState.npcControls.entries.size() == 1, "authoring adapter should publish fixture control");
	Expect(adapter.profileScenario.initialState.session.hasPlayer, "authoring adapter should publish fixture player start");
	Expect(adapter.profileScenario.frames.size() == 1, "authoring adapter should publish fixture frame");
	Expect(adapter.packet.asciiSourcePlan.sourceId == read.text.plan.sourceId, "authoring adapter should preserve parsed source-plan packet");
	Expect(adapter.config.hasAsciiSourcePlanProfileScenarioConfig, "authoring adapter should preserve explicit conversion config");
}

void TestValidFixtureRunsScenarioAndRendersFinalDebugRows()
{
	const std::filesystem::path path = FixturePath("valid_guard_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig adapterConfig;
	adapterConfig.hasAsciiSourcePlanProfileScenarioConfig = true;
	adapterConfig.asciiSourcePlanProfileScenario = ConverterConfig();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(
			packet,
			adapterConfig);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	const std::vector<std::string> rows = FinalDebugRows(read.text.plan, run.state);
	const std::vector<std::string> expected {
		"#######",
		"#A...@#",
		"#.....#",
		"#######",
	};

	Expect(read.ok(), "vertical path should read checked-in TOML fixture");
	Expect(adapter.ok(), "vertical path should adapt parsed source plan to profile scenario");
	Expect(run.ran(), "vertical path should run converted profile scenario");
	Expect(run.frameCount == 1, "vertical path should execute one scenario frame");
	Expect(run.state.session.hasPlayer, "vertical path should preserve promoted player");
	Expect(run.state.session.player.position.x == 5.5F && run.state.session.player.position.y == 1.5F, "vertical path should preserve idle player position");
	Expect(run.state.npcActors.actors.size() == 1, "vertical path should preserve final NPC actor");
	if (!run.state.npcActors.actors.empty()) {
		const iggy::NpcActorState2D &actor = run.state.npcActors.actors.front();
		Expect(actor.npcId == Id("npc:guard"), "vertical path final actor should preserve id");
		Expect(actor.position.x == 1.5F && actor.position.y == 1.5F, "vertical path default controls should not move actor");
	}
	Expect(rows == expected, "vertical path should render final ASCII debug rows");
}

void TestMovingFixtureRunsScenarioAndMovesNpcFromAuthoredControl()
{
	const std::filesystem::path path = FixturePath("moving_guard_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig adapterConfig;
	adapterConfig.hasAsciiSourcePlanProfileScenarioConfig = true;
	adapterConfig.asciiSourcePlanProfileScenario = ConverterConfig();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(
			packet,
			adapterConfig);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	const std::vector<std::string> rows = FinalDebugRows(read.text.plan, run.state);
	const std::vector<std::string> expected {
		"#######",
		"#.A..@#",
		"#.....#",
		"#######",
	};

	Expect(read.ok(), "movement vertical path should read checked-in TOML fixture");
	Expect(read.text.plan.sourceId == Id("scenario:moving-guard-room"), "movement fixture should preserve source id");
	Expect(read.text.plan.authoredControlCount() == 1, "movement fixture should parse one authored control");
	Expect(adapter.ok(), "movement vertical path should adapt parsed source plan to profile scenario");
	Expect(adapter.asciiSourcePlanConversion.ok(), "movement vertical path should preserve source-plan conversion success");
	Expect(adapter.asciiSourcePlanConversion.authoredControlCount == 1, "movement conversion should consume authored control");
	Expect(!adapterConfig.asciiSourcePlanProfileScenario.hasDefaultControl, "movement fixture should not rely on C++ default-control movement");
	Expect(run.ran(), "movement vertical path should run converted profile scenario");
	Expect(run.frameCount == 1, "movement vertical path should execute one scenario frame");
	Expect(run.npcMovementPlannedRequestCount == 1, "movement vertical path should plan one NPC movement request");
	Expect(run.npcMovedCount == 1, "movement vertical path should move one NPC");
	Expect(run.npcActorsChanged, "movement vertical path should mark NPC actors changed");
	Expect(run.state.npcActors.actors.size() == 1, "movement vertical path should preserve one final NPC actor");
	if (!run.state.npcActors.actors.empty()) {
		const iggy::NpcActorState2D &actor = run.state.npcActors.actors.front();
		Expect(actor.npcId == Id("npc:guard"), "movement vertical path final actor should preserve id");
		Expect(actor.position.x == 2.5F && actor.position.y == 1.5F, "movement vertical path should move actor one tile");
	}
	Expect(rows == expected, "movement vertical path should render moved final ASCII debug rows");
}

void TestMultiFrameFixtureRunsScenarioAndMovesNpcAcrossFrames()
{
	const std::filesystem::path path = FixturePath("multi_frame_guard_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig adapterConfig;
	adapterConfig.hasAsciiSourcePlanProfileScenarioConfig = true;
	adapterConfig.asciiSourcePlanProfileScenario = ConverterConfig();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(
			packet,
			adapterConfig);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	const std::vector<std::string> rows = FinalDebugRows(read.text.plan, run.state);
	const std::vector<std::string> expected {
		"#######",
		"#..A.@#",
		"#.....#",
		"#######",
	};

	Expect(read.ok(), "multi-frame vertical path should read checked-in TOML fixture");
	Expect(read.text.plan.sourceId == Id("scenario:multi-frame-guard-room"), "multi-frame fixture should preserve source id");
	Expect(read.text.plan.authoredControlCount() == 2, "multi-frame fixture should parse two authored controls");
	Expect(adapter.ok(), "multi-frame vertical path should adapt parsed source plan to profile scenario");
	Expect(adapter.asciiSourcePlanConversion.ok(), "multi-frame vertical path should preserve source-plan conversion success");
	Expect(adapter.asciiSourcePlanConversion.authoredControlCount == 2, "multi-frame conversion should consume authored controls");
	Expect(adapter.profileScenario.frames.size() == 2, "multi-frame conversion should publish two profile frames");
	if (adapter.profileScenario.frames.size() == 2) {
		Expect(adapter.profileScenario.frames[0].frameId == Id("frame:move-1"), "multi-frame conversion should preserve first frame id");
		Expect(adapter.profileScenario.frames[1].frameId == Id("frame:move-2"), "multi-frame conversion should preserve second frame id");
		Expect(adapter.profileScenario.frames[0].controlOverrides.size() == 1, "first multi-frame profile frame should carry override");
		Expect(adapter.profileScenario.frames[1].controlOverrides.size() == 1, "second multi-frame profile frame should carry override");
	}
	Expect(run.ran(), "multi-frame vertical path should run converted profile scenario");
	Expect(run.frameCount == 2, "multi-frame vertical path should execute two scenario frames");
	Expect(run.npcMovementPlannedRequestCount == 2, "multi-frame vertical path should plan two NPC movement requests");
	Expect(run.npcMovedCount == 2, "multi-frame vertical path should move one NPC twice");
	Expect(run.npcActorsChanged, "multi-frame vertical path should mark NPC actors changed");
	Expect(run.state.npcActors.actors.size() == 1, "multi-frame vertical path should preserve one final NPC actor");
	if (!run.state.npcActors.actors.empty()) {
		const iggy::NpcActorState2D &actor = run.state.npcActors.actors.front();
		Expect(actor.npcId == Id("npc:guard"), "multi-frame final actor should preserve id");
		Expect(actor.position.x == 3.5F && actor.position.y == 1.5F, "multi-frame vertical path should move actor two tiles");
	}
	Expect(rows == expected, "multi-frame vertical path should render moved final ASCII debug rows");
}

void TestPlayerAndGuardFixtureRunsSharedFrameThroughScenario()
{
	const std::filesystem::path path = FixturePath("player_and_guard_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig adapterConfig;
	adapterConfig.hasAsciiSourcePlanProfileScenarioConfig = true;
	adapterConfig.asciiSourcePlanProfileScenario = ConverterConfig();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(
			packet,
			adapterConfig);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	const std::vector<std::string> rows = FinalDebugRows(read.text.plan, run.state);
	const std::vector<std::string> expected {
		"#######",
		"#.A.@.#",
		"#.....#",
		"#######",
	};

	Expect(read.ok(), "player-and-guard fixture should read checked-in TOML fixture");
	Expect(read.text.plan.sourceId == Id("scenario:player-and-guard-room"), "player-and-guard fixture should preserve source id");
	Expect(read.text.plan.authoredControlCount() == 1, "player-and-guard fixture should parse one authored control");
	Expect(read.text.plan.authoredPlayerCommandCount() == 1, "player-and-guard fixture should parse one authored player command");
	Expect(adapter.ok(), "player-and-guard vertical path should adapt parsed source plan");
	Expect(adapter.asciiSourcePlanConversion.ok(), "player-and-guard conversion should succeed");
	Expect(adapter.asciiSourcePlanConversion.authoredControlCount == 1, "player-and-guard conversion should consume authored control");
	Expect(adapter.asciiSourcePlanConversion.authoredPlayerCommandCount == 1, "player-and-guard conversion should consume authored player command");
	Expect(adapter.profileScenario.frames.size() == 1, "shared player-and-guard frame should merge to one profile frame");
	if (adapter.profileScenario.frames.size() == 1) {
		const iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition &frame =
			adapter.profileScenario.frames[0];
		Expect(frame.hasFrameId && frame.frameId == Id("frame:shared"), "shared player-and-guard frame should preserve frame id");
		Expect(frame.controlOverrides.size() == 1, "shared player-and-guard frame should carry NPC override");
		Expect(frame.playerFrame.playerIntents.size() == 1, "shared player-and-guard frame should carry player intent");
		if (!frame.playerFrame.playerIntents.empty()) {
			const iggy::PlayerInputIntent2D &intent = frame.playerFrame.playerIntents[0];
			Expect(intent.type == iggy::PlayerInputIntent2DType::MoveToTile, "shared player command should become move-to-tile intent");
			Expect(intent.tile.x == 4 && intent.tile.y == 1, "shared player command should preserve target tile");
		}
	}
	Expect(run.ran(), "player-and-guard vertical path should run converted profile scenario");
	Expect(run.frameCount == 1, "player-and-guard vertical path should execute one shared frame");
	Expect(run.scenario.runner.acceptedCommandCount == 1, "player-and-guard vertical path should accept authored player command");
	Expect(run.state.session.hasPlayer, "player-and-guard vertical path should preserve promoted runtime player");
	Expect(run.state.session.player.position.x == 4.5F && run.state.session.player.position.y == 1.5F, "player-and-guard vertical path should move player to authored tile");
	if (!run.scenario.runner.frameResults.empty()
		&& !run.scenario.runner.frameResults[0].playerFrame.frame.interaction.playerInput.command.runner.runner.ticks.empty()) {
		const iggy::runtime::RuntimeSessionCommandTickResult &tick =
			run.scenario.runner.frameResults[0].playerFrame.frame.interaction.playerInput.command.runner.runner.ticks[0];
		Expect(tick.playerCommands.planning.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "player-and-guard run should plan player command");
		Expect(tick.playerCommands.execution.status == iggy::runtime::RuntimePlayerCommandExecutionStatus::Executed, "player-and-guard run should execute player command");
		Expect(tick.playerCommands.execution.executedPlanCount == 1, "player-and-guard run should execute one player plan");
		if (!tick.playerCommands.execution.movementResults.empty()) {
			const iggy::PlayerMovementExecutionResult &movement =
				tick.playerCommands.execution.movementResults[0];
			Expect(movement.status == iggy::PlayerMovementExecutionStatus::Moved, "player-and-guard run should move player");
			Expect(movement.reachedTarget, "player-and-guard run should reach authored player tile");
		}
	}
	Expect(run.npcMovementPlannedRequestCount == 1, "player-and-guard vertical path should plan NPC movement");
	Expect(run.npcMovedCount == 1, "player-and-guard vertical path should still move NPC");
	Expect(run.state.npcActors.actors.size() == 1, "player-and-guard vertical path should preserve one final NPC actor");
	if (!run.state.npcActors.actors.empty()) {
		const iggy::NpcActorState2D &actor = run.state.npcActors.actors.front();
		Expect(actor.npcId == Id("npc:guard"), "player-and-guard final actor should preserve id");
		Expect(actor.position.x == 2.5F && actor.position.y == 1.5F, "player-and-guard vertical path should move actor one tile");
	}
	Expect(rows == expected, "player-and-guard vertical path should render moved NPC and moved player");
}

void TestSelfContainedFixtureRunsWithEmptyConverterConfig()
{
	const std::filesystem::path path = FixturePath("self_contained_guard_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(packet);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	const std::vector<std::string> rows = FinalDebugRows(read.text.plan, run.state);
	const std::vector<std::string> expected {
		"#######",
		"#.A.@.#",
		"#.....#",
		"#######",
	};

	Expect(read.ok(), "self-contained fixture should read checked-in TOML fixture");
	Expect(read.text.plan.sourceId == Id("scenario:self-contained-guard-room"), "self-contained fixture should preserve source id");
	Expect(read.text.plan.authoredProfileCount() == 1, "self-contained fixture should parse one authored profile");
	Expect(read.text.plan.authoredControlCount() == 1, "self-contained fixture should parse one authored control");
	Expect(read.text.plan.authoredPlayerCommandCount() == 1, "self-contained fixture should parse one authored player command");
	Expect(!adapter.config.hasAsciiSourcePlanProfileScenarioConfig, "self-contained fixture should not need explicit source-plan conversion config");
	Expect(!adapter.config.asciiSourcePlanProfileScenario.hasDefaultFrame, "self-contained fixture should not supply C++ default frame");
	Expect(adapter.config.asciiSourcePlanProfileScenario.profileTraits.entries.empty(), "self-contained fixture should not supply C++ profile catalog");
	Expect(adapter.ok(), "self-contained fixture should adapt parsed source plan with empty converter config");
	Expect(adapter.asciiSourcePlanConversion.ok(), "self-contained fixture conversion should succeed");
	Expect(adapter.asciiSourcePlanConversion.profileTraitCatalog.built, "self-contained conversion should build profile catalog from TOML");
	Expect(adapter.asciiSourcePlanConversion.profileTraitCatalog.entryCount == 1, "self-contained conversion should use one TOML profile");
	Expect(adapter.profileScenario.profileTraits.contains(Id("profile:guard")), "self-contained profile scenario should contain TOML-authored profile");
	Expect(adapter.profileScenario.frames.size() == 1, "self-contained fixture should publish one shared profile frame");
	if (adapter.profileScenario.frames.size() == 1) {
		const iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition &frame =
			adapter.profileScenario.frames[0];
		Expect(frame.hasFrameId && frame.frameId == Id("frame:shared"), "self-contained shared frame should preserve frame id");
		Expect(frame.movementMap.id == Id("scenario:self-contained-guard-room"), "self-contained fallback frame should use promoted TOML map");
		Expect(frame.controlOverrides.size() == 1, "self-contained shared frame should carry NPC override");
		Expect(frame.playerFrame.playerIntents.size() == 1, "self-contained shared frame should carry player intent");
	}
	Expect(run.ran(), "self-contained fixture should run converted profile scenario");
	Expect(run.validation.ok(), "self-contained fixture runner validation should pass");
	Expect(run.frameCount == 1, "self-contained fixture should execute one shared frame");
	Expect(run.scenario.runner.acceptedCommandCount == 1, "self-contained fixture should accept authored player command");
	Expect(run.npcMovementPlannedRequestCount == 1, "self-contained fixture should plan NPC movement");
	Expect(run.npcMovedCount == 1, "self-contained fixture should move NPC");
	Expect(run.state.session.hasPlayer, "self-contained final state should preserve player");
	Expect(run.state.session.player.position.x == 4.5F && run.state.session.player.position.y == 1.5F, "self-contained fixture should move player to authored tile");
	Expect(run.state.npcActors.actors.size() == 1, "self-contained fixture should preserve one final NPC actor");
	if (!run.state.npcActors.actors.empty()) {
		const iggy::NpcActorState2D &actor = run.state.npcActors.actors.front();
		Expect(actor.npcId == Id("npc:guard"), "self-contained final actor should preserve id");
		Expect(actor.position.x == 2.5F && actor.position.y == 1.5F, "self-contained fixture should move actor one tile");
	}
	Expect(rows == expected, "self-contained fixture should render moved NPC and moved player");
}

void TestPlayerInteractionFixtureRunsScenarioAndTogglesTarget()
{
	const std::filesystem::path path = FixturePath("player_interacts_guard_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig adapterConfig;
	adapterConfig.hasAsciiSourcePlanProfileScenarioConfig = true;
	adapterConfig.asciiSourcePlanProfileScenario = ConverterConfig();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(
			packet,
			adapterConfig);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	const std::vector<std::string> rows = FinalDebugRows(read.text.plan, run.state);
	const std::vector<std::string> expected {
		"#######",
		"#A..@.#",
		"#.....#",
		"#######",
	};

	Expect(read.ok(), "player interaction fixture should read checked-in TOML fixture");
	Expect(read.text.plan.sourceId == Id("scenario:player-interacts-guard-room"), "player interaction fixture should preserve source id");
	Expect(read.text.plan.authoredInteractionTargetCount() == 1, "player interaction fixture should parse one authored interaction target");
	Expect(read.text.plan.authoredPlayerCommandCount() == 1, "player interaction fixture should parse one authored player command");
	Expect(adapter.ok(), "player interaction fixture should adapt parsed source plan");
	Expect(adapter.asciiSourcePlanConversion.ok(), "player interaction fixture conversion should succeed");
	Expect(adapter.asciiSourcePlanConversion.promotedInteractionTargetCount == 1, "player interaction conversion should promote one target");
	Expect(adapter.asciiSourcePlanConversion.promotedInteractionEffectEntryCount == 1, "player interaction conversion should promote one effect entry");
	Expect(adapter.asciiSourcePlanConversion.authoredPlayerCommandCount == 1, "player interaction conversion should consume authored player command");
	Expect(adapter.profileScenario.initialState.session.hasPlayer, "player interaction conversion should promote player start");
	Expect(adapter.profileScenario.initialState.interaction.targets.find(Id("target:door")) != nullptr, "player interaction conversion should publish interaction target");
	if (adapter.profileScenario.frames.size() == 1) {
		const iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition &frame =
			adapter.profileScenario.frames[0];
		Expect(frame.hasFrameId && frame.frameId == Id("frame:interact"), "player interaction frame should preserve frame id");
		Expect(frame.playerFrame.playerIntents.size() == 1, "player interaction frame should carry player intent");
		if (!frame.playerFrame.playerIntents.empty()) {
			const iggy::PlayerInputIntent2D &intent = frame.playerFrame.playerIntents[0];
			Expect(intent.type == iggy::PlayerInputIntent2DType::Interact, "player interaction command should become interact intent");
			Expect(intent.targetId == Id("target:door"), "player interaction command should preserve target id");
		}
	}
	Expect(run.ran(), "player interaction fixture should run converted profile scenario");
	Expect(run.frameCount == 1, "player interaction fixture should execute one frame");
	Expect(run.scenario.runner.acceptedCommandCount == 1, "player interaction fixture should accept authored interact command");
	Expect(run.scenario.runner.interactionChanged, "player interaction fixture should mark interaction changed");
	Expect(run.scenario.report.frames.size() == 1, "player interaction fixture should produce one frame report");
	if (run.scenario.report.frames.size() == 1) {
		const iggy::runtime::RuntimeGameplayOrchestratedFrameReport &frameReport =
			run.scenario.report.frames[0];
		Expect(frameReport.interactionChanged, "player interaction frame report should mark interaction changed");
		Expect(frameReport.player.interactionEventCount == 1, "player interaction frame report should count interaction event");
		Expect(frameReport.player.interactionChanged, "player interaction player report should mark interaction changed");
	}
	const iggy::InteractionTarget2D *finalTarget =
		run.state.interaction.targets.find(Id("target:door"));
	Expect(finalTarget != nullptr, "player interaction final state should preserve interaction target");
	if (finalTarget != nullptr) {
		Expect(!finalTarget->enabled, "player interaction final state should show target toggled disabled");
	}
	Expect(run.state.npcActors.actors.size() == 1, "player interaction fixture should preserve final NPC actor");
	if (!run.state.npcActors.actors.empty()) {
		const iggy::NpcActorState2D &actor = run.state.npcActors.actors.front();
		Expect(actor.npcId == Id("npc:guard"), "player interaction final actor should preserve id");
		Expect(actor.position.x == 1.5F && actor.position.y == 1.5F, "player interaction fixture should leave guard idle");
	}
	Expect(run.state.session.hasPlayer, "player interaction final state should preserve player");
	Expect(run.state.session.player.position.x == 4.5F && run.state.session.player.position.y == 1.5F, "player interaction fixture should leave player at start tile");
	Expect(rows == expected, "player interaction fixture should render final ASCII debug rows");
}

void TestPlayerPickupFixtureRunsScenarioAndPicksUpItem()
{
	const std::filesystem::path path = FixturePath("player_picks_up_item_room.toml");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult read =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);
	iggy::runtime::RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = iggy::runtime::RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = read.text.plan;
	iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterConfig adapterConfig;
	adapterConfig.hasAsciiSourcePlanProfileScenarioConfig = true;
	adapterConfig.asciiSourcePlanProfileScenario = ConverterConfig();

	const iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterResult adapter =
		iggy::runtime::RuntimeGameplayScenarioAuthoringAdapter {}.convert(
			packet,
			adapterConfig);
	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult run =
		iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(adapter.profileScenario);
	const std::vector<std::string> rows = FinalDebugRows(read.text.plan, run.state);
	const std::vector<std::string> expected {
		"#######",
		"#A.@..#",
		"#.....#",
		"#######",
	};

	Expect(read.ok(), "player pickup fixture should read checked-in TOML fixture");
	Expect(read.text.plan.sourceId == Id("scenario:player-picks-up-item-room"), "player pickup fixture should preserve source id");
	Expect(read.text.plan.authoredItemDropCount() == 1, "player pickup fixture should parse one authored item drop");
	Expect(read.text.plan.authoredInteractionTargetCount() == 1, "player pickup fixture should parse one pickup interaction target");
	Expect(read.text.plan.authoredPlayerCommandCount() == 2, "player pickup fixture should parse move and pickup player commands");
	Expect(adapter.ok(), "player pickup fixture should adapt parsed source plan");
	Expect(adapter.asciiSourcePlanConversion.ok(), "player pickup fixture conversion should succeed");
	Expect(adapter.asciiSourcePlanConversion.promotedItemDropCount == 1, "player pickup conversion should promote one item drop");
	Expect(adapter.asciiSourcePlanConversion.promotedInteractionTargetCount == 1, "player pickup conversion should promote one pickup target");
	Expect(adapter.asciiSourcePlanConversion.promotedInteractionEffectEntryCount == 1, "player pickup conversion should promote one pickup effect");
	Expect(adapter.asciiSourcePlanConversion.authoredPlayerCommandCount == 2, "player pickup conversion should consume both authored player commands");
	Expect(adapter.profileScenario.initialState.session.hasPlayer, "player pickup conversion should promote player start");
	Expect(adapter.profileScenario.initialState.inventory.drops.find(Id("drop:key")) != nullptr, "player pickup conversion should publish item drop");
	Expect(adapter.profileScenario.initialState.interaction.targets.find(Id("target:key")) != nullptr, "player pickup conversion should publish pickup target");
	Expect(adapter.profileScenario.frames.size() == 2, "player pickup conversion should publish two profile frames");
	if (adapter.profileScenario.frames.size() == 2) {
		const iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition &moveFrame =
			adapter.profileScenario.frames[0];
		const iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition &pickupFrame =
			adapter.profileScenario.frames[1];
		Expect(moveFrame.hasFrameId && moveFrame.frameId == Id("frame:move-to-key"), "player pickup move frame should preserve frame id");
		Expect(pickupFrame.hasFrameId && pickupFrame.frameId == Id("frame:pickup-key"), "player pickup pickup frame should preserve frame id");
		Expect(moveFrame.playerFrame.playerIntents.size() == 1, "player pickup move frame should carry one intent");
		Expect(pickupFrame.playerFrame.playerIntents.size() == 1, "player pickup pickup frame should carry one intent");
		if (!moveFrame.playerFrame.playerIntents.empty()) {
			const iggy::PlayerInputIntent2D &intent =
				moveFrame.playerFrame.playerIntents[0];
			Expect(intent.type == iggy::PlayerInputIntent2DType::MoveToTile, "player pickup first command should become move-to-tile intent");
			Expect(intent.tile.x == 3 && intent.tile.y == 1, "player pickup first command should preserve target tile");
		}
		if (!pickupFrame.playerFrame.playerIntents.empty()) {
			const iggy::PlayerInputIntent2D &intent =
				pickupFrame.playerFrame.playerIntents[0];
			Expect(intent.type == iggy::PlayerInputIntent2DType::Interact, "player pickup command should reuse interact intent");
			Expect(intent.targetId == Id("target:key"), "player pickup command should preserve pickup target id");
		}
	}
	Expect(run.ran(), "player pickup fixture should run converted profile scenario");
	Expect(run.frameCount == 2, "player pickup fixture should execute move and pickup frames");
	Expect(run.scenario.runner.acceptedCommandCount == 2, "player pickup fixture should accept both authored player commands");
	Expect(run.scenario.runner.pickedUpCount == 1, "player pickup fixture should record one pickup");
	Expect(run.scenario.runner.inventoryChanged, "player pickup fixture should mark inventory changed");
	Expect(run.inventoryEventCount == 3, "player pickup fixture should record add, consume, and picked-up inventory events");
	Expect(run.state.session.hasPlayer, "player pickup final state should preserve player");
	Expect(run.state.session.player.position.x == 3.5F && run.state.session.player.position.y == 1.5F, "player pickup final state should keep player at moved tile");
	const iggy::InventoryItemStack2D *stack =
		run.state.inventory.inventory.find(Id("item:key"));
	Expect(stack != nullptr, "player pickup final inventory should contain picked item");
	if (stack != nullptr) {
		Expect(stack->count == 1, "player pickup final inventory should preserve picked count");
	}
	const iggy::LevelItemDrop2D *drop =
		run.state.inventory.drops.find(Id("drop:key"));
	Expect(drop != nullptr, "player pickup final drops should preserve consumed drop record");
	if (drop != nullptr) {
		Expect(!drop->enabled, "player pickup final drop should be disabled after pickup");
	}
	Expect(run.state.npcActors.actors.size() == 1, "player pickup fixture should preserve final NPC actor");
	if (!run.state.npcActors.actors.empty()) {
		const iggy::NpcActorState2D &actor = run.state.npcActors.actors.front();
		Expect(actor.npcId == Id("npc:guard"), "player pickup final actor should preserve id");
		Expect(actor.position.x == 1.5F && actor.position.y == 1.5F, "player pickup fixture should leave guard idle");
	}
	Expect(rows == expected, "player pickup fixture should render moved player and consumed item");
}

} // namespace

int main()
{
	TestEmptyPathDoesNotCallTextReader();
	TestMissingPathReportsMissingFile();
	TestDirectoryReportsNonRegularFile();
	TestValidFileReadsAndParses();
	TestInvalidTomlFilePreservesNestedDiagnostics();
	TestValidFixtureReadsAndParses();
	TestCorruptFixturePreservesNestedParserDiagnostics();
	TestSemanticInvalidFixturePreservesNestedSourceValidation();
	TestValidFixtureConvertsToValidatedProfileScenario();
	TestValidFixtureFeedsAuthoringAdapterThroughParsedSourcePlan();
	TestValidFixtureRunsScenarioAndRendersFinalDebugRows();
	TestMovingFixtureRunsScenarioAndMovesNpcFromAuthoredControl();
	TestMultiFrameFixtureRunsScenarioAndMovesNpcAcrossFrames();
	TestPlayerAndGuardFixtureRunsSharedFrameThroughScenario();
	TestSelfContainedFixtureRunsWithEmptyConverterConfig();
	TestPlayerInteractionFixtureRunsScenarioAndTogglesTarget();
	TestPlayerPickupFixtureRunsScenarioAndPicksUpItem();

	CleanupTempRoot();

	if (Failures != 0) {
		std::cerr << Failures << " runtime gameplay ASCII source plan TOML file reader test(s) failed\n";
		return 1;
	}

	return 0;
}
