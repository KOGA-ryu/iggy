#include <cstdlib>
#include <vector>

#include "scene/draft/DraftBuildingCompile2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::DraftSymbol2D Symbol(
	const char *id,
	iggy::DraftSymbol2DKind kind,
	iggy::Vec2 position = { 0.0F, 0.0F },
	iggy::Vec2 size = { 1.0F, 1.0F },
	float rotationRadians = 0.0F,
	const char *assetId = "asset:symbol",
	const char *definitionId = "definition:symbol",
	bool enabled = true)
{
	return {
		Id(id),
		kind,
		position,
		size,
		rotationRadians,
		Id(assetId),
		Id(definitionId),
		enabled,
	};
}

iggy::DraftDocument2D Document(std::vector<iggy::DraftSymbol2D> symbols)
{
	return { symbols };
}

bool SameSymbols(const std::vector<iggy::DraftSymbol2D> &actual, const std::vector<iggy::DraftSymbol2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].id != expected[index].id
			|| actual[index].kind != expected[index].kind
			|| !NearVec(actual[index].position, expected[index].position)
			|| !NearVec(actual[index].size, expected[index].size)
			|| actual[index].rotationRadians != expected[index].rotationRadians
			|| actual[index].assetId != expected[index].assetId
			|| actual[index].definitionId != expected[index].definitionId
			|| actual[index].enabled != expected[index].enabled) {
			return false;
		}
	}
	return true;
}

void TestEmptyDocumentCompilesThroughEmptyStages()
{
	const iggy::DraftBuildingCompile2DResult result = iggy::DraftBuildingCompiler2D {}.compile({});

	Expect(result.plan.walls.empty() && result.plan.doors.empty(), "empty building compile should have empty compile plan");
	Expect(result.walls.walls.empty(), "empty building compile should have no compiled walls");
	Expect(result.doors.doors.empty(), "empty building compile should have no compiled doors");
	Expect(result.attachments.attachments.empty(), "empty building compile should have no door attachments");
	Expect(result.morphPlan.wallPlans.empty(), "empty building compile should have no morph wall plans");
	Expect(result.wallCuts.segments.empty(), "empty building compile should have no wall cut segments");
	Expect(result.compiledWallSegmentCount == 0, "empty building compile should report zero segments");
	Expect(result.compiledDoorCount == 0, "empty building compile should report zero doors");
	Expect(!result.hasIssues(), "empty building compile should report no issues");
}

void TestOneWallNoDoorsProducesOneFinalWallSegment()
{
	const iggy::DraftSymbol2D wall =
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 8.0F, 2.0F }, 0.0F, "asset:wall", "definition:wall");

	const iggy::DraftBuildingCompile2DResult result =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ wall }));

	Expect(result.plan.walls.size() == 1, "single wall building compile should plan one wall");
	Expect(result.walls.walls.size() == 1, "single wall building compile should compile one wall");
	Expect(result.doors.doors.empty(), "single wall building compile should compile no doors");
	Expect(result.wallCuts.segments.size() == 1, "single wall building compile should output one wall segment");
	Expect(result.compiledWallSegmentCount == 1, "single wall building compile should count one segment");
	Expect(result.compiledDoorCount == 0, "single wall building compile should count zero doors");
	Expect(!result.hasIssues(), "single valid wall building compile should report no issues");
	if (result.wallCuts.segments.size() == 1) {
		Expect(result.wallCuts.segments[0].sourceWallId == wall.id, "single wall segment should preserve wall id");
		Expect(NearVec(result.wallCuts.segments[0].center, wall.position), "single wall segment should preserve wall position");
		Expect(NearVec(result.wallCuts.segments[0].size, wall.size), "single wall segment should preserve wall size");
		Expect(!result.wallCuts.segments[0].hasDoorCut, "single wall segment should report no door cut");
	}
}

void TestWallAndAttachedDoorProduceCutSegmentsAndDoorFacts()
{
	const iggy::DraftSymbol2D wall =
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 10.0F, 2.0F }, 0.0F, "asset:wall", "definition:wall");
	const iggy::DraftSymbol2D door =
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { 2.0F, 2.0F }, 0.0F, "asset:door", "definition:door");

	const iggy::DraftBuildingCompile2DResult result =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ wall, door }));

	Expect(result.walls.walls.size() == 1, "wall door building compile should compile one wall");
	Expect(result.doors.doors.size() == 1, "wall door building compile should compile one door");
	Expect(result.attachments.attachments.size() == 1, "wall door building compile should attach door");
	Expect(result.morphPlan.wallPlans.size() == 1 && result.morphPlan.wallPlans[0].doors.size() == 1, "wall door building compile should group door under wall");
	Expect(result.wallCuts.segments.size() == 2, "wall door building compile should output cut wall segments");
	Expect(result.compiledWallSegmentCount == 2, "wall door building compile should count cut wall segments");
	Expect(result.compiledDoorCount == 1, "wall door building compile should count compiled door");
	Expect(!result.hasIssues(), "valid wall door building compile should report no issues");
	if (result.wallCuts.segments.size() == 2) {
		Expect(NearVec(result.wallCuts.segments[0].center, { -3.0F, 0.0F }), "left wall segment should be preserved");
		Expect(NearVec(result.wallCuts.segments[1].center, { 3.0F, 0.0F }), "right wall segment should be preserved");
		Expect(result.wallCuts.segments[0].sourceDoorIds.size() == 1 && result.wallCuts.segments[0].sourceDoorIds[0] == door.id, "cut segment should preserve source door id");
	}
}

void TestUnknownSymbolIssueRemainsVisibleWhileValidWallCompiles()
{
	const iggy::DraftSymbol2D unknown = Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown);
	const iggy::DraftSymbol2D wall = Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 2.0F, 0.0F }, { 4.0F, 2.0F });

	const iggy::DraftBuildingCompile2DResult result =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ unknown, wall }));

	Expect(result.plan.issues.size() == 1, "unknown draft symbol should remain visible in compile plan issues");
	Expect(result.walls.walls.size() == 1, "valid wall should still compile when unknown symbol exists");
	Expect(result.wallCuts.segments.size() == 1, "valid wall should still produce wall segment when unknown symbol exists");
	Expect(result.hasIssues(), "unknown draft symbol should make building compile report issues");
	if (result.plan.issues.size() == 1)
		Expect(result.plan.issues[0].code == iggy::DraftCompilePlan2DIssueCode::UnknownSymbolKind, "unknown issue should preserve issue code");
}

void TestInvalidWallSizeRemainsVisibleAndPreventsSegmentOutput()
{
	const iggy::DraftSymbol2D wall =
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 0.0F, 2.0F });

	const iggy::DraftBuildingCompile2DResult result =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ wall }));

	Expect(result.plan.walls.size() == 1, "invalid wall size should still be planned");
	Expect(result.walls.issues.size() == 1, "invalid wall size should remain visible in wall compiler issues");
	Expect(result.walls.walls.empty(), "invalid wall size should prevent compiled wall output");
	Expect(result.morphPlan.wallPlans.empty(), "invalid wall size should prevent wall morph plan output");
	Expect(result.wallCuts.segments.empty(), "invalid wall size should prevent wall segment output");
	Expect(result.compiledWallSegmentCount == 0, "invalid wall size should count zero wall segments");
	Expect(result.hasIssues(), "invalid wall size should make building compile report issues");
}

void TestDoorOutsideWallReportsAttachmentIssueAndWallPassThrough()
{
	const iggy::DraftSymbol2D wall =
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 4.0F, 2.0F });
	const iggy::DraftSymbol2D door =
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 10.0F, 0.0F }, { 1.0F, 2.0F });

	const iggy::DraftBuildingCompile2DResult result =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ wall, door }));

	Expect(result.doors.doors.size() == 1, "outside door should still compile as door fact");
	Expect(result.attachments.issues.size() == 1, "outside door should remain visible in attachment issues");
	Expect(result.wallCuts.segments.size() == 1, "outside door should leave wall as pass-through segment");
	Expect(result.hasIssues(), "outside door should make building compile report issues");
	if (result.attachments.issues.size() == 1)
		Expect(result.attachments.issues[0].code == iggy::DraftWallDoorAttach2DIssueCode::NoContainingWall, "outside door should report NoContainingWall");
	if (result.wallCuts.segments.size() == 1)
		Expect(!result.wallCuts.segments[0].hasDoorCut, "outside door pass-through segment should report no door cut");
}

void TestOverlappingDoorCutsSurfaceCutIssueAndPassThroughSegment()
{
	const iggy::DraftSymbol2D wall =
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 10.0F, 2.0F });
	const iggy::DraftSymbol2D doorA =
		Symbol("draft:door_a", iggy::DraftSymbol2DKind::Door, { -0.5F, 0.0F }, { 3.0F, 2.0F });
	const iggy::DraftSymbol2D doorB =
		Symbol("draft:door_b", iggy::DraftSymbol2DKind::Door, { 0.5F, 0.0F }, { 3.0F, 2.0F });

	const iggy::DraftBuildingCompile2DResult result =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ wall, doorA, doorB }));

	Expect(result.attachments.attachments.size() == 2, "overlapping door compile should attach both doors before cut validation");
	Expect(result.wallCuts.issues.size() == 1, "overlapping door compile should surface cut issue");
	Expect(result.wallCuts.segments.size() == 1, "overlapping door compile should preserve original wall segment");
	Expect(result.hasIssues(), "overlapping door cuts should make building compile report issues");
	if (result.wallCuts.issues.size() == 1)
		Expect(result.wallCuts.issues[0].code == iggy::DraftWallCutCompile2DIssueCode::OverlappingDoorCuts, "overlapping door compile should report OverlappingDoorCuts");
	if (result.wallCuts.segments.size() == 1) {
		Expect(NearVec(result.wallCuts.segments[0].center, wall.position), "overlapping door pass-through segment should preserve wall center");
		Expect(!result.wallCuts.segments[0].hasDoorCut, "overlapping door pass-through segment should report no door cut");
	}
}

void TestCountsReflectNestedOutput()
{
	const iggy::DraftSymbol2D wall =
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 12.0F, 2.0F });
	const iggy::DraftSymbol2D doorA =
		Symbol("draft:door_a", iggy::DraftSymbol2DKind::Door, { -3.0F, 0.0F }, { 2.0F, 2.0F });
	const iggy::DraftSymbol2D doorB =
		Symbol("draft:door_b", iggy::DraftSymbol2DKind::Door, { 3.0F, 0.0F }, { 2.0F, 2.0F });

	const iggy::DraftBuildingCompile2DResult result =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ wall, doorA, doorB }));

	Expect(result.wallCuts.segments.size() == 3, "two valid door cuts should produce three wall segments");
	Expect(result.doors.doors.size() == 2, "two valid doors should compile");
	Expect(result.compiledWallSegmentCount == result.wallCuts.segments.size(), "building segment count should mirror wall cut output");
	Expect(result.compiledDoorCount == result.doors.doors.size(), "building door count should mirror door compile output");
}

void TestInputDocumentIsNotMutated()
{
	const std::vector<iggy::DraftSymbol2D> symbols {
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 10.0F, 2.0F }),
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { 2.0F, 2.0F }),
		Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown),
	};
	const iggy::DraftDocument2D document = Document(symbols);
	const std::vector<iggy::DraftSymbol2D> before = document.symbols;

	const iggy::DraftBuildingCompile2DResult result = iggy::DraftBuildingCompiler2D {}.compile(document);

	Expect(result.walls.walls.size() == 1 && result.doors.doors.size() == 1 && result.plan.issues.size() == 1, "building compile immutability setup should exercise multiple stages");
	Expect(SameSymbols(document.symbols, before), "draft building compiler should not mutate input document");
}

} // namespace

int main()
{
	TestEmptyDocumentCompilesThroughEmptyStages();
	TestOneWallNoDoorsProducesOneFinalWallSegment();
	TestWallAndAttachedDoorProduceCutSegmentsAndDoorFacts();
	TestUnknownSymbolIssueRemainsVisibleWhileValidWallCompiles();
	TestInvalidWallSizeRemainsVisibleAndPreventsSegmentOutput();
	TestDoorOutsideWallReportsAttachmentIssueAndWallPassThrough();
	TestOverlappingDoorCutsSurfaceCutIssueAndPassThroughSegment();
	TestCountsReflectNestedOutput();
	TestInputDocumentIsNotMutated();

	return Failures;
}
