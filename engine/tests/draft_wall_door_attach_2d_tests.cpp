#include <cstdlib>
#include <vector>

#include "scene/draft/DraftWallDoorAttach2D.hpp"
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

bool SameDoors(const std::vector<iggy::DraftCompiledDoor2D> &actual, const std::vector<iggy::DraftCompiledDoor2D> &expected)
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
			|| actual[index].definitionId != expected[index].definitionId
			|| actual[index].swing != expected[index].swing) {
			return false;
		}
	}
	return true;
}

void TestEmptyWallsAndDoorsProduceEmptyResult()
{
	const iggy::DraftWallDoorAttach2DResult result = iggy::DraftWallDoorAttach2D {}.attach({}, {});

	Expect(result.attachments.empty(), "empty wall door attach result should have no attachments");
	Expect(result.issues.empty(), "empty wall door attach result should have no issues");
	Expect(!result.hasIssues(), "empty wall door attach result should report no issues");
}

void TestDoorInsideOneWallAttaches()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 4, { 10.0F, 10.0F }, { 8.0F, 2.0F }, 0.25F, "asset:wall", "definition:wall"),
	};
	const std::vector<iggy::DraftCompiledDoor2D> doors {
		Door("draft:door", 7, { 11.0F, 10.25F }, { 1.0F, 2.0F }, 0.5F, "asset:door", "definition:door"),
	};

	const iggy::DraftWallDoorAttach2DResult result = iggy::DraftWallDoorAttach2D {}.attach(walls, doors);

	Expect(result.attachments.size() == 1, "door inside one wall should attach");
	Expect(result.issues.empty(), "door inside one wall should produce no issue");
	if (result.attachments.size() == 1) {
		Expect(result.attachments[0].doorIndex == 0, "attachment should preserve door index");
		Expect(result.attachments[0].wallIndex == 0, "attachment should preserve wall index");
		ExpectDoor(result.attachments[0].door, doors[0], "attachment should copy door payload");
		ExpectWall(result.attachments[0].wall, walls[0], "attachment should copy wall payload");
	}
}

void TestDoorOutsideAllWallsReportsIssue()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
	};
	const std::vector<iggy::DraftCompiledDoor2D> doors {
		Door("draft:door", 1, { 5.0F, 0.0F }),
	};

	const iggy::DraftWallDoorAttach2DResult result = iggy::DraftWallDoorAttach2D {}.attach(walls, doors);

	Expect(result.attachments.empty(), "door outside all walls should not attach");
	Expect(result.issues.size() == 1, "door outside all walls should report issue");
	Expect(result.hasIssues(), "door outside all walls should make result report issues");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftWallDoorAttach2DIssueCode::NoContainingWall, "outside door issue should be NoContainingWall");
		Expect(result.issues[0].doorIndex == 0, "outside door issue should preserve door index");
		ExpectDoor(result.issues[0].door, doors[0], "outside door issue should copy door payload");
		Expect(result.issues[0].wallIndexes.empty(), "outside door issue should have no containing wall indexes");
		Expect(result.issues[0].walls.empty(), "outside door issue should have no containing walls");
	}
}

void TestDoorInsideOverlappingWallsReportsAmbiguousIssue()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall_a", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
		Wall("draft:wall_b", 1, { 1.0F, 0.0F }, { 4.0F, 4.0F }),
	};
	const std::vector<iggy::DraftCompiledDoor2D> doors {
		Door("draft:door", 2, { 0.5F, 0.0F }),
	};

	const iggy::DraftWallDoorAttach2DResult result = iggy::DraftWallDoorAttach2D {}.attach(walls, doors);

	Expect(result.attachments.empty(), "door inside overlapping walls should not attach");
	Expect(result.issues.size() == 1, "door inside overlapping walls should report issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::DraftWallDoorAttach2DIssueCode::AmbiguousContainingWall, "overlapping wall issue should be AmbiguousContainingWall");
		Expect(result.issues[0].wallIndexes.size() == 2, "ambiguous issue should preserve containing wall indexes");
		Expect(result.issues[0].walls.size() == 2, "ambiguous issue should copy containing walls");
		if (result.issues[0].wallIndexes.size() == 2) {
			Expect(result.issues[0].wallIndexes[0] == 0, "ambiguous issue should preserve first wall index");
			Expect(result.issues[0].wallIndexes[1] == 1, "ambiguous issue should preserve second wall index");
		}
		if (result.issues[0].walls.size() == 2) {
			ExpectWall(result.issues[0].walls[0], walls[0], "ambiguous issue should copy first wall payload");
			ExpectWall(result.issues[0].walls[1], walls[1], "ambiguous issue should copy second wall payload");
		}
	}
}

void TestBoundaryToleranceAttachesDeterministically()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 2.0F }),
	};
	const std::vector<iggy::DraftCompiledDoor2D> doors {
		Door("draft:door_on_boundary", 1, { 2.0F, 1.0F }),
		Door("draft:door_within_tolerance", 2, { 2.05F, 0.0F }),
		Door("draft:door_beyond_tolerance", 3, { 2.2F, 0.0F }),
	};
	const iggy::DraftWallDoorAttach2DConfig config { 0.1F };

	const iggy::DraftWallDoorAttach2DResult result = iggy::DraftWallDoorAttach2D {}.attach(walls, doors, config);

	Expect(result.attachments.size() == 2, "boundary and tolerance doors should attach");
	Expect(result.issues.size() == 1, "door beyond tolerance should report issue");
	if (result.attachments.size() == 2) {
		Expect(result.attachments[0].doorIndex == 0, "door exactly on boundary should attach first");
		Expect(result.attachments[1].doorIndex == 1, "door within tolerance should attach second");
	}
	if (result.issues.size() == 1) {
		Expect(result.issues[0].doorIndex == 2, "door beyond tolerance should preserve issue order");
		Expect(result.issues[0].code == iggy::DraftWallDoorAttach2DIssueCode::NoContainingWall, "door beyond tolerance should be outside");
	}
}

void TestMultipleDoorsPreserveDoorOrder()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall_a", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
		Wall("draft:wall_b", 1, { 10.0F, 0.0F }, { 4.0F, 4.0F }),
	};
	const std::vector<iggy::DraftCompiledDoor2D> doors {
		Door("draft:door_a", 0, { 0.0F, 0.0F }),
		Door("draft:door_outside", 1, { 20.0F, 0.0F }),
		Door("draft:door_b", 2, { 10.0F, 0.0F }),
	};

	const iggy::DraftWallDoorAttach2DResult result = iggy::DraftWallDoorAttach2D {}.attach(walls, doors);

	Expect(result.attachments.size() == 2, "multiple door attach should include two attachments");
	Expect(result.issues.size() == 1, "multiple door attach should include one issue");
	if (result.attachments.size() == 2) {
		Expect(result.attachments[0].doorIndex == 0, "first attachment should preserve first door order");
		Expect(result.attachments[1].doorIndex == 2, "second attachment should preserve third door order");
	}
	if (result.issues.size() == 1)
		Expect(result.issues[0].doorIndex == 1, "issue should preserve second door order");
}

void TestWallRotationIsPreservedButNotUsedForContainment()
{
	const std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:rotated_wall", 0, { 0.0F, 0.0F }, { 2.0F, 8.0F }, 1.5708F),
	};
	const std::vector<iggy::DraftCompiledDoor2D> doors {
		Door("draft:door", 1, { 0.75F, 3.0F }),
	};

	const iggy::DraftWallDoorAttach2DResult result = iggy::DraftWallDoorAttach2D {}.attach(walls, doors);

	Expect(result.attachments.size() == 1, "axis-aligned containment should attach door inside unrotated bounds");
	if (result.attachments.size() == 1) {
		Expect(result.attachments[0].wall.rotationRadians == walls[0].rotationRadians, "attachment should preserve wall rotation");
		ExpectWall(result.attachments[0].wall, walls[0], "attachment should copy rotated wall payload");
	}
}

void TestInputWallsAndDoorsAreNotMutated()
{
	std::vector<iggy::DraftCompiledWall2D> walls {
		Wall("draft:wall", 0, { 0.0F, 0.0F }, { 4.0F, 4.0F }),
	};
	std::vector<iggy::DraftCompiledDoor2D> doors {
		Door("draft:door", 0, { 0.0F, 0.0F }),
		Door("draft:outside", 1, { 10.0F, 0.0F }),
	};
	const std::vector<iggy::DraftCompiledWall2D> beforeWalls = walls;
	const std::vector<iggy::DraftCompiledDoor2D> beforeDoors = doors;

	const iggy::DraftWallDoorAttach2DResult result = iggy::DraftWallDoorAttach2D {}.attach(walls, doors);

	Expect(result.attachments.size() == 1 && result.issues.size() == 1, "draft wall door attach immutability setup should exercise success and issue paths");
	Expect(SameWalls(walls, beforeWalls), "draft wall door attach should not mutate walls");
	Expect(SameDoors(doors, beforeDoors), "draft wall door attach should not mutate doors");
}

} // namespace

int main()
{
	TestEmptyWallsAndDoorsProduceEmptyResult();
	TestDoorInsideOneWallAttaches();
	TestDoorOutsideAllWallsReportsIssue();
	TestDoorInsideOverlappingWallsReportsAmbiguousIssue();
	TestBoundaryToleranceAttachesDeterministically();
	TestMultipleDoorsPreserveDoorOrder();
	TestWallRotationIsPreservedButNotUsedForContainment();
	TestInputWallsAndDoorsAreNotMutated();

	return Failures;
}
