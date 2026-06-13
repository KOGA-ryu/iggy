#include <cstdlib>
#include <vector>

#include "scene/level/LevelCollisionSource2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::DraftLevelCollisionBox2D GeometryBox(
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
	};
}

iggy::DraftLevelGeometryPlan2DResult GeometryPlan(std::vector<iggy::DraftLevelCollisionBox2D> boxes)
{
	iggy::DraftLevelGeometryPlan2DResult geometry;
	geometry.collisionBoxes = boxes;
	geometry.sourceWallSegmentCount = boxes.size();
	return geometry;
}

void AddGeometryIssue(iggy::DraftLevelGeometryPlan2DResult &geometry)
{
	geometry.issues.push_back({
		iggy::DraftLevelGeometryPlan2DIssueCode::BuildingCompileHasIssues,
		0,
		{},
		1,
	});
}

void ExpectSourceBox(
	const iggy::LevelCollisionSourceBox2D &actual,
	const iggy::DraftLevelCollisionBox2D &expected,
	const char *message)
{
	Expect(actual.sourceWallId == expected.sourceWallId, message);
	Expect(actual.sourceWallIndex == expected.sourceWallIndex, message);
	Expect(NearVec(actual.center, expected.center), message);
	Expect(NearVec(actual.size, expected.size), message);
	Expect(actual.rotationRadians == expected.rotationRadians, message);
	Expect(actual.assetId == expected.assetId, message);
	Expect(actual.definitionId == expected.definitionId, message);
}

bool SameGeometryPlan(
	const iggy::DraftLevelGeometryPlan2DResult &actual,
	const iggy::DraftLevelGeometryPlan2DResult &expected)
{
	if (actual.collisionBoxes.size() != expected.collisionBoxes.size()
		|| actual.issues.size() != expected.issues.size()
		|| actual.sourceWallSegmentCount != expected.sourceWallSegmentCount
		|| actual.buildingCompileHadIssues != expected.buildingCompileHadIssues) {
		return false;
	}

	for (std::size_t index = 0; index < actual.collisionBoxes.size(); ++index) {
		const iggy::DraftLevelCollisionBox2D &left = actual.collisionBoxes[index];
		const iggy::DraftLevelCollisionBox2D &right = expected.collisionBoxes[index];
		if (left.sourceWallId != right.sourceWallId
			|| left.sourceWallIndex != right.sourceWallIndex
			|| !NearVec(left.center, right.center)
			|| !NearVec(left.size, right.size)
			|| left.rotationRadians != right.rotationRadians
			|| left.assetId != right.assetId
			|| left.definitionId != right.definitionId) {
			return false;
		}
	}

	return true;
}

void TestEmptyPlanBuildsValidEmptySource()
{
	const iggy::DraftLevelGeometryPlan2DResult geometry;

	const iggy::LevelCollisionSource2DBuildResult result = iggy::LevelCollisionSource2DBuilder {}.build(geometry);

	Expect(result.built, "empty geometry plan should build valid empty collision source");
	Expect(result.source.boxes.empty(), "empty geometry plan should produce no source boxes");
	Expect(result.issues.empty(), "empty geometry plan should produce no issues");
	Expect(result.sourceGeometryBoxCount == 0, "empty geometry plan should preserve source box count");
	Expect(!result.geometryPlanHadIssues, "empty geometry plan should not report source plan issues");
}

void TestCleanGeometryPlanBuildsSourceBoxesPreservingFieldsAndOrder()
{
	const iggy::DraftLevelGeometryPlan2DResult geometry = GeometryPlan({
		GeometryBox("draft:wall_a", 0, { -2.0F, 1.0F }, { 4.0F, 1.0F }, 0.0F, "asset:a", "definition:a"),
		GeometryBox("draft:wall_b", 2, { 3.0F, 4.0F }, { 2.0F, 5.0F }, 0.0F, "asset:b", "definition:b"),
	});

	const iggy::LevelCollisionSource2DBuildResult result = iggy::LevelCollisionSource2DBuilder {}.build(geometry);

	Expect(result.built, "clean geometry plan should build collision source");
	Expect(result.source.boxes.size() == 2, "clean geometry plan should produce source boxes");
	Expect(result.issues.empty(), "clean geometry plan should produce no source issues");
	Expect(result.sourceGeometryBoxCount == 2, "source builder should preserve input geometry box count");
	if (result.source.boxes.size() == 2) {
		ExpectSourceBox(result.source.boxes[0], geometry.collisionBoxes[0], "first source box should preserve first geometry box");
		ExpectSourceBox(result.source.boxes[1], geometry.collisionBoxes[1], "second source box should preserve second geometry box");
	}
}

void TestIssueBearingGeometryPlanBlocksOutputByDefault()
{
	iggy::DraftLevelGeometryPlan2DResult geometry = GeometryPlan({
		GeometryBox("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 2.0F }),
	});
	AddGeometryIssue(geometry);

	const iggy::LevelCollisionSource2DBuildResult result = iggy::LevelCollisionSource2DBuilder {}.build(geometry);

	Expect(!result.built, "issue-bearing geometry plan should not build by default");
	Expect(result.source.boxes.empty(), "issue-bearing geometry plan should publish no source boxes by default");
	Expect(result.issues.size() == 1, "issue-bearing geometry plan should produce source issue");
	Expect(result.geometryPlanHadIssues, "source builder should preserve geometry issue signal");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelCollisionSource2DIssueCode::GeometryPlanHasIssues, "blocked issue should be GeometryPlanHasIssues");
		Expect(result.issues[0].geometryIssueCount == 1, "blocked issue should preserve geometry issue count");
	}
}

void TestAllowedIssueBearingGeometryPlanBuildsValidBoxesAndPreservesIssue()
{
	iggy::DraftLevelGeometryPlan2DResult geometry = GeometryPlan({
		GeometryBox("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 2.0F }),
	});
	AddGeometryIssue(geometry);
	const iggy::LevelCollisionSource2DConfig config { true };

	const iggy::LevelCollisionSource2DBuildResult result = iggy::LevelCollisionSource2DBuilder {}.build(geometry, config);

	Expect(result.built, "allowed issue-bearing geometry plan should build from valid boxes");
	Expect(result.source.boxes.size() == 1, "allowed issue-bearing geometry plan should produce valid source boxes");
	Expect(result.issues.size() == 1, "allowed issue-bearing geometry plan should preserve issue signal");
	if (result.source.boxes.size() == 1)
		ExpectSourceBox(result.source.boxes[0], geometry.collisionBoxes[0], "allowed issue-bearing source box should preserve geometry box");
}

void TestNonPositiveBoxSizeIsRejectedDeterministically()
{
	const iggy::DraftLevelGeometryPlan2DResult geometry = GeometryPlan({
		GeometryBox("draft:valid", 0, { -3.0F, 0.0F }, { 2.0F, 2.0F }),
		GeometryBox("draft:invalid", 1, { 0.0F, 0.0F }, { 0.0F, 2.0F }),
		GeometryBox("draft:valid_after", 2, { 3.0F, 0.0F }, { 2.0F, 2.0F }),
	});

	const iggy::LevelCollisionSource2DBuildResult result = iggy::LevelCollisionSource2DBuilder {}.build(geometry);

	Expect(result.built, "geometry plan with rejected boxes should still build valid source subset");
	Expect(result.source.boxes.size() == 2, "non-positive box should be skipped");
	Expect(result.issues.size() == 1, "non-positive box should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelCollisionSource2DIssueCode::NonPositiveBoxSize, "non-positive box issue should use NonPositiveBoxSize");
		Expect(result.issues[0].boxIndex == 1, "non-positive box issue should preserve box index");
		Expect(result.issues[0].box.sourceWallId == Id("draft:invalid"), "non-positive box issue should preserve box payload");
	}
	if (result.source.boxes.size() == 2) {
		Expect(result.source.boxes[0].sourceWallId == Id("draft:valid"), "source output should keep first valid box");
		Expect(result.source.boxes[1].sourceWallId == Id("draft:valid_after"), "source output should keep later valid box");
	}
}

void TestUnsupportedRotationIsRejectedDeterministically()
{
	const iggy::DraftLevelGeometryPlan2DResult geometry = GeometryPlan({
		GeometryBox("draft:rotated", 0, { 0.0F, 0.0F }, { 4.0F, 2.0F }, 0.25F),
	});

	const iggy::LevelCollisionSource2DBuildResult result = iggy::LevelCollisionSource2DBuilder {}.build(geometry);

	Expect(result.built, "geometry plan with unsupported rotation should build valid source subset");
	Expect(result.source.boxes.empty(), "unsupported rotated box should be skipped");
	Expect(result.issues.size() == 1, "unsupported rotated box should produce issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelCollisionSource2DIssueCode::UnsupportedRotation, "rotated box issue should use UnsupportedRotation");
		Expect(result.issues[0].boxIndex == 0, "rotated box issue should preserve box index");
	}
}

void TestInputGeometryPlanIsNotMutated()
{
	iggy::DraftLevelGeometryPlan2DResult geometry = GeometryPlan({
		GeometryBox("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 2.0F }),
	});
	const iggy::DraftLevelGeometryPlan2DResult before = geometry;

	const iggy::LevelCollisionSource2DBuildResult result = iggy::LevelCollisionSource2DBuilder {}.build(geometry);

	Expect(result.built, "immutability setup should build source data");
	Expect(SameGeometryPlan(geometry, before), "level collision source builder should not mutate input geometry plan");
}

} // namespace

int main()
{
	TestEmptyPlanBuildsValidEmptySource();
	TestCleanGeometryPlanBuildsSourceBoxesPreservingFieldsAndOrder();
	TestIssueBearingGeometryPlanBlocksOutputByDefault();
	TestAllowedIssueBearingGeometryPlanBuildsValidBoxesAndPreservesIssue();
	TestNonPositiveBoxSizeIsRejectedDeterministically();
	TestUnsupportedRotationIsRejectedDeterministically();
	TestInputGeometryPlanIsNotMutated();

	return Failures;
}
