#include <cstdlib>
#include <vector>

#include "scene/draft/DraftWallCutCompile2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::DraftCompiledWall2D Wall(
	const char *id,
	std::size_t sourceIndex,
	iggy::Vec2 center,
	iggy::Vec2 size,
	float rotationRadians = 0.0F,
	const char *assetId = "asset:wall",
	const char *definitionId = "definition:wall")
{
	return {
		Id(id),
		sourceIndex,
		center,
		size,
		rotationRadians,
		Id(assetId),
		Id(definitionId),
	};
}

iggy::DraftCompiledDoor2D Door(
	const char *id,
	std::size_t sourceIndex,
	iggy::Vec2 center,
	iggy::Vec2 size = { 1.0F, 2.0F },
	float rotationRadians = 0.0F,
	const char *assetId = "asset:door",
	const char *definitionId = "definition:door")
{
	return {
		Id(id),
		sourceIndex,
		center,
		size,
		rotationRadians,
		Id(assetId),
		Id(definitionId),
		iggy::DraftDoorSwing2D::Unknown,
	};
}

iggy::DraftWallDoorMorphPlanDoor2D PlanDoor(std::size_t doorIndex, const iggy::DraftCompiledDoor2D &door)
{
	return { doorIndex, door };
}

iggy::DraftWallDoorMorphPlanWall2D PlanWall(
	std::size_t wallIndex,
	const iggy::DraftCompiledWall2D &wall,
	std::vector<iggy::DraftWallDoorMorphPlanDoor2D> doors = {})
{
	return { wallIndex, wall, doors };
}

iggy::DraftWallDoorMorphPlan2DResult Plan(std::vector<iggy::DraftWallDoorMorphPlanWall2D> wallPlans)
{
	iggy::DraftWallDoorMorphPlan2DResult result;
	result.wallPlans = wallPlans;
	return result;
}

void ExpectSegment(
	const iggy::DraftCompiledWallSegment2D &actual,
	const iggy::DraftCompiledWall2D &wall,
	iggy::Vec2 center,
	iggy::Vec2 size,
	bool hasDoorCut,
	const char *message)
{
	Expect(actual.sourceWallId == wall.sourceSymbolId, message);
	Expect(actual.sourceWallIndex == wall.sourceSymbolIndex, message);
	Expect(NearVec(actual.center, center), message);
	Expect(NearVec(actual.size, size), message);
	Expect(actual.rotationRadians == wall.rotationRadians, message);
	Expect(actual.assetId == wall.assetId, message);
	Expect(actual.definitionId == wall.definitionId, message);
	Expect(actual.hasDoorCut == hasDoorCut, message);
}

void ExpectOriginalSegment(const iggy::DraftCompiledWallSegment2D &actual, const iggy::DraftCompiledWall2D &wall, const char *message)
{
	ExpectSegment(actual, wall, wall.center, wall.size, false, message);
	Expect(actual.sourceDoorIds.empty(), message);
}

bool SameWallPlans(
	const std::vector<iggy::DraftWallDoorMorphPlanWall2D> &actual,
	const std::vector<iggy::DraftWallDoorMorphPlanWall2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t wallIndex = 0; wallIndex < actual.size(); ++wallIndex) {
		if (actual[wallIndex].wallIndex != expected[wallIndex].wallIndex
			|| actual[wallIndex].wall.sourceSymbolId != expected[wallIndex].wall.sourceSymbolId
			|| !NearVec(actual[wallIndex].wall.center, expected[wallIndex].wall.center)
			|| !NearVec(actual[wallIndex].wall.size, expected[wallIndex].wall.size)
			|| actual[wallIndex].doors.size() != expected[wallIndex].doors.size()) {
			return false;
		}
		for (std::size_t doorIndex = 0; doorIndex < actual[wallIndex].doors.size(); ++doorIndex) {
			if (actual[wallIndex].doors[doorIndex].doorIndex != expected[wallIndex].doors[doorIndex].doorIndex
				|| actual[wallIndex].doors[doorIndex].door.sourceSymbolId != expected[wallIndex].doors[doorIndex].door.sourceSymbolId) {
				return false;
			}
		}
	}
	return true;
}

void TestWallWithNoDoorsOutputsUnchangedSegment()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 8.0F, 2.0F }, 0.25F);

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall) }));

	Expect(result.segments.size() == 1, "wall without doors should output one segment");
	Expect(result.issues.empty(), "wall without doors should not produce cut issues");
	if (result.segments.size() == 1)
		ExpectOriginalSegment(result.segments[0], wall, "wall without doors should preserve original segment");
}

void TestHorizontalCenteredDoorOutputsLeftAndRightSegments()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 10.0F, 2.0F });
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 0.0F, 0.0F }, { 2.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(0, door) }) }));

	Expect(result.segments.size() == 2, "centered horizontal door should output two segments");
	Expect(result.issues.empty(), "centered horizontal door should not produce cut issues");
	if (result.segments.size() == 2) {
		ExpectSegment(result.segments[0], wall, { -3.0F, 0.0F }, { 4.0F, 2.0F }, true, "left segment should preserve horizontal geometry");
		ExpectSegment(result.segments[1], wall, { 3.0F, 0.0F }, { 4.0F, 2.0F }, true, "right segment should preserve horizontal geometry");
		Expect(result.segments[0].sourceDoorIds.size() == 1 && result.segments[0].sourceDoorIds[0] == Id("draft:door"), "left segment should preserve source door id");
		Expect(result.segments[1].sourceDoorIds.size() == 1 && result.segments[1].sourceDoorIds[0] == Id("draft:door"), "right segment should preserve source door id");
	}
}

void TestHorizontalDoorAtOneEndOutputsOneRemainingSegment()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 10.0F, 2.0F });
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { -4.0F, 0.0F }, { 2.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(0, door) }) }));

	Expect(result.segments.size() == 1, "horizontal door at one end should output one segment");
	if (result.segments.size() == 1)
		ExpectSegment(result.segments[0], wall, { 1.0F, 0.0F }, { 8.0F, 2.0F }, true, "remaining segment should cover space after end door");
}

void TestVerticalCenteredDoorOutputsLowerAndUpperSegments()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 2.0F, 10.0F });
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 0.0F, 0.0F }, { 2.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(0, door) }) }));

	Expect(result.segments.size() == 2, "centered vertical door should output two segments");
	if (result.segments.size() == 2) {
		ExpectSegment(result.segments[0], wall, { 0.0F, -3.0F }, { 2.0F, 4.0F }, true, "lower vertical segment should be first");
		ExpectSegment(result.segments[1], wall, { 0.0F, 3.0F }, { 2.0F, 4.0F }, true, "upper vertical segment should be second");
	}
}

void TestMultipleNonOverlappingDoorsProduceOrderedSegments()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 12.0F, 2.0F });
	const iggy::DraftCompiledDoor2D doorA = Door("draft:door_a", 1, { -3.0F, 0.0F }, { 2.0F, 2.0F });
	const iggy::DraftCompiledDoor2D doorB = Door("draft:door_b", 2, { 3.0F, 0.0F }, { 2.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(0, doorA), PlanDoor(1, doorB) }) }));

	Expect(result.segments.size() == 3, "two door cuts should produce three ordered segments");
	if (result.segments.size() == 3) {
		ExpectSegment(result.segments[0], wall, { -5.0F, 0.0F }, { 2.0F, 2.0F }, true, "first segment should be left of first door");
		ExpectSegment(result.segments[1], wall, { 0.0F, 0.0F }, { 4.0F, 2.0F }, true, "second segment should be between doors");
		ExpectSegment(result.segments[2], wall, { 5.0F, 0.0F }, { 2.0F, 2.0F }, true, "third segment should be right of second door");
	}
}

void TestDoorOutsideWallBoundsReportsIssueAndPreservesOriginalSegment()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 8.0F, 2.0F });
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 4.5F, 0.0F }, { 2.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(3, door) }) }));

	Expect(result.segments.size() == 1, "outside door should preserve original wall segment");
	Expect(result.issues.size() == 1, "outside door should produce cut issue");
	if (result.segments.size() == 1)
		ExpectOriginalSegment(result.segments[0], wall, "outside door should preserve original segment");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftWallCutCompile2DIssueCode::DoorOutsideWallBounds, "outside door issue should use DoorOutsideWallBounds");
		Expect(result.issues[0].wallPlanIndex == 0, "outside door issue should preserve wall plan index");
		Expect(result.issues[0].doors.size() == 1 && result.issues[0].doors[0].doorIndex == 3, "outside door issue should preserve door context");
	}
}

void TestOverlappingDoorsReportIssueAndPreserveOriginalSegment()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 10.0F, 2.0F });
	const iggy::DraftCompiledDoor2D doorA = Door("draft:door_a", 1, { -0.5F, 0.0F }, { 3.0F, 2.0F });
	const iggy::DraftCompiledDoor2D doorB = Door("draft:door_b", 2, { 0.5F, 0.0F }, { 3.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(0, doorA), PlanDoor(1, doorB) }) }));

	Expect(result.segments.size() == 1, "overlapping doors should preserve original wall segment");
	Expect(result.issues.size() == 1, "overlapping doors should produce cut issue");
	if (result.segments.size() == 1)
		ExpectOriginalSegment(result.segments[0], wall, "overlapping doors should preserve original segment");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::DraftWallCutCompile2DIssueCode::OverlappingDoorCuts, "overlapping door issue should use OverlappingDoorCuts");
}

void TestOversizedDoorReportsIssueAndPreservesOriginalSegment()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 8.0F, 2.0F });
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 0.0F, 0.0F }, { 9.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(0, door) }) }));

	Expect(result.segments.size() == 1, "oversized door should preserve original wall segment");
	Expect(result.issues.size() == 1, "oversized door should produce cut issue");
	if (result.segments.size() == 1)
		ExpectOriginalSegment(result.segments[0], wall, "oversized door should preserve original segment");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::DraftWallCutCompile2DIssueCode::DoorLargerThanWall, "oversized door issue should use DoorLargerThanWall");
}

void TestRotatedWallReportsIssueAndPreservesOriginalSegment()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 8.0F, 2.0F }, 0.25F);
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 0.0F, 0.0F }, { 2.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(0, door) }) }));

	Expect(result.segments.size() == 1, "rotated wall should preserve original wall segment");
	Expect(result.issues.size() == 1, "rotated wall should produce unsupported issue");
	if (result.segments.size() == 1)
		ExpectOriginalSegment(result.segments[0], wall, "rotated wall should preserve original segment");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::DraftWallCutCompile2DIssueCode::UnsupportedRotatedWall, "rotated wall issue should use UnsupportedRotatedWall");
}

void TestNoPositiveSegmentProducedReportsIssueAndPreservesOriginalSegment()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 8.0F, 2.0F });
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 0.0F, 0.0F }, { 8.0F, 2.0F });

	const iggy::DraftWallCutCompile2DResult result =
		iggy::DraftWallCutCompiler2D {}.compile(Plan({ PlanWall(0, wall, { PlanDoor(0, door) }) }));

	Expect(result.segments.size() == 1, "full-width door should preserve original wall segment");
	Expect(result.issues.size() == 1, "full-width door should produce no-positive-segment issue");
	if (result.segments.size() == 1)
		ExpectOriginalSegment(result.segments[0], wall, "full-width door should preserve original segment");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::DraftWallCutCompile2DIssueCode::NoPositiveSegmentProduced, "full-width door issue should use NoPositiveSegmentProduced");
}

void TestInputPlanIsNotMutated()
{
	const iggy::DraftCompiledWall2D wall = Wall("draft:wall", 0, { 0.0F, 0.0F }, { 10.0F, 2.0F });
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 0.0F, 0.0F }, { 2.0F, 2.0F });
	iggy::DraftWallDoorMorphPlan2DResult plan = Plan({ PlanWall(0, wall, { PlanDoor(0, door) }) });
	const std::vector<iggy::DraftWallDoorMorphPlanWall2D> before = plan.wallPlans;

	const iggy::DraftWallCutCompile2DResult result = iggy::DraftWallCutCompiler2D {}.compile(plan);

	Expect(result.segments.size() == 2, "cut compile immutability setup should produce cut segments");
	Expect(SameWallPlans(plan.wallPlans, before), "draft wall cut compiler should not mutate input plan");
}

} // namespace

int main()
{
	TestWallWithNoDoorsOutputsUnchangedSegment();
	TestHorizontalCenteredDoorOutputsLeftAndRightSegments();
	TestHorizontalDoorAtOneEndOutputsOneRemainingSegment();
	TestVerticalCenteredDoorOutputsLowerAndUpperSegments();
	TestMultipleNonOverlappingDoorsProduceOrderedSegments();
	TestDoorOutsideWallBoundsReportsIssueAndPreservesOriginalSegment();
	TestOverlappingDoorsReportIssueAndPreserveOriginalSegment();
	TestOversizedDoorReportsIssueAndPreservesOriginalSegment();
	TestRotatedWallReportsIssueAndPreservesOriginalSegment();
	TestNoPositiveSegmentProducedReportsIssueAndPreservesOriginalSegment();
	TestInputPlanIsNotMutated();

	return Failures;
}
