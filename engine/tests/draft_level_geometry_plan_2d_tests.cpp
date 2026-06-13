#include <cstdlib>
#include <vector>

#include "scene/draft/DraftLevelGeometryPlan2D.hpp"
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
	const char *definitionId = "definition:symbol")
{
	return {
		Id(id),
		kind,
		position,
		size,
		rotationRadians,
		Id(assetId),
		Id(definitionId),
		true,
	};
}

iggy::DraftDocument2D Document(std::vector<iggy::DraftSymbol2D> symbols)
{
	return { symbols };
}

iggy::DraftCompiledWallSegment2D Segment(
	const char *wallId,
	std::size_t wallIndex,
	iggy::Vec2 center,
	iggy::Vec2 size,
	float rotationRadians = 0.0F,
	const char *assetId = "asset:wall",
	const char *definitionId = "definition:wall")
{
	return {
		Id(wallId),
		wallIndex,
		center,
		size,
		rotationRadians,
		Id(assetId),
		Id(definitionId),
		false,
		{},
	};
}

iggy::DraftBuildingCompile2DResult BuildingWithSegments(std::vector<iggy::DraftCompiledWallSegment2D> segments)
{
	iggy::DraftBuildingCompile2DResult building;
	building.wallCuts.segments = segments;
	building.compiledWallSegmentCount = segments.size();
	return building;
}

void ExpectBox(
	const iggy::DraftLevelCollisionBox2D &actual,
	const iggy::DraftCompiledWallSegment2D &segment,
	const char *message)
{
	Expect(actual.sourceWallId == segment.sourceWallId, message);
	Expect(actual.sourceWallIndex == segment.sourceWallIndex, message);
	Expect(NearVec(actual.center, segment.center), message);
	Expect(NearVec(actual.size, segment.size), message);
	Expect(actual.rotationRadians == segment.rotationRadians, message);
	Expect(actual.assetId == segment.assetId, message);
	Expect(actual.definitionId == segment.definitionId, message);
}

bool SameSegments(
	const std::vector<iggy::DraftCompiledWallSegment2D> &actual,
	const std::vector<iggy::DraftCompiledWallSegment2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].sourceWallId != expected[index].sourceWallId
			|| actual[index].sourceWallIndex != expected[index].sourceWallIndex
			|| !NearVec(actual[index].center, expected[index].center)
			|| !NearVec(actual[index].size, expected[index].size)
			|| actual[index].rotationRadians != expected[index].rotationRadians
			|| actual[index].assetId != expected[index].assetId
			|| actual[index].definitionId != expected[index].definitionId) {
			return false;
		}
	}
	return true;
}

void TestCleanBuildingCompileWithOneWallSegmentProducesOneCollisionBox()
{
	const iggy::DraftSymbol2D wall =
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 1.0F, 2.0F }, { 6.0F, 2.0F }, 0.0F, "asset:wall", "definition:wall");
	const iggy::DraftBuildingCompile2DResult building =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ wall }));

	const iggy::DraftLevelGeometryPlan2DResult result = iggy::DraftLevelGeometryPlanner2D {}.plan(building);

	Expect(result.collisionBoxes.size() == 1, "clean one-wall building compile should produce one collision box");
	Expect(result.issues.empty(), "clean one-wall building compile should produce no geometry issues");
	Expect(result.sourceWallSegmentCount == 1, "geometry plan should preserve source segment count");
	Expect(!result.buildingCompileHadIssues, "clean one-wall building compile should report no building issues");
	if (result.collisionBoxes.size() == 1)
		ExpectBox(result.collisionBoxes[0], building.wallCuts.segments[0], "collision box should preserve wall segment payload");
}

void TestWallDoorCutCompileProducesCollisionBoxesForSplitSegments()
{
	const iggy::DraftSymbol2D wall =
		Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 10.0F, 2.0F });
	const iggy::DraftSymbol2D door =
		Symbol("draft:door", iggy::DraftSymbol2DKind::Door, { 0.0F, 0.0F }, { 2.0F, 2.0F });
	const iggy::DraftBuildingCompile2DResult building =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ wall, door }));

	const iggy::DraftLevelGeometryPlan2DResult result = iggy::DraftLevelGeometryPlanner2D {}.plan(building);

	Expect(building.wallCuts.segments.size() == 2, "wall door setup should create split wall segments");
	Expect(result.collisionBoxes.size() == 2, "split wall segments should become collision boxes");
	Expect(result.issues.empty(), "clean split wall segments should produce no geometry issues");
	if (result.collisionBoxes.size() == 2) {
		ExpectBox(result.collisionBoxes[0], building.wallCuts.segments[0], "first split collision box should preserve first segment");
		ExpectBox(result.collisionBoxes[1], building.wallCuts.segments[1], "second split collision box should preserve second segment");
	}
}

void TestBuildingCompileWithIssuesBlocksOutputByDefault()
{
	const iggy::DraftSymbol2D unknown = Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown);
	const iggy::DraftSymbol2D wall = Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 4.0F, 2.0F });
	const iggy::DraftBuildingCompile2DResult building =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ unknown, wall }));

	const iggy::DraftLevelGeometryPlan2DResult result = iggy::DraftLevelGeometryPlanner2D {}.plan(building);

	Expect(building.hasIssues(), "issue-bearing building compile setup should have nested issues");
	Expect(result.collisionBoxes.empty(), "issue-bearing building compile should block geometry output by default");
	Expect(result.issues.size() == 1, "issue-bearing building compile should report geometry issue");
	Expect(result.buildingCompileHadIssues, "geometry plan should preserve building issue signal");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftLevelGeometryPlan2DIssueCode::BuildingCompileHasIssues, "default blocked geometry issue should be BuildingCompileHasIssues");
		Expect(result.issues[0].buildingIssueCount == 1, "default blocked geometry issue should preserve nested issue count");
	}
}

void TestIssueBearingCompileCanProduceValidOutputWhenAllowed()
{
	const iggy::DraftSymbol2D unknown = Symbol("draft:unknown", iggy::DraftSymbol2DKind::Unknown);
	const iggy::DraftSymbol2D wall = Symbol("draft:wall", iggy::DraftSymbol2DKind::Wall, { 0.0F, 0.0F }, { 4.0F, 2.0F });
	const iggy::DraftBuildingCompile2DResult building =
		iggy::DraftBuildingCompiler2D {}.compile(Document({ unknown, wall }));
	const iggy::DraftLevelGeometryPlan2DConfig config { true };

	const iggy::DraftLevelGeometryPlan2DResult result = iggy::DraftLevelGeometryPlanner2D {}.plan(building, config);

	Expect(result.collisionBoxes.size() == 1, "allowed issue-bearing compile should still produce valid collision boxes");
	Expect(result.issues.size() == 1, "allowed issue-bearing compile should preserve issue signal");
	Expect(result.hasIssues(), "allowed issue-bearing compile should report issues");
	if (result.collisionBoxes.size() == 1)
		ExpectBox(result.collisionBoxes[0], building.wallCuts.segments[0], "allowed issue-bearing collision box should preserve segment");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::DraftLevelGeometryPlan2DIssueCode::BuildingCompileHasIssues, "allowed issue-bearing issue should be BuildingCompileHasIssues");
}

void TestNonPositiveSegmentSizeReportsIssueDefensively()
{
	iggy::DraftBuildingCompile2DResult building = BuildingWithSegments({
		Segment("draft:wall", 0, { 0.0F, 0.0F }, { 0.0F, 2.0F }),
	});

	const iggy::DraftLevelGeometryPlan2DResult result = iggy::DraftLevelGeometryPlanner2D {}.plan(building);

	Expect(result.collisionBoxes.empty(), "non-positive segment should not produce collision box");
	Expect(result.issues.size() == 1, "non-positive segment should produce issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftLevelGeometryPlan2DIssueCode::NonPositiveSegmentSize, "non-positive segment issue should use NonPositiveSegmentSize");
		Expect(result.issues[0].segmentIndex == 0, "non-positive segment issue should preserve segment index");
		ExpectBox({ result.issues[0].segment.sourceWallId, result.issues[0].segment.sourceWallIndex, result.issues[0].segment.center, result.issues[0].segment.size, result.issues[0].segment.rotationRadians, result.issues[0].segment.assetId, result.issues[0].segment.definitionId }, building.wallCuts.segments[0], "non-positive segment issue should copy segment payload");
	}
}

void TestRotatedSegmentReportsUnsupportedAndSkipsOutput()
{
	iggy::DraftBuildingCompile2DResult building = BuildingWithSegments({
		Segment("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 2.0F }, 0.25F),
	});

	const iggy::DraftLevelGeometryPlan2DResult result = iggy::DraftLevelGeometryPlanner2D {}.plan(building);

	Expect(result.collisionBoxes.empty(), "rotated segment should not produce collision box");
	Expect(result.issues.size() == 1, "rotated segment should produce issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftLevelGeometryPlan2DIssueCode::UnsupportedRotatedSegment, "rotated segment issue should use UnsupportedRotatedSegment");
		Expect(result.issues[0].segmentIndex == 0, "rotated segment issue should preserve segment index");
	}
}

void TestOrderPreservedAcrossMultipleSegments()
{
	iggy::DraftBuildingCompile2DResult building = BuildingWithSegments({
		Segment("draft:wall_a", 0, { -5.0F, 0.0F }, { 2.0F, 2.0F }),
		Segment("draft:wall_b", 1, { 0.0F, 0.0F }, { 4.0F, 2.0F }),
		Segment("draft:wall_c", 2, { 5.0F, 0.0F }, { 6.0F, 2.0F }),
	});

	const iggy::DraftLevelGeometryPlan2DResult result = iggy::DraftLevelGeometryPlanner2D {}.plan(building);

	Expect(result.collisionBoxes.size() == 3, "multiple valid segments should produce three boxes");
	if (result.collisionBoxes.size() == 3) {
		Expect(result.collisionBoxes[0].sourceWallId == Id("draft:wall_a"), "first collision box should preserve segment order");
		Expect(result.collisionBoxes[1].sourceWallId == Id("draft:wall_b"), "second collision box should preserve segment order");
		Expect(result.collisionBoxes[2].sourceWallId == Id("draft:wall_c"), "third collision box should preserve segment order");
	}
}

void TestInputBuildingCompileResultIsNotMutated()
{
	iggy::DraftBuildingCompile2DResult building = BuildingWithSegments({
		Segment("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 2.0F }),
	});
	const std::vector<iggy::DraftCompiledWallSegment2D> before = building.wallCuts.segments;

	const iggy::DraftLevelGeometryPlan2DResult result = iggy::DraftLevelGeometryPlanner2D {}.plan(building);

	Expect(result.collisionBoxes.size() == 1, "geometry immutability setup should produce one box");
	Expect(SameSegments(building.wallCuts.segments, before), "draft level geometry planner should not mutate input building result");
}

} // namespace

int main()
{
	TestCleanBuildingCompileWithOneWallSegmentProducesOneCollisionBox();
	TestWallDoorCutCompileProducesCollisionBoxesForSplitSegments();
	TestBuildingCompileWithIssuesBlocksOutputByDefault();
	TestIssueBearingCompileCanProduceValidOutputWhenAllowed();
	TestNonPositiveSegmentSizeReportsIssueDefensively();
	TestRotatedSegmentReportsUnsupportedAndSkipsOutput();
	TestOrderPreservedAcrossMultipleSegments();
	TestInputBuildingCompileResultIsNotMutated();

	return Failures;
}
