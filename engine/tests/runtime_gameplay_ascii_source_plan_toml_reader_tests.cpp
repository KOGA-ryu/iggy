#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

#include <iostream>
#include <string>

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

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult Read(
	const std::string &text)
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReader {}.read(text);
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
	Expect(result.sourcePlanIssueCount > 0, "ragged parsed rows should mirror nested source validation issue count");
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
	Expect(result.sourcePlanIssueCount > 0, "unsafe no-claim should mirror nested source validation issues");
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
	const std::size_t start = text.find("glyph = \"A\"");
	text.replace(start, std::string("glyph = \"A\"").size(), "glyph = \"AA\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "invalid legend glyph should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "invalid legend glyph should report type invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString), "invalid legend glyph should report invalid glyph");
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
	Expect(result.sourcePlanIssueCount > 0, "cell glyph mismatch should preserve nested source issues");
}

void TestCellOutOfBoundsSurfacesSourcePlanValidation()
{
	std::string text = RootGridLegendCellsRegionsToml();
	const std::size_t start = text.find("column = 1");
	text.replace(start, std::string("column = 1").size(), "column = 8");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "out-of-bounds cell should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "out-of-bounds cell should report source invalid");
	Expect(result.sourcePlanIssueCount > 0, "out-of-bounds cell should preserve nested source issues");
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
	Expect(result.sourcePlanIssueCount > 0, "out-of-bounds region should preserve nested source issues");
}

} // namespace

int main()
{
	TestEmptyInputFailsDeterministically();
	TestUnsupportedInputFailsDeterministically();
	TestRootAndGridParse();
	TestCommentsAndInlineRowsParse();
	TestMissingGridFailsAsSyntax();
	TestWrongTypeFailsAsTypeInvalid();
	TestRaggedRowsSurfaceSourcePlanValidation();
	TestInvalidBackgroundGlyphFailsAsTypeInvalid();
	TestNoClaimsPromotionAndLegendParse();
	TestUnsafeNoClaimsSurfaceSourcePlanValidation();
	TestDuplicateLegendGlyphSurfacesSourcePlanValidation();
	TestUnknownLegendEnumFailsAsTypeInvalid();
	TestInvalidLegendGlyphFailsAsTypeInvalid();
	TestCellsAndRegionsParse();
	TestCellGlyphMismatchSurfacesSourcePlanValidation();
	TestCellOutOfBoundsSurfacesSourcePlanValidation();
	TestWrongTypedCellFieldFailsAsTypeInvalid();
	TestUnsupportedInlineCellShapeFailsAsUnsupported();
	TestInvalidRegionBoundsSurfaceSourcePlanValidation();
	return Failures;
}
