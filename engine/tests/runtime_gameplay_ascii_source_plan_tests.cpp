#include <cstdlib>

#include "runtime/RuntimeGameplayAsciiSourcePlan.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

void TestDefaultSourcePlanIsEmptyAndSafe()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;

	Expect(plan.formatId.empty(), "default source plan should not have format id");
	Expect(plan.version == 1, "default source plan should use version 1");
	Expect(!plan.hasSourceId, "default source plan should not have source id");
	Expect(!plan.hasSourceRef, "default source plan should not have source ref");
	Expect(!plan.hasRows(), "default source plan should not have rows");
	Expect(plan.rowCount() == 0, "default source plan should report zero rows");
	Expect(plan.legendCount() == 0, "default source plan should report zero legend entries");
	Expect(plan.annotatedCellCount() == 0, "default source plan should report zero annotated cells");
	Expect(plan.regionCount() == 0, "default source plan should report zero regions");
	Expect(plan.safeForAuthoring(), "default source plan should be safe authoring data");
}

void TestSourcePlanPreservesGridAndSourceFacts()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.formatId = Id("iggy:ascii-source-plan");
	plan.version = 2;
	plan.hasSourceId = true;
	plan.sourceId = Id("source:yard");
	plan.hasSourceRef = true;
	plan.sourceRef = Id("ref:sketch-01");
	plan.grid.width = 5;
	plan.grid.height = 3;
	plan.grid.rows = {
		"#####",
		"#A..#",
		"#####",
	};
	plan.grid.backgroundGlyph = '.';

	Expect(plan.formatId == Id("iggy:ascii-source-plan"), "source plan should preserve format id");
	Expect(plan.version == 2, "source plan should preserve version");
	Expect(plan.hasSourceId && plan.sourceId == Id("source:yard"), "source plan should preserve source id");
	Expect(plan.hasSourceRef && plan.sourceRef == Id("ref:sketch-01"), "source plan should preserve source ref");
	Expect(plan.hasRows(), "source plan should report rows");
	Expect(plan.rowCount() == 3, "source plan should preserve row count");
	Expect(plan.grid.width == 5 && plan.grid.height == 3, "source plan should preserve declared grid dimensions");
	Expect(plan.grid.rows[1] == "#A..#", "source plan should preserve row text exactly");
	Expect(plan.grid.backgroundGlyph == '.', "source plan should preserve background glyph");
}

void TestLegendPreservesRolesAndScenarioMappings()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.legend = {
		{
			'A',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor,
			Id("role:npc"),
			{ Id("tag:hostile"), Id("tag:guard") },
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			Id("npc:guard"),
			Id("profile:guard"),
		},
		{
			'.',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Terrain,
			Id("role:floor"),
			{},
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Floor,
			{},
			{},
		},
	};

	Expect(plan.legendCount() == 2, "source plan should preserve legend count");
	Expect(plan.legend[0].glyph == 'A', "legend should preserve glyph");
	Expect(plan.legend[0].kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor, "legend should preserve glyph kind");
	Expect(plan.legend[0].roleId == Id("role:npc"), "legend should preserve role id");
	Expect(plan.legend[0].roleTags.size() == 2, "legend should preserve role tags");
	Expect(plan.legend[0].roleTags[0] == Id("tag:hostile"), "legend should preserve exact namespaced role tag");
	Expect(plan.legend[0].roleTags[1] == Id("tag:guard"), "legend should preserve second role tag");
	Expect(plan.legend[0].mapsToScenarioMarker, "legend should preserve scenario mapping flag");
	Expect(plan.legend[0].scenarioMarkerKind == iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor, "legend should preserve scenario marker kind");
	Expect(plan.legend[0].targetMarkerId == Id("npc:guard"), "legend should preserve target marker id");
	Expect(plan.legend[0].targetProfileId == Id("profile:guard"), "legend should preserve target profile id");
}

void TestAnnotatedCellsPreserveLocalFactsAndExactIds()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAnnotatedCell cell;
	cell.hasCellId = true;
	cell.cellId = Id("cell:npc-a");
	cell.row = 1;
	cell.column = 2;
	cell.glyph = 'A';
	cell.localTile = { true, 7, 9 };
	cell.localPosition = { true, 7.5, 9.25 };
	cell.cellBounds = { true, 7.0, 9.0, 8.0, 10.0 };
	cell.roleTags = { Id("actor"), Id("profile:guard") };
	cell.markerId = Id("npc:guard");
	cell.profileId = Id("profile:guard");
	plan.annotatedCells.push_back(cell);

	Expect(plan.annotatedCellCount() == 1, "source plan should preserve annotated cell count");
	Expect(plan.annotatedCells[0].hasCellId && plan.annotatedCells[0].cellId == Id("cell:npc-a"), "annotated cell should preserve cell id");
	Expect(plan.annotatedCells[0].row == 1 && plan.annotatedCells[0].column == 2, "annotated cell should preserve row and column");
	Expect(plan.annotatedCells[0].glyph == 'A', "annotated cell should preserve glyph");
	Expect(plan.annotatedCells[0].localTile.present && plan.annotatedCells[0].localTile.x == 7 && plan.annotatedCells[0].localTile.y == 9, "annotated cell should preserve local tile");
	Expect(plan.annotatedCells[0].localPosition.present && plan.annotatedCells[0].localPosition.x == 7.5 && plan.annotatedCells[0].localPosition.y == 9.25, "annotated cell should preserve local position");
	Expect(plan.annotatedCells[0].cellBounds.present && plan.annotatedCells[0].cellBounds.maxX == 8.0, "annotated cell should preserve cell bounds");
	Expect(plan.annotatedCells[0].roleTags[1] == Id("profile:guard"), "annotated cell should preserve exact role tag ids");
	Expect(plan.annotatedCells[0].markerId == Id("npc:guard"), "annotated cell should preserve marker id");
	Expect(plan.annotatedCells[0].profileId == Id("profile:guard"), "annotated cell should preserve profile id");
}

void TestRegionsAndNoClaimFlagsAreExplicit()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.regions = {
		{
			true,
			Id("region:room-a"),
			1,
			2,
			4,
			7,
			{ Id("tag:room"), Id("tag:encounter") },
		},
	};

	Expect(plan.regionCount() == 1, "source plan should preserve region count");
	Expect(plan.regions[0].hasRegionId && plan.regions[0].regionId == Id("region:room-a"), "region should preserve id");
	Expect(plan.regions[0].minRow == 1 && plan.regions[0].maxRow == 4, "region should preserve row bounds");
	Expect(plan.regions[0].minColumn == 2 && plan.regions[0].maxColumn == 7, "region should preserve column bounds");
	Expect(plan.regions[0].roleTags[0] == Id("tag:room"), "region should preserve role tags");
	Expect(plan.safeForAuthoring(), "safe no-claim defaults should report safe authoring data");

	plan.noClaims.claimsRuntimeTruth = true;
	Expect(!plan.safeForAuthoring(), "runtime truth claim should make source plan unsafe");
	plan.noClaims.claimsRuntimeTruth = false;
	plan.promotionPolicy.allowsProfileScenarioConversion = true;
	Expect(!plan.safeForAuthoring(), "profile scenario conversion permission should make source plan unsafe for this boundary");
}

void TestSourcePlanCopiesAreIndependent()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.grid.rows = { "A." };
	plan.legend = {
		{
			'A',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor,
			Id("role:npc"),
			{ Id("tag:original") },
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			Id("npc:copy"),
			Id("profile:copy"),
		},
	};
	plan.annotatedCells = {
		{ true, Id("cell:copy"), 0, 0, 'A', {}, {}, {}, { Id("tag:cell") }, Id("npc:copy"), Id("profile:copy") },
	};

	iggy::runtime::RuntimeGameplayAsciiSourcePlan copy = plan;
	copy.grid.rows[0] = "..";
	copy.legend[0].targetMarkerId = Id("npc:changed");
	copy.annotatedCells[0].profileId = Id("profile:changed");

	Expect(plan.grid.rows[0] == "A.", "source plan copy should not mutate source rows");
	Expect(plan.legend[0].targetMarkerId == Id("npc:copy"), "source plan copy should not mutate legend target id");
	Expect(plan.annotatedCells[0].profileId == Id("profile:copy"), "source plan copy should not mutate annotated cell profile id");
}

} // namespace

int main()
{
	TestDefaultSourcePlanIsEmptyAndSafe();
	TestSourcePlanPreservesGridAndSourceFacts();
	TestLegendPreservesRolesAndScenarioMappings();
	TestAnnotatedCellsPreserveLocalFactsAndExactIds();
	TestRegionsAndNoClaimFlagsAreExplicit();
	TestSourcePlanCopiesAreIndependent();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
