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

std::string RootGridLegendCellsRegionsProfileToml()
{
	return RootGridLegendCellsRegionsToml() + R"toml(

[[profiles]]
id = "plain-profile"
strength = 12
dexterity = 11
constitution = 10
intelligence = 9
wisdom = 8
charisma = 7
)toml";
}

std::string RootGridLegendCellsRegionsPlayerInteractCommandToml()
{
	return RootGridLegendCellsRegionsToml() + R"toml(

[[frame_player_commands]]
frame_id = "frame:interact"
command = "interact"
target_id = "target:door"
)toml";
}

std::string RootGridLegendCellsRegionsPlayerPickupCommandToml()
{
	return RootGridLegendCellsRegionsToml() + R"toml(

[[frame_player_commands]]
frame_id = "frame:pickup"
command = "pickup"
target_id = "target:pickup"
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
required_item_id = "item:key"
enabled_value = true
)toml";
}

std::string RootGridLegendCellsRegionsItemDropToml()
{
	return RootGridLegendCellsRegionsToml() + R"toml(

[[item_drops]]
drop_id = "plain-drop"
item_id = "item:key"
count = 2
tile = { x = 3, y = 1 }
position = { x = 3.5, y = 1.5 }
pickup_radius = 0.75
enabled = false
glyph = "k"
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

void TestSourcePlanVersionPolicy()
{
	const std::string unsupportedVersion = R"toml(
format_id = "iggy:ascii-source-plan"
version = 2

[grid]
width = 1
height = 1
background = "."
rows = ["."]
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult versionRead =
		Read(unsupportedVersion);

	Expect(versionRead.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "unsupported source-plan version should report source-plan invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *versionIssue =
		FindSourceIssue(versionRead, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedVersion);
	Expect(versionIssue != nullptr, "unsupported source-plan version should surface source-plan issue");
	if (versionIssue != nullptr) {
		Expect(versionIssue->table == "root", "unsupported source-plan version should report root table");
		Expect(versionIssue->key == "version", "unsupported source-plan version should report version key");
		Expect(versionIssue->line == LineOfNth(unsupportedVersion, "version = 2"), "unsupported source-plan version should preserve version line");
		Expect(versionIssue->sourceIssue.index == 2 && versionIssue->sourceIssue.firstIndex == 1, "unsupported source-plan version should preserve actual and supported versions");
	}

	const std::string unsupportedFormat = R"toml(
format_id = "iggy:other-source-plan"
version = 1

[grid]
width = 1
height = 1
background = "."
rows = ["."]
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult formatRead =
		Read(unsupportedFormat);

	Expect(formatRead.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "unsupported source-plan format id should report source-plan invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *formatIssue =
		FindSourceIssue(formatRead, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::UnsupportedFormatId);
	Expect(formatIssue != nullptr, "unsupported source-plan format id should surface source-plan issue");
	if (formatIssue != nullptr) {
		Expect(formatIssue->table == "root", "unsupported source-plan format id should report root table");
		Expect(formatIssue->key == "format_id", "unsupported source-plan format id should report format_id key");
		Expect(formatIssue->line == LineOfNth(unsupportedFormat, "format_id"), "unsupported source-plan format id should preserve format line");
		Expect(formatIssue->sourceIssue.id == Id("iggy:other-source-plan"), "unsupported source-plan format id should preserve format id");
	}
}

void TestSourceLocationsAreCaptured()
{
	const std::string text = RootGridLegendCellsRegionsToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "source location fixture should parse");
	Expect(result.sourceLocations.formatIdLine == LineOfNth(text, "format_id"), "format_id line should be captured");
	Expect(result.sourceLocations.versionLine == LineOfNth(text, "version = 1"), "version line should be captured");
	Expect(result.sourceLocations.gridTableLine == LineOfNth(text, "[grid]"), "grid table line should be captured");
	Expect(result.sourceLocations.gridRowsLine == LineOfNth(text, "rows = ["), "grid rows key line should be captured");
	Expect(result.sourceLocations.noClaimsTableLine == LineOfNth(text, "[no_claims]"), "no_claims table line should be captured");
	Expect(result.sourceLocations.noClaimsRuntimeTruthLine == LineOfNth(text, "runtime_truth"), "no_claims runtime_truth line should be captured");
	Expect(result.sourceLocations.noClaimsGameplayExecutionLine == LineOfNth(text, "gameplay_execution"), "no_claims gameplay_execution line should be captured");
	Expect(result.sourceLocations.noClaimsFileParsingLine == LineOfNth(text, "file_parsing"), "no_claims file_parsing line should be captured");
	Expect(result.sourceLocations.noClaimsProfileScenarioConversionLine == LineOfNth(text, "profile_scenario_conversion"), "no_claims profile_scenario_conversion line should be captured");
	Expect(result.sourceLocations.promotionTableLine == LineOfNth(text, "[promotion]"), "promotion table line should be captured");
	Expect(result.sourceLocations.promotionReadyLine == LineOfNth(text, "ready"), "promotion ready line should be captured");
	Expect(result.sourceLocations.promotionRuntimeExecutionLine == LineOfNth(text, "runtime_execution"), "promotion runtime_execution line should be captured");
	Expect(result.sourceLocations.promotionFileParsingLine == LineOfNth(text, "file_parsing", 1), "promotion file_parsing line should be captured");
	Expect(result.sourceLocations.promotionProfileScenarioConversionLine == LineOfNth(text, "profile_scenario_conversion", 1), "promotion profile_scenario_conversion line should be captured");
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
		Expect(issue->table == "no_claims", "mirrored unsafe no-claim should report no_claims table");
		Expect(issue->key == "runtime_truth", "mirrored unsafe no-claim should report unsafe key");
		Expect(issue->line == LineOfNth(text, "runtime_truth = true"), "mirrored unsafe no-claim should report unsafe key line");
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
		Expect(issue->table == "promotion", "mirrored unsafe promotion should report promotion table");
		Expect(issue->key == "runtime_execution", "mirrored unsafe promotion should report unsafe key");
		Expect(issue->line == LineOfNth(text, "runtime_execution = true"), "mirrored unsafe promotion should report unsafe key line");
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

void TestProfilesParse()
{
	const std::string text = RootGridLegendCellsRegionsProfileToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "valid profile TOML should parse");
	Expect(result.plan.authoredProfiles.size() == 1, "one profile should parse");
	Expect(result.sourceLocations.profileTableLines.size() == 1, "profile source location should be captured");
	Expect(result.sourceLocations.profileTableLines[0] == LineOfNth(text, "[[profiles]]"), "profile source location should preserve table line");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredProfile &profile =
		result.plan.authoredProfiles[0];
	Expect(profile.profileId == Id("plain-profile"), "profile should parse exact id");
	Expect(profile.traits.strength == 12, "profile should parse strength");
	Expect(profile.traits.dexterity == 11, "profile should parse dexterity");
	Expect(profile.traits.constitution == 10, "profile should parse constitution");
	Expect(profile.traits.intelligence == 9, "profile should parse intelligence");
	Expect(profile.traits.wisdom == 8, "profile should parse wisdom");
	Expect(profile.traits.charisma == 7, "profile should parse charisma");
}

void TestProfileWrongTypedTraitReportsContext()
{
	std::string text = RootGridLegendCellsRegionsProfileToml();
	const std::size_t start = text.find("strength = 12");
	text.replace(start, std::string("strength = 12").size(), "strength = \"12\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "wrong typed profile trait should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "wrong typed profile trait should report type invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType, "strength");
	Expect(issue != nullptr, "wrong typed profile trait issue should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "profiles", "wrong typed profile trait should report profiles table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "wrong typed profile trait should report first profile index");
		Expect(issue->key == "strength", "wrong typed profile trait should report strength key");
	}
}

void TestProfileMissingIdSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsProfileToml();
	const std::size_t start = text.find("id = \"plain-profile\"\n");
	text.erase(start, std::string("id = \"plain-profile\"\n").size());
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "profile without id should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "profile without id should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileMissingId);
	Expect(issue != nullptr, "profile without id should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[profiles]]"), "mirrored profile id issue should report table line");
		Expect(issue->table == "profiles", "mirrored profile id issue should report profiles table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored profile id issue should report first profile index");
		Expect(issue->key == "id", "mirrored profile id issue should report id key");
	}
}

void TestProfileInvalidTraitSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsProfileToml();
	const std::size_t start = text.find("strength = 12");
	text.replace(start, std::string("strength = 12").size(), "strength = 21");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "out-of-range profile trait should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "out-of-range profile trait should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredProfileInvalidTraits);
	Expect(issue != nullptr, "out-of-range profile trait should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[profiles]]"), "mirrored invalid profile trait should report table line");
		Expect(issue->table == "profiles", "mirrored invalid profile trait should report profiles table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored invalid profile trait should report first profile index");
		Expect(issue->key == "traits", "mirrored invalid profile trait should report traits key");
		Expect(issue->sourceIssue.id == Id("plain-profile"), "mirrored invalid profile trait should preserve profile id");
	}
}

void TestUnsupportedProfileKeyReportsContext()
{
	std::string text = RootGridLegendCellsRegionsProfileToml();
	const std::size_t start = text.find("charisma = 7");
	text.insert(start + std::string("charisma = 7").size(), "\nluck = 10");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "unsupported profile key should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported profile key should report unsupported");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape, "luck");
	Expect(issue != nullptr, "unsupported profile key should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "profiles", "unsupported profile key should report profiles table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unsupported profile key should report first profile index");
		Expect(issue->detail.find("unsupported profiles key") != std::string::npos, "unsupported profile key should preserve detail");
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
	Expect(target.requiredItemId == Id("item:key"), "interaction target should parse required item id");
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

void TestItemDropsParse()
{
	const std::string text = RootGridLegendCellsRegionsItemDropToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "valid item drop TOML should parse");
	Expect(result.plan.authoredItemDrops.size() == 1, "one item drop should parse");
	Expect(result.sourceLocations.itemDropTableLines.size() == 1, "item drop source location should be captured");
	Expect(result.sourceLocations.itemDropTableLines[0] == LineOfNth(text, "[[item_drops]]"), "item drop source location should preserve table line");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &drop =
		result.plan.authoredItemDrops[0];
	Expect(drop.dropId == Id("plain-drop"), "item drop should parse exact drop id");
	Expect(drop.itemId == Id("item:key"), "item drop should parse exact item id");
	Expect(drop.count == 2, "item drop should parse count");
	Expect(drop.localTile.present && drop.localTile.x == 3 && drop.localTile.y == 1, "item drop should parse tile");
	Expect(drop.localPosition.present && drop.localPosition.x == 3.5 && drop.localPosition.y == 1.5, "item drop should parse point position");
	Expect(drop.pickupRadius == 0.75, "item drop should parse pickup radius");
	Expect(!drop.enabled, "item drop should parse enabled flag");
	Expect(drop.glyph == 'k', "item drop should parse optional glyph");
}

void TestItemDropWrongTypedCountReportsContext()
{
	std::string text = RootGridLegendCellsRegionsItemDropToml();
	const std::size_t start = text.find("count = 2");
	text.replace(start, std::string("count = 2").size(), "count = \"2\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "wrong typed item drop count should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "wrong typed item drop count should report type invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType, "count");
	Expect(issue != nullptr, "wrong typed item drop count issue should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "item_drops", "wrong typed item drop count should report item_drops table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "wrong typed item drop count should report first table index");
		Expect(issue->key == "count", "wrong typed item drop count should report count key");
	}
}

void TestItemDropMissingIdSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsItemDropToml();
	const std::size_t start = text.find("drop_id = \"plain-drop\"\n");
	text.erase(start, std::string("drop_id = \"plain-drop\"\n").size());
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "item drop without id should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "item drop without id should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredItemDropMissingDropId);
	Expect(issue != nullptr, "item drop without id should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[item_drops]]"), "mirrored item drop id issue should report table line");
		Expect(issue->table == "item_drops", "mirrored item drop id issue should report item_drops table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored item drop id issue should report first drop index");
		Expect(issue->key == "drop_id", "mirrored item drop id issue should report drop_id key");
	}
}

void TestItemDropUnsupportedTileShapeReportsContext()
{
	std::string text = RootGridLegendCellsRegionsItemDropToml();
	const std::size_t start = text.find("tile = { x = 3, y = 1 }");
	text.replace(start, std::string("tile = { x = 3, y = 1 }").size(), "tile = { x = 3, z = 1 }");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "unsupported item drop tile shape should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported item drop tile shape should report unsupported");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape, "tile");
	Expect(issue != nullptr, "unsupported item drop tile issue should be findable");
	if (issue != nullptr) {
		Expect(issue->table == "item_drops", "unsupported item drop tile should report item_drops table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unsupported item drop tile should report first table index");
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

void TestFramePlayerInteractCommandParses()
{
	const std::string text = RootGridLegendCellsRegionsPlayerInteractCommandToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "valid frame player interact command TOML should parse");
	Expect(result.plan.authoredPlayerCommands.size() == 1, "one frame player interact command should parse");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand
		&command = result.plan.authoredPlayerCommands[0];
	Expect(command.hasFrameId && command.frameId == Id("frame:interact"), "frame player interact command should parse optional frame id");
	Expect(command.command == iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact, "frame player command should parse interact command");
	Expect(command.targetId == Id("target:door"), "frame player interact command should parse exact target id");
	Expect(command.hasDeclarationIndex && command.declarationIndex == 0, "frame player interact command should preserve authored declaration order");
}

void TestFramePlayerPickupCommandParses()
{
	const std::string text = RootGridLegendCellsRegionsPlayerPickupCommandToml();
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(result.ok(), "valid frame player pickup command TOML should parse");
	Expect(result.plan.authoredPlayerCommands.size() == 1, "one frame player pickup command should parse");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand
		&command = result.plan.authoredPlayerCommands[0];
	Expect(command.hasFrameId && command.frameId == Id("frame:pickup"), "frame player pickup command should parse optional frame id");
	Expect(command.command == iggy::runtime::RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup, "frame player command should parse pickup command");
	Expect(command.targetId == Id("target:pickup"), "frame player pickup command should parse exact target id");
	Expect(command.hasDeclarationIndex && command.declarationIndex == 0, "frame player pickup command should preserve authored declaration order");
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

void TestFramePlayerInteractCommandMissingTargetSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsPlayerInteractCommandToml();
	const std::size_t start = text.find("target_id = \"target:door\"\n");
	text.erase(start, std::string("target_id = \"target:door\"\n").size());
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "interact frame player command without target should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "interact frame player command without target should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget);
	Expect(issue != nullptr, "interact frame player command without target should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[frame_player_commands]]"), "mirrored frame player interact target issue should report table line");
		Expect(issue->table == "frame_player_commands", "mirrored frame player interact target issue should report frame_player_commands table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored frame player interact target issue should report first player command index");
		Expect(issue->key == "target", "mirrored frame player interact target issue should report target key");
	}
}

void TestFramePlayerPickupCommandMissingTargetSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsPlayerPickupCommandToml();
	const std::size_t start = text.find("target_id = \"target:pickup\"\n");
	text.erase(start, std::string("target_id = \"target:pickup\"\n").size());
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "pickup frame player command without target should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "pickup frame player command without target should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandMissingTarget);
	Expect(issue != nullptr, "pickup frame player command without target should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[frame_player_commands]]"), "mirrored frame player pickup target issue should report table line");
		Expect(issue->table == "frame_player_commands", "mirrored frame player pickup target issue should report frame_player_commands table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "mirrored frame player pickup target issue should report first player command index");
		Expect(issue->key == "target", "mirrored frame player pickup target issue should report target key");
	}
}

void TestFramePlayerInteractUnknownTargetSurfacesSourcePlanValidation()
{
	const std::string text = RootGridLegendCellsRegionsToml() + R"toml(

[[interaction_targets]]
target_id = "target:door"
kind = "door"
tile = { x = 3, y = 1 }
radius = 1.0
enabled = true

[[frame_player_commands]]
frame_id = "frame:interact"
command = "interact"
target_id = "target:missing"
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "interact command with unknown authored target should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "unknown interact target should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandUnknownInteractionTarget);
	Expect(issue != nullptr, "unknown interact target should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[frame_player_commands]]"), "unknown interact target issue should report player command table line");
		Expect(issue->table == "frame_player_commands", "unknown interact target issue should report frame_player_commands table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unknown interact target issue should report first player command index");
		Expect(issue->key == "target_id", "unknown interact target issue should report target_id key");
		Expect(issue->sourceIssue.id == Id("target:missing"), "unknown interact target issue should preserve target id");
	}
}

void TestFramePlayerPickupInvalidTargetSurfacesSourcePlanValidation()
{
	const std::string text = RootGridLegendCellsRegionsToml() + R"toml(

[[interaction_targets]]
target_id = "target:door"
kind = "door"
tile = { x = 3, y = 1 }
radius = 1.0
enabled = true

[[item_drops]]
drop_id = "drop:key"
item_id = "item:key"
count = 1
tile = { x = 3, y = 1 }
pickup_radius = 1.0
enabled = true

[[frame_player_commands]]
frame_id = "frame:pickup"
command = "pickup"
target_id = "target:door"
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(text);

	Expect(!result.ok(), "pickup command with non-pickup target should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "invalid pickup target should report source invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AuthoredPlayerCommandInvalidPickupTarget);
	Expect(issue != nullptr, "invalid pickup target should mirror source issue");
	if (issue != nullptr) {
		Expect(issue->line == LineOfNth(text, "[[frame_player_commands]]"), "invalid pickup target issue should report player command table line");
		Expect(issue->table == "frame_player_commands", "invalid pickup target issue should report frame_player_commands table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "invalid pickup target issue should report first player command index");
		Expect(issue->key == "target_id", "invalid pickup target issue should report target_id key");
		Expect(issue->sourceIssue.id == Id("target:door"), "invalid pickup target issue should preserve target id");
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

void TestExpectTableParses()
{
	const std::string toml = RootGridToml() + R"toml(

[expect]
final_rows = [
  "#######",
  "#..@..#",
  "#.....#",
  "#######",
]
frame_count = 2
accepted_command_count = 1
picked_up_count = 1
interaction_changed = true
npc_moved_count = 3
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "expect table should parse");
	Expect(read.sourceValidation.ok(), "well-shaped expect table should validate");
	Expect(read.plan.hasExpectations(), "expect table should populate source-plan expectations");
	Expect(read.plan.expectations.hasFinalRows, "expect table should mark final rows present");
	Expect(read.plan.expectations.finalRows[1] == "#..@..#", "expect table should preserve expected row text");
	Expect(read.plan.expectations.hasFrameCount && read.plan.expectations.frameCount == 2, "expect table should preserve frame count");
	Expect(read.plan.expectations.hasAcceptedCommandCount && read.plan.expectations.acceptedCommandCount == 1, "expect table should preserve accepted command count");
	Expect(read.plan.expectations.hasPickedUpCount && read.plan.expectations.pickedUpCount == 1, "expect table should preserve picked up count");
	Expect(read.plan.expectations.hasInteractionChanged && read.plan.expectations.interactionChanged, "expect table should preserve interaction flag");
	Expect(read.plan.expectations.hasNpcMovedCount && read.plan.expectations.npcMovedCount == 3, "expect table should preserve NPC moved count");
}

void TestExpectInlineRowsParse()
{
	const std::string toml = RootGridToml() + R"toml(

[expect]
final_rows = ["#######", "#..@..#", "#.....#", "#######"]
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "inline expect final rows should parse");
	Expect(read.plan.expectations.hasFinalRows, "inline expect final rows should mark rows present");
	Expect(read.plan.expectations.finalRows.size() == 4, "inline expect final rows should preserve row count");
	Expect(read.plan.expectations.finalRows[1] == "#..@..#", "inline expect final rows should preserve row text");
}

void TestExpectTraceFramesParse()
{
	const std::string toml = RootGridToml() + R"toml(

[[expect_trace_frames]]
frame_id = "frame:one"
accepted_command_count = 1
picked_up_count = 0
interaction_changed = false
npc_moved_count = 2
rows = [
  "#######",
  "#..@..#",
  "#.....#",
  "#######",
]
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "expect trace frame table should parse");
	Expect(read.sourceValidation.ok(), "well-shaped expect trace frame should validate");
	Expect(read.plan.hasExpectations(), "expect trace frame table should populate expectations");
	Expect(read.plan.expectations.traceFrames.size() == 1, "expect trace frame table should preserve frame count");
	if (!read.plan.expectations.traceFrames.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectedTraceFrame &frame =
			read.plan.expectations.traceFrames[0];
		Expect(frame.hasFrameId && frame.frameId == Id("frame:one"), "expect trace frame should preserve frame id");
		Expect(frame.hasRows && frame.rows[1] == "#..@..#", "expect trace frame should preserve rows");
		Expect(frame.hasAcceptedCommandCount && frame.acceptedCommandCount == 1, "expect trace frame should preserve accepted command count");
		Expect(frame.hasPickedUpCount && frame.pickedUpCount == 0, "expect trace frame should preserve picked up count");
		Expect(frame.hasInteractionChanged && !frame.interactionChanged, "expect trace frame should preserve interaction flag");
		Expect(frame.hasNpcMovedCount && frame.npcMovedCount == 2, "expect trace frame should preserve NPC moved count");
	}
}

void TestExpectInventoryStacksParse()
{
	const std::string toml = RootGridToml() + R"toml(

[[expect_inventory_stacks]]
item_id = "item:key"
count = 1
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "expect inventory stack table should parse");
	Expect(read.sourceValidation.ok(), "well-shaped expect inventory stack should validate");
	Expect(read.plan.hasExpectations(), "expect inventory stack table should populate expectations");
	Expect(read.plan.expectations.inventoryStacks.size() == 1, "expect inventory stack table should preserve stack count");
	if (!read.plan.expectations.inventoryStacks.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectedInventoryStack &stack =
			read.plan.expectations.inventoryStacks[0];
		Expect(stack.itemId == Id("item:key"), "expect inventory stack should preserve item id");
		Expect(stack.count == 1, "expect inventory stack should preserve count");
	}
}

void TestExpectInteractionTargetsParse()
{
	const std::string toml = RootGridToml() + R"toml(

[[interaction_targets]]
target_id = "target:door"
kind = "usable"
tile = { x = 3, y = 1 }
radius = 1.0
enabled = true
effect = "toggle_target"
effect_target_id = "target:door"
enabled_value = false

[[expect_interaction_targets]]
target_id = "target:door"
enabled = false
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "expect interaction target table should parse");
	Expect(read.sourceValidation.ok(), "well-shaped expect interaction target should validate");
	Expect(read.plan.hasExpectations(), "expect interaction target table should populate expectations");
	Expect(read.plan.expectations.interactionTargets.size() == 1, "expect interaction target table should preserve target count");
	if (!read.plan.expectations.interactionTargets.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectedInteractionTarget &target =
			read.plan.expectations.interactionTargets[0];
		Expect(target.targetId == Id("target:door"), "expect interaction target should preserve target id");
		Expect(!target.enabled, "expect interaction target should preserve enabled state");
	}
}

void TestExpectActorAndPlayerStatesParse()
{
	const std::string toml = RootGridLegendCellsRegionsToml() + R"toml(

[[expect_actor_states]]
actor_id = "npc:guard"
tile = { x = 3, y = 1 }

[expect_player_state]
tile = { x = 4, y = 1 }
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "expect actor and player state tables should parse");
	Expect(read.sourceValidation.ok(), "well-shaped actor and player state expectations should validate");
	Expect(read.plan.hasExpectations(), "actor and player state tables should populate expectations");
	Expect(read.plan.expectations.actorStates.size() == 1, "expect actor state table should preserve actor count");
	if (!read.plan.expectations.actorStates.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectedActorState &actor =
			read.plan.expectations.actorStates[0];
		Expect(actor.actorId == Id("npc:guard"), "expect actor state should preserve actor id");
		Expect(actor.hasTile && actor.tile == iggy::TileCoord { 3, 1 }, "expect actor state should preserve tile");
	}
	Expect(read.plan.expectations.hasPlayerState, "expect player state table should mark player state present");
	Expect(read.plan.expectations.playerState.hasTile && read.plan.expectations.playerState.tile == iggy::TileCoord { 4, 1 }, "expect player state should preserve tile");
}

void TestExpectWrongTypedFieldReportsContext()
{
	const std::string toml = RootGridToml() + R"toml(

[expect]
frame_count = "two"
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "wrong typed expect field should report type invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindIssue(read, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType, "frame_count");
	Expect(issue != nullptr, "wrong typed expect field should report wrong type");
	if (issue != nullptr) {
		Expect(issue->table == "expect", "wrong typed expect field should report expect table");
		Expect(issue->line == LineOfNth(toml, "frame_count"), "wrong typed expect field should preserve source line");
	}
}

void TestExpectFinalRowsShapeSurfacesSourcePlanValidation()
{
	const std::string toml = RootGridToml() + R"toml(

[expect]
final_rows = [
  "#######",
  "#@#",
  "#.....#",
  "#######",
]
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "malformed expect final rows should report source-plan invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(read, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedFinalRowsDimensionMismatch);
	Expect(issue != nullptr, "malformed expect final rows should surface source-plan issue");
	if (issue != nullptr) {
		Expect(issue->table == "expect", "malformed expect final rows should report expect table");
		Expect(issue->key == "final_rows", "malformed expect final rows should report final_rows key");
		Expect(issue->line == LineOfNth(toml, "final_rows"), "malformed expect final rows should preserve source line");
	}
}

void TestExpectTraceFrameRowsShapeSurfacesSourcePlanValidation()
{
	const std::string toml = RootGridToml() + R"toml(

[[expect_trace_frames]]
frame_id = "frame:bad"
rows = [
  "#######",
  "#@#",
  "#.....#",
  "#######",
]
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "malformed expect trace rows should report source-plan invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(read, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedTraceFrameRowsDimensionMismatch);
	Expect(issue != nullptr, "malformed expect trace rows should surface source-plan issue");
	if (issue != nullptr) {
		Expect(issue->table == "expect_trace_frames", "malformed expect trace rows should report trace expectation table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "malformed expect trace rows should report table index");
		Expect(issue->key == "rows", "malformed expect trace rows should report rows key");
		Expect(issue->line == LineOfNth(toml, "rows = [", 1), "malformed expect trace rows should preserve source line");
	}
}

void TestExpectInventoryStackShapeSurfacesSourcePlanValidation()
{
	const std::string toml = RootGridToml() + R"toml(

[[expect_inventory_stacks]]
item_id = "item:key"
count = 0
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "invalid expect inventory stack should report source-plan invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(read, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedInventoryStackInvalidCount);
	Expect(issue != nullptr, "invalid expect inventory stack should surface source-plan issue");
	if (issue != nullptr) {
		Expect(issue->table == "expect_inventory_stacks", "invalid expect inventory stack should report inventory expectation table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "invalid expect inventory stack should report table index");
		Expect(issue->key == "count", "invalid expect inventory stack should report count key");
		Expect(issue->line == LineOfNth(toml, "[[expect_inventory_stacks]]"), "invalid expect inventory stack should preserve source line");
	}
}

void TestExpectInteractionTargetUnknownSurfacesSourcePlanValidation()
{
	const std::string toml = RootGridToml() + R"toml(

[[interaction_targets]]
target_id = "target:door"
kind = "usable"
tile = { x = 3, y = 1 }
radius = 1.0
enabled = true
effect = "toggle_target"
effect_target_id = "target:door"
enabled_value = false

[[expect_interaction_targets]]
target_id = "target:missing"
enabled = false
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "unknown expect interaction target should report source-plan invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(read, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedInteractionTargetUnknownId);
	Expect(issue != nullptr, "unknown expect interaction target should surface source-plan issue");
	if (issue != nullptr) {
		Expect(issue->table == "expect_interaction_targets", "unknown expect interaction target should report interaction expectation table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unknown expect interaction target should report table index");
		Expect(issue->key == "target_id", "unknown expect interaction target should report target_id key");
		Expect(issue->line == LineOfNth(toml, "[[expect_interaction_targets]]"), "unknown expect interaction target should preserve source line");
	}
}

void TestExpectActorStateUnknownSurfacesSourcePlanValidation()
{
	const std::string toml = RootGridLegendCellsRegionsToml() + R"toml(

[[expect_actor_states]]
actor_id = "npc:missing"
tile = { x = 3, y = 1 }
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "unknown expect actor state should report source-plan invalid");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *issue =
		FindSourceIssue(read, iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::ExpectedActorStateUnknownId);
	Expect(issue != nullptr, "unknown expect actor state should surface source-plan issue");
	if (issue != nullptr) {
		Expect(issue->table == "expect_actor_states", "unknown expect actor state should report actor state expectation table");
		Expect(issue->hasTableIndex && issue->tableIndex == 0, "unknown expect actor state should report table index");
		Expect(issue->key == "actor_id", "unknown expect actor state should report actor_id key");
		Expect(issue->line == LineOfNth(toml, "[[expect_actor_states]]"), "unknown expect actor state should preserve source line");
	}
}

void TestSubsetWhitespaceCommentsAndEscapedStringsParse()
{
	const std::string toml = R"toml(
  # leading whitespace and comments are ignored
	format_id = "iggy:ascii-source-plan" # trailing comments are ignored
version = 1
source_ref = "authoring:manual\tfixture"

  [grid]
width = 3
height = 2
background = "."
rows = [ "###", "#.#" ] # compact inline arrays are accepted

[[interaction_targets]]
target_id = "target:sign"
kind = "inspectable"
tile = { x = 1, y = 1 }
radius = 1.0
enabled = true
effect = "inspect_text"
text = "read # literally, then \"quoted\" text"
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "subset stress whitespace/comments/escaped strings should parse");
	Expect(read.plan.hasSourceRef && read.plan.sourceRef == Id("authoring:manual\tfixture"), "escaped tab in quoted string should be preserved");
	Expect(read.plan.grid.rows.size() == 2 && read.plan.grid.rows[1] == "#.#", "inline rows with spaces should parse");
	Expect(read.plan.authoredInteractionTargets.size() == 1, "interaction target should parse after comments");
	if (!read.plan.authoredInteractionTargets.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &target =
			read.plan.authoredInteractionTargets[0];
		Expect(target.text == "read # literally, then \"quoted\" text", "comments inside strings and escaped quotes should be preserved");
	}
}

void TestSubsetTableOrderingAndInterleavedFrameDeclarationsParse()
{
	const std::string toml = R"toml(
format_id = "iggy:ascii-source-plan"
version = 1
source_id = "scenario:ordered"

[[profiles]]
id = "profile:guard"
strength = 10
dexterity = 10
constitution = 10
intelligence = 10
wisdom = 10
charisma = 10

[[legend]]
glyph = "A"
kind = "actor"
role_id = "role:npc"
maps_to_scenario_marker = true
scenario_marker_kind = "actor"

[[legend]]
glyph = "@"
kind = "player_start"
role_id = "role:player-start"
maps_to_scenario_marker = true
scenario_marker_kind = "player_start"

[[frame_player_commands]]
frame_id = "frame:first"
command = "move_to_tile"
x = 3
y = 1

[[cells]]
id = "cell:guard"
row = 1
column = 1
glyph = "A"
local_tile = { x = 1, y = 1 }
marker_id = "npc:guard"
profile_id = "profile:guard"

[[frame_controls]]
frame_id = "frame:second"
npc = "npc:guard"
behavior = "seeking"
move_mode = "walk"
target = { x = 2.5, y = 1.5 }

[grid]
width = 5
height = 3
background = "."
rows = [
  "#####",
  "#A.@#",
  "#####",
]
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "subset stress reordered supported tables should parse");
	Expect(read.plan.grid.width == 5 && read.plan.grid.height == 3, "grid should parse even when it appears after repeated tables");
	Expect(read.plan.authoredProfiles.size() == 1, "profile table should parse before grid");
	Expect(read.plan.authoredPlayerCommands.size() == 1, "player command should parse before actor cell");
	Expect(read.plan.authoredControls.size() == 1, "NPC control should parse after actor cell");
	Expect(read.plan.authoredPlayerCommands[0].declarationIndex == 0, "first interleaved authored frame declaration should keep order");
	Expect(read.plan.authoredControls[0].declarationIndex == 1, "second interleaved authored frame declaration should keep order");
}

void TestSubsetRepeatedTablesAndInlineTableFieldOrderParse()
{
	const std::string toml = RootGridLegendCellsRegionsToml() + R"toml(

[[item_drops]]
drop_id = "drop:first"
item_id = "item:key"
count = 1
tile = { y = 1, x = 3 }
pickup_radius = 1.0
enabled = true
glyph = "k"

[[item_drops]]
drop_id = "drop:second"
item_id = "item:coin"
count = 2
position = { y = 1.5, x = 4.5 }
pickup_radius = 1.0
enabled = false
glyph = "c"

[[expect_trace_frames]]
frame_id = "frame:first"
rows = ["#######", "#A...@#", "#.....#", "#######"]

[[expect_trace_frames]]
frame_id = "frame:second"
rows = ["#######", "#.A..@#", "#.....#", "#######"]
)toml";

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult read =
		Read(toml);

	Expect(read.ok(), "subset stress repeated tables and reordered inline fields should parse");
	Expect(read.plan.authoredItemDrops.size() == 2, "repeated item_drops tables should append");
	if (read.plan.authoredItemDrops.size() == 2) {
		Expect(read.plan.authoredItemDrops[0].localTile.present &&
			read.plan.authoredItemDrops[0].localTile.x == 3 &&
			read.plan.authoredItemDrops[0].localTile.y == 1,
			"inline tile fields should parse by key regardless of order");
		Expect(read.plan.authoredItemDrops[1].localPosition.present &&
			read.plan.authoredItemDrops[1].localPosition.x == 4.5 &&
			read.plan.authoredItemDrops[1].localPosition.y == 1.5,
			"inline position fields should parse by key regardless of order");
	}
	Expect(read.plan.expectations.traceFrames.size() == 2, "repeated expectation trace frames should append");
}

void TestSubsetUnsupportedTomlFeaturesReportDiagnostics()
{
	const std::string singleQuoted = R"toml(
format_id = 'iggy:ascii-source-plan'
version = 1

[grid]
width = 1
height = 1
background = "."
rows = ["."]
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult singleRead =
		Read(singleQuoted);
	Expect(singleRead.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "single-quoted strings should remain unsupported as wrong type");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *formatIssue =
		FindIssue(singleRead, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType, "format_id");
	Expect(formatIssue != nullptr, "single-quoted strings should report the affected key");
	if (formatIssue != nullptr) {
		Expect(formatIssue->table == "root", "single-quoted root string should report root table");
	}

	const std::string dottedTable = R"toml(
format_id = "iggy:ascii-source-plan"
version = 1

[grid.rows]
value = ["."]
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult dottedRead =
		Read(dottedTable);
	Expect(dottedRead.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "dotted tables should report unsupported syntax");
	Expect(HasIssue(dottedRead, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape), "dotted tables should report unsupported issue");

	const std::string inlineArrayOfTables = R"toml(
format_id = "iggy:ascii-source-plan"
version = 1

[grid]
width = 1
height = 1
background = "."
rows = ["."]

[[cells]]
id = "cell:bad"
row = 0
column = 0
glyph = "."
role_tags = [{ id = "tag:unsupported" }]
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult arrayRead =
		Read(inlineArrayOfTables);
	Expect(arrayRead.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "arrays of inline tables should remain unsupported as wrong type");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue *roleTagsIssue =
		FindIssue(arrayRead, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType, "role_tags");
	Expect(roleTagsIssue != nullptr, "unsupported array shape should report the affected key");
	if (roleTagsIssue != nullptr) {
		Expect(roleTagsIssue->table == "cells", "unsupported array shape should report repeated table context");
		Expect(roleTagsIssue->hasTableIndex && roleTagsIssue->tableIndex == 0, "unsupported array shape should report repeated table index");
	}
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
	TestSourcePlanVersionPolicy();
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
	TestProfilesParse();
	TestProfileWrongTypedTraitReportsContext();
	TestProfileMissingIdSurfacesSourcePlanValidation();
	TestProfileInvalidTraitSurfacesSourcePlanValidation();
	TestUnsupportedProfileKeyReportsContext();
	TestInteractionTargetsParse();
	TestInteractionTargetInvalidFieldsReportContext();
	TestInteractionTargetMissingIdSurfacesSourcePlanValidation();
	TestInteractionTargetUnsupportedTileShapeReportsContext();
	TestItemDropsParse();
	TestItemDropWrongTypedCountReportsContext();
	TestItemDropMissingIdSurfacesSourcePlanValidation();
	TestItemDropUnsupportedTileShapeReportsContext();
	TestFramePlayerCommandsParse();
	TestFramePlayerInteractCommandParses();
	TestFramePlayerPickupCommandParses();
	TestFramePlayerCommandInvalidFieldsReportContext();
	TestFramePlayerCommandWrongTypedTargetReportsContext();
	TestFramePlayerCommandMissingTargetSurfacesSourcePlanValidation();
	TestFramePlayerInteractCommandMissingTargetSurfacesSourcePlanValidation();
	TestFramePlayerPickupCommandMissingTargetSurfacesSourcePlanValidation();
	TestFramePlayerInteractUnknownTargetSurfacesSourcePlanValidation();
	TestFramePlayerPickupInvalidTargetSurfacesSourcePlanValidation();
	TestFrameAuthoredDeclarationOrderSpansControlsAndPlayerCommands();
	TestExpectTableParses();
	TestExpectInlineRowsParse();
	TestExpectTraceFramesParse();
	TestExpectInventoryStacksParse();
	TestExpectInteractionTargetsParse();
	TestExpectActorAndPlayerStatesParse();
	TestExpectWrongTypedFieldReportsContext();
	TestExpectFinalRowsShapeSurfacesSourcePlanValidation();
	TestExpectTraceFrameRowsShapeSurfacesSourcePlanValidation();
	TestExpectInventoryStackShapeSurfacesSourcePlanValidation();
	TestExpectInteractionTargetUnknownSurfacesSourcePlanValidation();
	TestExpectActorStateUnknownSurfacesSourcePlanValidation();
	TestSubsetWhitespaceCommentsAndEscapedStringsParse();
	TestSubsetTableOrderingAndInterleavedFrameDeclarationsParse();
	TestSubsetRepeatedTablesAndInlineTableFieldOrderParse();
	TestSubsetUnsupportedTomlFeaturesReportDiagnostics();
	TestTomlSourcePlanFeedsConverterAndProfileValidator();
	return Failures;
}
