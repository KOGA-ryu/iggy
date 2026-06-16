#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

#include <iostream>
#include <string>

#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "support/LevelMapFixtures.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue :
		result.issues) {
		if (issue.code == code) {
			return true;
		}
	}
	return false;
}

const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *FindIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode code,
	const std::string &key = {})
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue :
		result.issues) {
		if (issue.code == code && (key.empty() || issue.key == key)) {
			return &issue;
		}
	}
	return nullptr;
}

const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *FindSourceIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue :
		result.issues) {
		if (issue.code == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid &&
			issue.sourceIssue.code == code) {
			return &issue;
		}
	}
	return nullptr;
}

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult Read(
	const std::string &text)
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReader {}.read(text);
}

std::size_t LineOfNth(
	const std::string &text,
	const std::string &needle,
	std::size_t occurrence = 0)
{
	std::size_t line = 1;
	std::size_t matched = 0;
	std::size_t lineStart = 0;
	while (lineStart <= text.size()) {
		const std::size_t lineEnd = text.find('\n', lineStart);
		const std::string current = lineEnd == std::string::npos
			? text.substr(lineStart)
			: text.substr(lineStart, lineEnd - lineStart);
		if (current.find(needle) != std::string::npos) {
			if (matched == occurrence) {
				return line;
			}
			++matched;
		}
		if (lineEnd == std::string::npos) {
			break;
		}
		lineStart = lineEnd + 1;
		++line;
	}
	return 0;
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
	Expect(result.built, "TOML reader acceptance profile catalog should build");
	return result.catalog;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition DefaultFrame()
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:toml-reader");
	frame.movementMap = iggy::test::MapFromRows({
		"#######",
		"#.....#",
		"#.....#",
		"#######",
	});
	frame.movementMap.id = Id("level:toml-reader");
	return frame;
}

std::string RootGridToml()
{
	return R"toml(
format_id = "iggy:ascii-source-plan"
version = 1
source_id = "scenario:guard-room"
source_ref = "authoring:manual-fixture"

[grid]
width = 7
height = 4
background = "."
rows = [
  "#######",
  "#.....#",
  "#.....#",
  "#######",
]
)toml";
}

std::string RootGridLegendToml()
{
	return R"toml(
format_id = "iggy:ascii-source-plan"
version = 1
source_id = "scenario:guard-room"
source_ref = "authoring:manual-fixture"

[grid]
width = 7
height = 4
background = "."
rows = [
  "#######",
  "#A...@#",
  "#.....#",
  "#######",
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

[[legend]]
glyph = "A"
kind = "actor"
role_id = "role:npc"
role_tags = ["tag:guard"]
maps_to_scenario_marker = true
scenario_marker_kind = "actor"
target_marker_id = "npc:guard"
target_profile_id = "profile:guard"

[[legend]]
glyph = "@"
kind = "player_start"
role_id = "role:player-start"
maps_to_scenario_marker = true
scenario_marker_kind = "player_start"

[[legend]]
glyph = "#"
kind = "terrain"
role_id = "role:wall"
role_tags = ["tag:blocking"]
maps_to_scenario_marker = true
scenario_marker_kind = "wall"

[[legend]]
glyph = "."
kind = "background"
role_id = "role:floor"
maps_to_scenario_marker = true
scenario_marker_kind = "floor"
)toml";
}

std::string RootGridLegendCellsRegionsToml()
{
	return RootGridLegendToml() + R"toml(

[[cells]]
id = "cell:guard"
row = 1
column = 1
glyph = "A"
local_tile = { x = 1, y = 1 }
local_position = { x = 1.5, y = 1.5 }
cell_bounds = { min_x = 1.0, min_y = 1.0, max_x = 2.0, max_y = 2.0 }
role_tags = ["tag:guard", "tag:namespaced"]
marker_id = "npc:guard"
profile_id = "profile:guard"

[[regions]]
id = "region:room"
min_row = 0
min_column = 0
max_row = 3
max_column = 6
role_tags = ["tag:room"]
)toml";
}

std::string RootGridLegendCellsRegionsControlToml()
{
	return RootGridLegendCellsRegionsToml() + R"toml(

[[frame_controls]]
frame_id = "frame:one"
npc = "npc:guard"
behavior = "seeking"
move_mode = "walk"
target = { x = 2.5, y = 1.5 }
)toml";
}

std::string RootGridLegendCellsRegionsPlayerCommandToml()
{
	return RootGridLegendCellsRegionsToml() + R"toml(

[[frame_player_commands]]
frame_id = "frame:player-move"
command = "move_to_tile"
x = 3
y = 1
)toml";
}

std::string RootGridLegendCellsRegionsInteractionTargetToml()
{
	return RootGridLegendCellsRegionsToml() + R"toml(

[[interaction_targets]]
target_id = "plain-target"
kind = "door"
tile = { x = 3, y = 1 }
position = { x = 3.5, y = 1.5 }
radius = 1.25
enabled = false
effect = "toggle_target"
effect_target_id = "plain-target"
enabled_value = true
)toml";
}

void TestEmptyInputFailsDeterministically()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read("");

	Expect(!result.ok(), "empty input should not parse");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SyntaxInvalid, "empty input should report syntax invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError), "empty input should report syntax issue");
	Expect(result.issueCount == 1 && result.syntaxIssueCount == 1, "empty input should mirror syntax counts");
	Expect(result.input.empty(), "empty input should be preserved by value");
}

void TestUnsupportedInputFailsDeterministically()
{
	const std::string text = "unsupported = true\n[grid]\nwidth = 1\nheight = 1\nrows = [\".\"]\n";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "unsupported input should not parse");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported input should report unsupported status");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape), "unsupported input should report unsupported issue");
	Expect(result.issueCount == 1 && result.unsupportedIssueCount == 1, "unsupported input should mirror unsupported counts");
	Expect(result.input == text && result.inputSize == text.size(), "unsupported input should be copied for diagnostics");
}

void TestRootAndGridParse()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(RootGridToml());

	Expect(result.ok(), "valid root/grid TOML should parse");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::Parsed, "valid root/grid TOML should report Parsed");
	Expect(result.sourceValidation.ok(), "valid root/grid TOML should pass nested source validation");
	Expect(result.plan.formatId == Id("iggy:ascii-source-plan"), "root format id should parse");
	Expect(result.plan.version == 1, "root version should parse");
	Expect(result.plan.hasSourceId && result.plan.sourceId == Id("scenario:guard-room"), "root source id should parse");
	Expect(result.plan.hasSourceRef && result.plan.sourceRef == Id("authoring:manual-fixture"), "root source ref should parse");
	Expect(result.plan.grid.width == 7 && result.plan.grid.height == 4, "grid dimensions should parse");
	Expect(result.plan.grid.backgroundGlyph == '.', "grid background glyph should parse");
	Expect(result.plan.grid.rows.size() == 4, "grid rows should parse");
	Expect(result.plan.grid.rows[1] == "#.....#", "grid row contents should be preserved");
}

void TestSourceLocationsAreCaptured()
{
	const std::string text = RootGridLegendCellsRegionsToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "source location fixture should parse");
	Expect(result.sourceLocations.gridTableLine == LineOfNth(text, "[grid]"), "grid table line should be captured");
	Expect(result.sourceLocations.gridRowsLine == LineOfNth(text, "rows = ["), "grid rows key line should be captured");
	Expect(result.sourceLocations.noClaimsTableLine == LineOfNth(text, "[no_claims]"), "no_claims table line should be captured");
	Expect(result.sourceLocations.promotionTableLine == LineOfNth(text, "[promotion]"), "promotion table line should be captured");
	Expect(result.sourceLocations.legendTableLines.size() == 4, "legend table lines should match parsed legend count");
	Expect(result.sourceLocations.legendTableLines[0] == LineOfNth(text, "[[legend]]", 0), "first legend table line should be captured");
	Expect(result.sourceLocations.legendTableLines[1] == LineOfNth(text, "[[legend]]", 1), "second legend table line should be captured");
	Expect(result.sourceLocations.annotatedCellTableLines.size() == 1, "cell table lines should match parsed cell count");
	Expect(result.sourceLocations.annotatedCellTableLines[0] == LineOfNth(text, "[[cells]]"), "cell table line should be captured");
	Expect(result.sourceLocations.regionTableLines.size() == 1, "region table lines should match parsed region count");
	Expect(result.sourceLocations.regionTableLines[0] == LineOfNth(text, "[[regions]]"), "region table line should be captured");
}

void TestCommentsAndInlineRowsParse()
{
	const std::string text = R"toml(
# leading comment
format_id = "iggy:ascii-source-plan" # trailing comment
[grid]
width = 3
height = 2
background = "."
rows = ["###", "#.#"] # comment outside quoted strings
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(result.ok(), "comments and inline rows should parse");
	Expect(result.plan.grid.rows.size() == 2, "inline row array should parse");
	Expect(result.plan.grid.rows[0] == "###" && result.plan.grid.rows[1] == "#.#", "inline row values should be preserved");
}

void TestMissingGridFailsAsSyntax()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read("format_id = \"iggy:ascii-source-plan\"\nversion = 1\n");

	Expect(!result.ok(), "missing grid should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SyntaxInvalid, "missing grid should report syntax invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::MissingTable), "missing grid should report missing table");
}

void TestWrongTypeFailsAsTypeInvalid()
{
	const std::string text = R"toml(
format_id = "iggy:ascii-source-plan"
[grid]
width = "7"
height = 1
background = "."
rows = ["."]
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "wrong typed width should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "wrong typed width should report type invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType), "wrong typed width should report wrong type");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType, "width");
	Expect(issue != nullptr, "wrong typed width should be findable by key");
	if (issue != nullptr) {
		Expect(issue->line > 0, "wrong typed width should report line");
		Expect(issue->table == "grid", "wrong typed width should report grid table");
		Expect(issue->key == "width", "wrong typed width should report width key");
		Expect(!issue->hasTableIndex, "wrong typed width should not report repeated table index");
	}
}

void TestRaggedRowsSurfaceSourcePlanValidation()
{
	std::string text = RootGridToml();
	const std::size_t start = text.find("\"#.....#\"");
	text.replace(start, std::string("\"#.....#\"").size(), "\"#....#\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "ragged parsed rows should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "ragged parsed rows should report source plan invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid), "ragged parsed rows should add source-plan issue");
	Expect(!result.sourceValidation.ok(), "ragged parsed rows should preserve nested validation failure");
	Expect(result.sourcePlanIssueCount == result.sourceValidation.issueCount, "ragged parsed rows should mirror one TOML issue per nested source issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::RaggedRow);
	Expect(issue != nullptr, "ragged parsed rows should mirror ragged row issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "rows = ["), "mirrored ragged row issue should report rows key line");
		Expect(issue->table == "grid", "mirrored ragged row issue should report grid table");
		Expect(!issue->hasTableIndex, "mirrored ragged row issue should not report table index");
	}
}

void TestInvalidBackgroundGlyphFailsAsTypeInvalid()
{
	std::string text = RootGridToml();
	const std::size_t start = text.find("background = \".\"");
	text.replace(start, std::string("background = \".\"").size(), "background = \"..\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "multi-character background glyph should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "multi-character background glyph should report type invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString), "multi-character background glyph should report invalid glyph");
}

void TestNoClaimsPromotionAndLegendParse()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(RootGridLegendToml());

	Expect(result.ok(), "valid no-claims/promotion/legend TOML should parse");
	Expect(result.plan.safeForAuthoring(), "false no-claims and promotion flags should remain safe");
	Expect(result.plan.legend.size() == 4, "legend entries should parse");
	Expect(result.plan.legend[0].glyph == 'A', "actor legend glyph should parse");
	Expect(result.plan.legend[0].kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor, "actor legend kind should parse");
	Expect(result.plan.legend[0].roleId == Id("role:npc"), "actor legend role id should parse");
	Expect(result.plan.legend[0].roleTags.size() == 1 && result.plan.legend[0].roleTags[0] == Id("tag:guard"), "actor legend role tags should parse");
	Expect(result.plan.legend[0].mapsToScenarioMarker, "actor legend marker mapping should parse");
	Expect(result.plan.legend[0].scenarioMarkerKind == iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor, "actor marker kind should parse");
	Expect(result.plan.legend[0].targetMarkerId == Id("npc:guard"), "actor target marker id should parse exactly");
	Expect(result.plan.legend[0].targetProfileId == Id("profile:guard"), "actor target profile id should parse exactly");
	Expect(result.plan.legend[1].kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart, "player-start legend kind should parse");
	Expect(result.plan.legend[1].scenarioMarkerKind == iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart, "player-start marker kind should parse");
	Expect(result.plan.legend[2].kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Terrain, "terrain legend kind should parse");
	Expect(result.plan.legend[3].kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Background, "background legend kind should parse");
}

void TestUnsafeNoClaimsSurfaceSourcePlanValidation()
{
	std::string text = RootGridLegendToml();
	const std::size_t start = text.find("runtime_truth = false");
	text.replace(start, std::string("runtime_truth = false").size(), "runtime_truth = true");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "unsafe no-claim should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "unsafe no-claim should report source invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid), "unsafe no-claim should add source-plan issue");
	Expect(result.sourcePlanIssueCount == result.sourceValidation.issueCount, "unsafe no-claim should mirror one TOML issue per nested source issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsafeNoClaims);
	Expect(issue != nullptr, "unsafe no-claim should mirror unsafe no-claims source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[no_claims]"), "mirrored unsafe no-claim should report no_claims table line");
		Expect(issue->table == "no_claims", "mirrored unsafe no-claim should report no_claims table");
		Expect(!issue->hasTableIndex, "mirrored unsafe no-claim should not report repeated table index");
	}
}

void TestUnsafePromotionSurfaceSourcePlanValidation()
{
	std::string text = RootGridLegendToml();
	const std::size_t start = text.find("runtime_execution = false");
	text.replace(start, std::string("runtime_execution = false").size(), "runtime_execution = true");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "unsafe promotion should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "unsafe promotion should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsafePromotionPolicy);
	Expect(issue != nullptr, "unsafe promotion should mirror unsafe promotion source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[promotion]"), "mirrored unsafe promotion should report promotion table line");
		Expect(issue->table == "promotion", "mirrored unsafe promotion should report promotion table");
		Expect(!issue->hasTableIndex, "mirrored unsafe promotion should not report repeated table index");
	}
}

void TestDuplicateLegendGlyphSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendToml();
	const std::size_t start = text.find("glyph = \"@\"");
	text.replace(start, std::string("glyph = \"@\"").size(), "glyph = \"A\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "duplicate legend glyph should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "duplicate legend glyph should report source invalid");
	Expect(result.sourceValidation.duplicateGlyphCount == 1, "duplicate legend glyph should preserve nested duplicate count");
	Expect(result.sourcePlanIssueCount == result.sourceValidation.issueCount, "duplicate legend glyph should mirror one TOML issue per nested source issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::DuplicateGlyph);
	Expect(issue != nullptr, "duplicate legend glyph should mirror duplicate source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[legend]]", 1), "mirrored duplicate legend glyph should report offending legend table line");
		Expect(issue->table == "legend", "mirrored duplicate legend glyph should report legend table");
		Expect(issue->hasTableIndex && issue->tableIndex == 1, "mirrored duplicate legend glyph should report second legend index");
		Expect(issue->key == "glyph", "mirrored duplicate legend glyph should report glyph key");
		Expect(issue->sourceIssue.index == 1 && issue->sourceIssue.firstIndex == 0, "mirrored duplicate legend glyph should copy source indexes");
		Expect(issue->sourceIssue.glyph == 'A', "mirrored duplicate legend glyph should copy source glyph");
	}
}

void TestUnknownLegendEnumFailsAsTypeInvalid()
{
	std::string text = RootGridLegendToml();
	const std::size_t start = text.find("kind = \"actor\"");
	text.replace(start, std::string("kind = \"actor\"").size(), "kind = \"monster\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "unknown legend enum should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "unknown legend enum should report type invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnknownEnumValue), "unknown legend enum should report unknown enum");
}

void TestInvalidLegendGlyphFailsAsTypeInvalid()
{
	std::string text = RootGridLegendToml();
	const std::size_t start = text.find("glyph = \"@\"");
	text.replace(start, std::string("glyph = \"@\"").size(), "glyph = \"@@\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "invalid legend glyph should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "invalid legend glyph should report type invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString), "invalid legend glyph should report invalid glyph");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString, "glyph");
	Expect(issue != nullptr, "invalid second legend glyph should be findable");
	if (issue != nullptr) {
		Expect(issue->line > 0, "invalid second legend glyph should report line");
		Expect(issue->table == "legend", "invalid second legend glyph should report legend table");
		Expect(issue->hasTableIndex && issue->tableIndex == 1, "invalid second legend glyph should report zero-based legend index");
		Expect(issue->key == "glyph", "invalid second legend glyph should report glyph key");
	}
}

void TestCellsAndRegionsParse()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(RootGridLegendCellsRegionsToml());

	Expect(result.ok(), "valid cells/regions TOML should parse");
	Expect(result.plan.annotatedCells.size() == 1, "one annotated cell should parse");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell =
		result.plan.annotatedCells[0];
	Expect(cell.hasCellId && cell.cellId == Id("cell:guard"), "cell id should parse exactly");
	Expect(cell.row == 1 && cell.column == 1, "cell row/column should parse");
	Expect(cell.glyph == 'A', "cell glyph should parse");
	Expect(cell.localTile.present && cell.localTile.x == 1 && cell.localTile.y == 1, "local tile inline table should parse");
	Expect(cell.localPosition.present && cell.localPosition.x == 1.5 && cell.localPosition.y == 1.5, "local position inline table should parse");
	Expect(cell.cellBounds.present && cell.cellBounds.minX == 1.0 && cell.cellBounds.maxY == 2.0, "cell bounds inline table should parse");
	Expect(cell.roleTags.size() == 2 && cell.roleTags[1] == Id("tag:namespaced"), "cell role tags should parse exactly");
	Expect(cell.markerId == Id("npc:guard"), "cell marker id should parse exactly");
	Expect(cell.profileId == Id("profile:guard"), "cell profile id should parse exactly");

	Expect(result.plan.regions.size() == 1, "one region should parse");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanRegion &region =
		result.plan.regions[0];
	Expect(region.hasRegionId && region.regionId == Id("region:room"), "region id should parse exactly");
	Expect(region.minRow == 0 && region.minColumn == 0 && region.maxRow == 3 && region.maxColumn == 6, "region bounds should parse");
	Expect(region.roleTags.size() == 1 && region.roleTags[0] == Id("tag:room"), "region role tags should parse exactly");
}

void TestCellGlyphMismatchSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsToml();
	const std::size_t start = text.find("glyph = \"A\"\nlocal_tile");
	text.replace(start, std::string("glyph = \"A\"").size(), "glyph = \"@\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "cell glyph mismatch should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "cell glyph mismatch should report source invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid), "cell glyph mismatch should add source-plan issue");
	Expect(result.sourcePlanIssueCount == result.sourceValidation.issueCount, "cell glyph mismatch should mirror one TOML issue per nested source issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellGlyphMismatch);
	Expect(issue != nullptr, "cell glyph mismatch should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[cells]]"), "mirrored cell glyph mismatch should report offending cell table line");
		Expect(issue->table == "cells", "mirrored cell glyph mismatch should report cells table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored cell glyph mismatch should report first cell index");
		Expect(issue->sourceIssue.row == 1 && issue->sourceIssue.column == 1, "mirrored cell glyph mismatch should copy row/column");
		Expect(issue->sourceIssue.glyph == '@', "mirrored cell glyph mismatch should copy annotated glyph");
	}
}

void TestCellOutOfBoundsSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsToml();
	const std::size_t start = text.find("column = 1");
	text.replace(start, std::string("column = 1").size(), "column = 8");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "out-of-bounds cell should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "out-of-bounds cell should report source invalid");
	Expect(result.sourcePlanIssueCount == result.sourceValidation.issueCount, "out-of-bounds cell should mirror one TOML issue per nested source issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellOutOfBounds);
	Expect(issue != nullptr, "out-of-bounds cell should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[cells]]"), "mirrored out-of-bounds cell should report offending cell table line");
		Expect(issue->table == "cells", "mirrored out-of-bounds cell should report cells table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored out-of-bounds cell should report first cell index");
		Expect(issue->sourceIssue.column == 8, "mirrored out-of-bounds cell should copy source column");
	}
}

void TestWrongTypedCellFieldFailsAsTypeInvalid()
{
	std::string text = RootGridLegendCellsRegionsToml();
	const std::size_t start = text.find("row = 1");
	text.replace(start, std::string("row = 1").size(), "row = \"1\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "wrong typed cell row should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "wrong typed cell row should report type invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType), "wrong typed cell row should report wrong type");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType, "row");
	Expect(issue != nullptr, "wrong typed cell row should be findable");
	if (issue != nullptr) {
		Expect(issue->line > 0, "wrong typed cell row should report line");
		Expect(issue->table == "cells", "wrong typed cell row should report cells table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "wrong typed cell row should report first cell index");
		Expect(issue->key == "row", "wrong typed cell row should report row key");
	}
}

void TestUnsupportedInlineCellShapeFailsAsUnsupported()
{
	std::string text = RootGridLegendCellsRegionsToml();
	const std::size_t start = text.find("local_tile = { x = 1, y = 1 }");
	text.replace(start, std::string("local_tile = { x = 1, y = 1 }").size(), "local_tile = { x = 1, z = 1 }");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "unsupported local tile shape should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported local tile shape should report unsupported");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape), "unsupported local tile shape should report unsupported issue");
}

void TestInvalidRegionBoundsSurfaceSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsToml();
	const std::size_t start = text.find("max_row = 3");
	text.replace(start, std::string("max_row = 3").size(), "max_row = 5");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "out-of-bounds region should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "out-of-bounds region should report source invalid");
	Expect(result.sourcePlanIssueCount == result.sourceValidation.issueCount, "out-of-bounds region should mirror one TOML issue per nested source issue");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::RegionOutOfBounds);
	Expect(issue != nullptr, "out-of-bounds region should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[regions]]"), "mirrored out-of-bounds region should report offending region table line");
		Expect(issue->table == "regions", "mirrored out-of-bounds region should report regions table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored out-of-bounds region should report first region index");
		Expect(issue->sourceIssue.row == 5, "mirrored out-of-bounds region should copy max row");
	}
}

void TestUnsupportedRegionKeyReportsContext()
{
	std::string text = RootGridLegendCellsRegionsToml();
	const std::size_t start = text.find("role_tags = [\"tag:room\"]");
	text.insert(start + std::string("role_tags = [\"tag:room\"]").size(), "\nunexpected = true");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "unsupported region key should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported region key should report unsupported status");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape, "unexpected");
	Expect(issue != nullptr, "unsupported region key should be findable by raw key");
	if (issue != nullptr) {
		Expect(issue->line > 0, "unsupported region key should report line");
		Expect(issue->table == "regions", "unsupported region key should report regions table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unsupported region key should report first region index");
		Expect(issue->detail.find("unsupported regions key") != std::string::npos, "unsupported region key should preserve detail");
	}
}

void TestFrameControlsParse()
{
	const std::string text = RootGridLegendCellsRegionsControlToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "valid frame control TOML should parse");
	Expect(result.plan.authoredControls.size() == 1, "one frame control should parse");
	Expect(result.sourceLocations.frameControlTableLines.size() == 1, "frame control source location should be captured");
	Expect(result.sourceLocations.frameControlTableLines[0] == LineOfNth(text, "[[frame_controls]]"), "frame control source location should preserve table line");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredControl &control =
		result.plan.authoredControls[0];
	Expect(control.hasFrameId && control.frameId == Id("frame:one"), "frame control should parse optional frame id");
	Expect(control.npcId == Id("npc:guard"), "frame control should parse exact npc id");
	Expect(control.behavior == iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking, "frame control should parse seeking behavior");
	Expect(control.moveMode == iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk, "frame control should parse walk move mode");
	Expect(control.targetPosition.present && control.targetPosition.x == 2.5 && control.targetPosition.y == 1.5, "frame control should parse target inline table");
}

void TestFrameControlInvalidFieldsReportContext()
{
	std::string text = RootGridLegendCellsRegionsControlToml();
	const std::size_t start = text.find("move_mode = \"walk\"");
	text.replace(start, std::string("move_mode = \"walk\"").size(), "move_mode = \"teleport\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "unknown frame control move mode should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "unknown frame control move mode should report type invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnknownEnumValue, "move_mode");
	Expect(issue != nullptr, "unknown frame control move mode issue should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "frame_controls", "unknown frame control move mode should report frame_controls table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unknown frame control move mode should report first table index");
		Expect(issue->detail == "teleport", "unknown frame control move mode should preserve raw value");
	}
}

void TestFrameControlMissingTargetSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsControlToml();
	const std::size_t start = text.find("target = { x = 2.5, y = 1.5 }\n");
	text.erase(start, std::string("target = { x = 2.5, y = 1.5 }\n").size());
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "seeking frame control without target should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "seeking frame control without target should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredControlMissingTarget);
	Expect(issue != nullptr, "seeking frame control without target should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[frame_controls]]"), "mirrored frame control target issue should report table line");
		Expect(issue->table == "frame_controls", "mirrored frame control target issue should report frame_controls table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored frame control target issue should report first control index");
		Expect(issue->key == "target", "mirrored frame control target issue should report target key");
		Expect(issue->sourceIssue.id == Id("npc:guard"), "mirrored frame control target issue should copy npc id");
	}
}

void TestFrameControlUnsupportedTargetShapeReportsContext()
{
	std::string text = RootGridLegendCellsRegionsControlToml();
	const std::size_t start = text.find("target = { x = 2.5, y = 1.5 }");
	text.replace(start, std::string("target = { x = 2.5, y = 1.5 }").size(), "target = { x = 2.5, z = 1.5 }");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "unsupported frame control target shape should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported frame control target shape should report unsupported");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape, "target");
	Expect(issue != nullptr, "unsupported frame control target shape issue should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "frame_controls", "unsupported frame control target shape should report frame_controls table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unsupported frame control target shape should report first table index");
	}
}

void TestInteractionTargetsParse()
{
	const std::string text = RootGridLegendCellsRegionsInteractionTargetToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "valid interaction target TOML should parse");
	Expect(result.plan.authoredInteractionTargets.size() == 1, "one interaction target should parse");
	Expect(result.sourceLocations.interactionTargetTableLines.size() == 1, "interaction target source location should be captured");
	Expect(result.sourceLocations.interactionTargetTableLines[0] == LineOfNth(text, "[[interaction_targets]]"), "interaction target source location should preserve table line");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &target =
		result.plan.authoredInteractionTargets[0];
	Expect(target.targetId == Id("plain-target"), "interaction target should parse exact target id");
	Expect(target.kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Door, "interaction target should parse door kind");
	Expect(target.localTile.present && target.localTile.x == 3 && target.localTile.y == 1, "interaction target should parse tile");
	Expect(target.localPosition.present && target.localPosition.x == 3.5 && target.localPosition.y == 1.5, "interaction target should parse point position");
	Expect(target.radius == 1.25, "interaction target should parse radius");
	Expect(!target.enabled, "interaction target should parse enabled flag");
	Expect(target.effect == iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionEffectKind::ToggleTarget, "interaction target should parse effect kind");
	Expect(target.effectTargetId == Id("plain-target"), "interaction target should parse effect target id");
	Expect(target.enabledValue, "interaction target should parse enabled value");
}

void TestInteractionTargetInvalidFieldsReportContext()
{
	std::string text = RootGridLegendCellsRegionsInteractionTargetToml();
	const std::size_t start = text.find("kind = \"door\"");
	text.replace(start, std::string("kind = \"door\"").size(), "kind = \"portal\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "unknown interaction target kind should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "unknown interaction target kind should report type invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnknownEnumValue, "kind");
	Expect(issue != nullptr, "unknown interaction target kind issue should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "interaction_targets", "unknown interaction target kind should report interaction_targets table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unknown interaction target kind should report first table index");
		Expect(issue->detail == "portal", "unknown interaction target kind should preserve raw value");
	}
}

void TestInteractionTargetMissingIdSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsInteractionTargetToml();
	const std::size_t start = text.find("target_id = \"plain-target\"\n");
	text.erase(start, std::string("target_id = \"plain-target\"\n").size());
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "interaction target without id should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "interaction target without id should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredInteractionTargetMissingId);
	Expect(issue != nullptr, "interaction target without id should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[interaction_targets]]"), "mirrored interaction target id issue should report table line");
		Expect(issue->table == "interaction_targets", "mirrored interaction target id issue should report interaction_targets table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored interaction target id issue should report first target index");
		Expect(issue->key == "target_id", "mirrored interaction target id issue should report target_id key");
	}
}

void TestInteractionTargetUnsupportedTileShapeReportsContext()
{
	std::string text = RootGridLegendCellsRegionsInteractionTargetToml();
	const std::size_t start = text.find("tile = { x = 3, y = 1 }");
	text.replace(start, std::string("tile = { x = 3, y = 1 }").size(), "tile = { x = 3, z = 1 }");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "unsupported interaction target tile shape should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported interaction target tile shape should report unsupported");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape, "tile");
	Expect(issue != nullptr, "unsupported interaction target tile issue should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "interaction_targets", "unsupported interaction target tile should report interaction_targets table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unsupported interaction target tile should report first table index");
	}
}

void TestFramePlayerCommandsParse()
{
	const std::string text = RootGridLegendCellsRegionsPlayerCommandToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "valid frame player command TOML should parse");
	Expect(result.plan.authoredPlayerCommands.size() == 1, "one frame player command should parse");
	Expect(result.sourceLocations.framePlayerCommandTableLines.size() == 1, "frame player command source location should be captured");
	Expect(result.sourceLocations.framePlayerCommandTableLines[0] == LineOfNth(text, "[[frame_player_commands]]"), "frame player command source location should preserve table line");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand
		&command = result.plan.authoredPlayerCommands[0];
	Expect(command.hasFrameId && command.frameId == Id("frame:player-move"), "frame player command should parse optional frame id");
	Expect(command.command == iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::MoveToTile, "frame player command should parse move_to_tile command");
	Expect(command.hasTargetTile && command.hasTargetTileX && command.hasTargetTileY, "frame player command should parse complete target tile");
	Expect(command.targetTile.x == 3 && command.targetTile.y == 1, "frame player command should preserve target tile coordinates");
	Expect(command.hasDeclarationIndex && command.declarationIndex == 0, "frame player command should preserve authored declaration order");
}

void TestFramePlayerCommandInvalidFieldsReportContext()
{
	std::string text = RootGridLegendCellsRegionsPlayerCommandToml();
	const std::size_t start = text.find("command = \"move_to_tile\"");
	text.replace(
		start,
		std::string("command = \"move_to_tile\"").size(),
		"command = \"jump\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "unknown frame player command should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "unknown frame player command should report type invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnknownEnumValue, "command");
	Expect(issue != nullptr, "unknown frame player command issue should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "frame_player_commands", "unknown frame player command should report frame_player_commands table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unknown frame player command should report first table index");
		Expect(issue->detail == "jump", "unknown frame player command should preserve raw value");
	}
}

void TestFramePlayerCommandWrongTypedTargetReportsContext()
{
	std::string text = RootGridLegendCellsRegionsPlayerCommandToml();
	const std::size_t start = text.find("x = 3");
	text.replace(start, std::string("x = 3").size(), "x = \"3\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "wrong typed frame player command target should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "wrong typed frame player target should report type invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType, "x");
	Expect(issue != nullptr, "wrong typed frame player target should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "frame_player_commands", "wrong typed frame player target should report frame_player_commands table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "wrong typed frame player target should report first table index");
		Expect(issue->key == "x", "wrong typed frame player target should report x key");
	}
}

void TestFramePlayerCommandMissingTargetSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsPlayerCommandToml();
	const std::size_t start = text.find("y = 1\n");
	text.erase(start, std::string("y = 1\n").size());
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "move_to_tile frame player command without full target should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "move_to_tile frame player command without full target should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget);
	Expect(issue != nullptr, "move_to_tile frame player command without full target should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[frame_player_commands]]"), "mirrored frame player command target issue should report table line");
		Expect(issue->table == "frame_player_commands", "mirrored frame player command target issue should report frame_player_commands table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored frame player command target issue should report first player command index");
		Expect(issue->key == "target", "mirrored frame player command target issue should report target key");
	}
}

void TestFrameAuthoredDeclarationOrderSpansControlsAndPlayerCommands()
{
	const std::string text = RootGridLegendCellsRegionsToml() + R"toml(

[[frame_player_commands]]
frame_id = "frame:first"
command = "move_to_tile"
x = 3
y = 1

[[frame_controls]]
frame_id = "frame:second"
npc = "npc:guard"
behavior = "seeking"
move_mode = "walk"
target = { x = 2.5, y = 1.5 }
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "mixed authored frame declarations should parse");
	Expect(result.plan.authoredPlayerCommands[0].declarationIndex == 0, "player command should get first declaration index");
	Expect(result.plan.authoredControls[0].declarationIndex == 1, "NPC control should get second declaration index");
}

void TestTomlSourcePlanFeedsConverterAndProfileValidator()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(RootGridLegendCellsRegionsToml());

	iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig config;
	config.hasDefaultFrame = true;
	config.defaultFrame = DefaultFrame();
	config.profileTraits = Catalog({ { Id("profile:guard"), Traits() } });

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult converted =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConverter {}.convert(
			read.plan,
			config);
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult validation =
		iggy::runtime::RuntimeGameplayProfileScenarioValidator {}.validate(
			converted.definition);

	Expect(read.ok(), "acceptance TOML should parse into source plan");
	Expect(read.sourceValidation.ok(), "acceptance TOML should validate as source plan");
	Expect(read.plan.annotatedCells.size() == 1 && read.plan.regions.size() == 1, "reader should only publish source-plan facts");
	Expect(converted.ok(), "parsed TOML source plan should convert with supplied C++ config");
	Expect(validation.ok(), "converted parsed TOML source plan should validate as profile scenario");
	Expect(converted.definition.scenarioId == Id("scenario:guard-room"), "converted scenario should preserve TOML source id");
	Expect(converted.definition.initialState.session.level.map.width == 7, "converted session map should use TOML grid width");
	Expect(converted.definition.initialState.session.level.map.height == 4, "converted session map should use TOML grid height");
	Expect(converted.definition.initialState.npcActors.actors.size() == 1, "converted scenario should promote one TOML actor cell");
	Expect(converted.definition.initialState.npcControls.entries.size() == 1, "converted scenario should create one default control");
	const iggy::NpcActorState2D &actor =
		converted.definition.initialState.npcActors.actors[0];
	Expect(actor.npcId == Id("npc:guard"), "converted actor should preserve TOML marker id");
	Expect(actor.aiProfileId == Id("profile:guard"), "converted actor should preserve TOML profile id");
	Expect(actor.position.x == 1.5F && actor.position.y == 1.5F, "converted actor should preserve TOML local position");
	Expect(converted.definition.frames.size() == 1, "converted scenario should use supplied default frame");
	Expect(converted.definition.frames[0].movementMap.width == 7, "converted frame movement map should use TOML promoted map");
	Expect(read.plan.annotatedCells[0].markerId == Id("npc:guard"), "converter should not mutate parsed TOML source plan");
}

} // namespace

int main()
{
	TestEmptyInputFailsDeterministically();
	TestUnsupportedInputFailsDeterministically();
	TestRootAndGridParse();
	TestSourceLocationsAreCaptured();
	TestCommentsAndInlineRowsParse();
	TestMissingGridFailsAsSyntax();
	TestWrongTypeFailsAsTypeInvalid();
	TestRaggedRowsSurfaceSourcePlanValidation();
	TestInvalidBackgroundGlyphFailsAsTypeInvalid();
	TestNoClaimsPromotionAndLegendParse();
	TestUnsafeNoClaimsSurfaceSourcePlanValidation();
	TestUnsafePromotionSurfaceSourcePlanValidation();
	TestDuplicateLegendGlyphSurfacesSourcePlanValidation();
	TestUnknownLegendEnumFailsAsTypeInvalid();
	TestInvalidLegendGlyphFailsAsTypeInvalid();
	TestCellsAndRegionsParse();
	TestCellGlyphMismatchSurfacesSourcePlanValidation();
	TestCellOutOfBoundsSurfacesSourcePlanValidation();
	TestWrongTypedCellFieldFailsAsTypeInvalid();
	TestUnsupportedInlineCellShapeFailsAsUnsupported();
	TestInvalidRegionBoundsSurfaceSourcePlanValidation();
	TestUnsupportedRegionKeyReportsContext();
	TestFrameControlsParse();
	TestFrameControlInvalidFieldsReportContext();
	TestFrameControlMissingTargetSurfacesSourcePlanValidation();
	TestFrameControlUnsupportedTargetShapeReportsContext();
	TestInteractionTargetsParse();
	TestInteractionTargetInvalidFieldsReportContext();
	TestInteractionTargetMissingIdSurfacesSourcePlanValidation();
	TestInteractionTargetUnsupportedTileShapeReportsContext();
	TestFramePlayerCommandsParse();
	TestFramePlayerCommandInvalidFieldsReportContext();
	TestFramePlayerCommandWrongTypedTargetReportsContext();
	TestFramePlayerCommandMissingTargetSurfacesSourcePlanValidation();
	TestFrameAuthoredDeclarationOrderSpansControlsAndPlayerCommands();
	TestTomlSourcePlanFeedsConverterAndProfileValidator();
	return Failures;
}
