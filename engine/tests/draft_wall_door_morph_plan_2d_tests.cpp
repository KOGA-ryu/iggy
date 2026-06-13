#include <cstdlib>
#include <vector>

#include "scene/draft/DraftWallDoorMorphPlan2D.hpp"
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

iggy::DraftWallDoorAttachment2D Attachment(
	std::size_t doorIndex,
	std::size_t wallIndex,
	const iggy::DraftCompiledDoor2D &door,
	const iggy::DraftCompiledWall2D &wall)
{
	return {
		doorIndex,
		wallIndex,
		door,
		wall,
	};
}

void ExpectWall(const iggy::DraftCompiledWall2D &actual, const iggy::DraftCompiledWall2D &expected, const char *message)
{
	Expect(actual.sourceSymbolId == expected.sourceSymbolId, message);
	Expect(actual.sourceSymbolIndex == expected.sourceSymbolIndex, message);
	Expect(NearVec(actual.center, expected.center), message);
	Expect(NearVec(actual.size, expected.size), message);
	Expect(actual.rotationRadians == expected.rotationRadians, message);
	Expect(actual.assetId == expected.assetId, message);
	Expect(actual.definitionId == expected.definitionId, message);
}

void ExpectDoor(const iggy::DraftCompiledDoor2D &actual, const iggy::DraftCompiledDoor2D &expected, const char *message)
{
	Expect(actual.sourceSymbolId == expected.sourceSymbolId, message);
	Expect(actual.sourceSymbolIndex == expected.sourceSymbolIndex, message);
	Expect(NearVec(actual.center, expected.center), message);
	Expect(NearVec(actual.size, expected.size), message);
	Expect(actual.rotationRadians == expected.rotationRadians, message);
	Expect(actual.assetId == expected.assetId, message);
	Expect(actual.definitionId == expected.definitionId, message);
	Expect(actual.swing == expected.swing, message);
}

bool SameWalls(const std::vector<iggy::DraftCompiledWall2D> &actual, const std::vector<iggy::DraftCompiledWall2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].sourceSymbolId != expected[index].sourceSymbolId
			|| actual[index].sourceSymbolIndex != expected[index].sourceSymbolIndex
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

bool SameAttachments(
	const std::vector<iggy::DraftWallDoorAttachment2D> &actual,
	const std::vector<iggy::DraftWallDoorAttachment2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].doorIndex != expected[index].doorIndex
			|| actual[index].wallIndex != expected[index].wallIndex) {
			return false;
		}
		if (actual[index].door.sourceSymbolId != expected[index].door.sourceSymbolId
			|| actual[index].wall.sourceSymbolId != expected[index].wall.sourceSymbolId) {
			return false;
		}
	}
	return true;
}

iggy::DraftWallDoorAttach2DResult Attachments(std::vector<iggy::DraftWallDoorAttachment2D> attachments)
{
	iggy::DraftWallDoorAttach2DResult result;
	result.attachments = attachments;
	return result;
}

void TestEmptyWallsAndAttachmentsProduceEmptyResult()
{
	const iggy::DraftWallDoorMorphPlan2DResult result =
		iggy::DraftWallDoorMorphPlanner2D {}.plan({}, {});

	Expect(result.wallPlans.empty(), "empty draft morph plan should have no wall plans");
	Expect(result.issues.empty(), "empty draft morph plan should have no issues");
	Expect(!result.hasIssues(), "empty draft morph plan should report no issues");
}

void TestWallWithNoDoorsAppearsAsNoCutPlan()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
	};

	const iggy::DraftWallDoorMorphPlan2DResult result =
		iggy::DraftWallDoorMorphPlanner2D {}.plan(walls, {});

	Expect(result.wallPlans.size() == 1, "wall without doors should still get morph plan");
	Expect(result.issues.empty(), "wall without doors should not produce morph issue");
	if (result.wallPlans.size() == 1) {
		Expect(result.wallPlans[0].wallIndex == 0, "no-cut wall plan should preserve wall index");
		ExpectWall(result.wallPlans[0].wall, walls[0], "no-cut wall plan should copy wall payload");
		Expect(result.wallPlans[0].doors.empty(), "no-cut wall plan should have no doors");
		Expect(!result.wallPlans[0].hasDoorCuts(), "no-cut wall plan should report no door cuts");
	}
}

void TestSingleDoorAttachmentGroupsUnderCorrectWall()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall_a", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
		Wall("draft:wall_b", 1, { 10.0F, 0.0F }, { 4.0F, 4.0F }),
	};
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 3, { 10.0F, 0.0F });
	const iggy::DraftWallDoorAttach2DResult attachments =
		Attachments({ Attachment(5, 1, door, walls[1]) });

	const iggy::DraftWallDoorMorphPlan2DResult result =
		iggy::DraftWallDoorMorphPlanner2D {}.plan(walls, attachments);

	Expect(result.wallPlans.size() == 2, "single door morph plan should preserve all walls");
	Expect(result.issues.empty(), "valid single door morph plan should not produce issues");
	if (result.wallPlans.size() == 2) {
		Expect(result.wallPlans[0].doors.empty(), "unattached wall should have no door cuts");
		Expect(result.wallPlans[1].doors.size() == 1, "attached wall should have one door cut");
		Expect(result.wallPlans[1].hasDoorCuts(), "attached wall should report door cuts");
		if (result.wallPlans[1].doors.size() == 1) {
			Expect(result.wallPlans[1].doors[0].doorIndex == 5, "grouped door should preserve door index");
			ExpectDoor(result.wallPlans[1].doors[0].door, door, "grouped door should copy door payload");
		}
	}
}

void TestMultipleDoorsOnOneWallPreserveAttachmentOrder()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 0, { 0.0F, 0.0F }, { 8.0F, 4.0F }),
	};
	const iggy::DraftCompiledDoor2D doorA = Door("draft:door_a", 1, { -1.0F, 0.0F });
	const iggy::DraftCompiledDoor2D doorB = Door("draft:door_b", 2, { 1.0F, 0.0F });
	const iggy::DraftWallDoorAttach2DResult attachments =
		Attachments({
			Attachment(3, 0, doorA, walls[0]),
			Attachment(4, 0, doorB, walls[0]),
		});

	const iggy::DraftWallDoorMorphPlan2DResult result =
		iggy::DraftWallDoorMorphPlanner2D {}.plan(walls, attachments);

	Expect(result.wallPlans.size() == 1, "multi-door morph plan should include one wall");
	if (result.wallPlans.size() == 1) {
		Expect(result.wallPlans[0].doors.size() == 2, "wall should group both doors");
		if (result.wallPlans[0].doors.size() == 2) {
			Expect(result.wallPlans[0].doors[0].doorIndex == 3, "first grouped door should preserve attachment order");
			Expect(result.wallPlans[0].doors[1].doorIndex == 4, "second grouped door should preserve attachment order");
			ExpectDoor(result.wallPlans[0].doors[0].door, doorA, "first grouped door should copy payload");
			ExpectDoor(result.wallPlans[0].doors[1].door, doorB, "second grouped door should copy payload");
		}
	}
}

void TestMultipleWallsPreserveWallOrderAndGroupDoors()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall_a", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
		Wall("draft:wall_b", 1, { 10.0F, 0.0F }, { 4.0F, 4.0F }),
		Wall("draft:wall_c", 2, { 20.0F, 0.0F }, { 4.0F, 4.0F }),
	};
	const iggy::DraftCompiledDoor2D doorB = Door("draft:door_b", 3, { 10.0F, 0.0F });
	const iggy::DraftCompiledDoor2D doorA = Door("draft:door_a", 4, { 0.0F, 0.0F });
	const iggy::DraftWallDoorAttach2DResult attachments =
		Attachments({
			Attachment(8, 1, doorB, walls[1]),
			Attachment(9, 0, doorA, walls[0]),
		});

	const iggy::DraftWallDoorMorphPlan2DResult result =
		iggy::DraftWallDoorMorphPlanner2D {}.plan(walls, attachments);

	Expect(result.wallPlans.size() == 3, "morph plan should include all walls");
	if (result.wallPlans.size() == 3) {
		Expect(result.wallPlans[0].wall.sourceSymbolId == Id("draft:wall_a"), "first wall plan should preserve wall order");
		Expect(result.wallPlans[1].wall.sourceSymbolId == Id("draft:wall_b"), "second wall plan should preserve wall order");
		Expect(result.wallPlans[2].wall.sourceSymbolId == Id("draft:wall_c"), "third wall plan should preserve wall order");
		Expect(result.wallPlans[0].doors.size() == 1, "first wall should receive its door");
		Expect(result.wallPlans[1].doors.size() == 1, "second wall should receive its door");
		Expect(result.wallPlans[2].doors.empty(), "third wall should remain no-cut");
		if (result.wallPlans[0].doors.size() == 1)
			Expect(result.wallPlans[0].doors[0].doorIndex == 9, "first wall door should preserve original door index");
		if (result.wallPlans[1].doors.size() == 1)
			Expect(result.wallPlans[1].doors[0].doorIndex == 8, "second wall door should preserve original door index");
	}
}

void TestOutOfRangeAttachmentWallIndexReportsIssue()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
	};
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 0.0F, 0.0F });
	const iggy::DraftWallDoorAttach2DResult attachments =
		Attachments({ Attachment(6, 5, door, Wall("draft:stale_wall", 2, { 10.0F, 0.0F }, { 4.0F, 4.0F })) });

	const iggy::DraftWallDoorMorphPlan2DResult result =
		iggy::DraftWallDoorMorphPlanner2D {}.plan(walls, attachments);

	Expect(result.wallPlans.size() == 1, "out-of-range attachment should still preserve input wall plan");
	Expect(result.wallPlans.size() == 1 && result.wallPlans[0].doors.empty(), "out-of-range attachment should not be grouped");
	Expect(result.issues.size() == 1, "out-of-range attachment should produce issue");
	Expect(result.hasIssues(), "out-of-range attachment should make result report issues");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftWallDoorMorphPlan2DIssueCode::AttachmentWallIndexOutOfRange, "out-of-range issue should use AttachmentWallIndexOutOfRange");
		Expect(result.issues[0].attachmentIndex == 0, "out-of-range issue should preserve attachment index");
		Expect(result.issues[0].wallIndex == 5, "out-of-range issue should preserve requested wall index");
		Expect(result.issues[0].doorIndex == 6, "out-of-range issue should preserve door index");
		ExpectDoor(result.issues[0].door, door, "out-of-range issue should copy door payload");
	}
}

void TestDoorLargerThanWallReportsIssueAndPreservesDoor()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 0, { 0.0F, 0.0F }, { 2.0F, 4.0F }),
	};
	const iggy::DraftCompiledDoor2D door = Door("draft:oversized_door", 1, { 0.0F, 0.0F }, { 3.0F, 2.0F });
	const iggy::DraftWallDoorAttach2DResult attachments =
		Attachments({ Attachment(2, 0, door, walls[0]) });

	const iggy::DraftWallDoorMorphPlan2DResult result =
		iggy::DraftWallDoorMorphPlanner2D {}.plan(walls, attachments);

	Expect(result.wallPlans.size() == 1, "oversized door morph plan should preserve wall plan");
	Expect(result.wallPlans.size() == 1 && result.wallPlans[0].doors.size() == 1, "oversized door should remain inspectable in wall plan");
	Expect(result.issues.size() == 1, "oversized door should report issue");
	if (result.wallPlans.size() == 1 && result.wallPlans[0].doors.size() == 1)
		ExpectDoor(result.wallPlans[0].doors[0].door, door, "oversized door should be preserved in plan");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftWallDoorMorphPlan2DIssueCode::DoorLargerThanWall, "oversized door issue should use DoorLargerThanWall");
		Expect(result.issues[0].attachmentIndex == 0, "oversized door issue should preserve attachment index");
		Expect(result.issues[0].wallIndex == 0, "oversized door issue should preserve wall index");
		Expect(result.issues[0].doorIndex == 2, "oversized door issue should preserve door index");
		ExpectDoor(result.issues[0].door, door, "oversized door issue should copy door payload");
		ExpectWall(result.issues[0].wall, walls[0], "oversized door issue should copy wall payload");
	}
}

void TestInputWallsAndAttachmentsAreNotMutated()
{
	std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
	};
	const iggy::DraftCompiledDoor2D door = Door("draft:door", 1, { 0.0F, 0.0F });
	iggy::DraftWallDoorAttach2DResult attachments =
		Attachments({ Attachment(2, 0, door, walls[0]) });
	const std::vector<iggy::DraftCompiledWall2D> beforeWalls = walls;
	const std::vector<iggy::DraftWallDoorAttachment2D> beforeAttachments = attachments.attachments;

	const iggy::DraftWallDoorMorphPlan2DResult result =
		iggy::DraftWallDoorMorphPlanner2D {}.plan(walls, attachments);

	Expect(result.wallPlans.size() == 1 && result.wallPlans[0].doors.size() == 1, "draft wall door morph immutability setup should exercise grouping");
	Expect(SameWalls(walls, beforeWalls), "draft wall door morph planner should not mutate walls");
	Expect(SameAttachments(attachments.attachments, beforeAttachments), "draft wall door morph planner should not mutate attachments");
}

} // namespace

int main()
{
	TestEmptyWallsAndAttachmentsProduceEmptyResult();
	TestWallWithNoDoorsAppearsAsNoCutPlan();
	TestSingleDoorAttachmentGroupsUnderCorrectWall();
	TestMultipleDoorsOnOneWallPreserveAttachmentOrder();
	TestMultipleWallsPreserveWallOrderAndGroupDoors();
	TestOutOfRangeAttachmentWallIndexReportsIssue();
	TestDoorLargerThanWallReportsIssueAndPreservesDoor();
	TestInputWallsAndAttachmentsAreNotMutated();

	return Failures;
}
