#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "modules/npc_ai/NpcNavigationController.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

bool NearVec(iggy::Vec2 actual, iggy::Vec2 expected)
{
	return Near(actual.x, expected.x) && Near(actual.y, expected.y);
}

iggy::LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map;
	map.height = static_cast<int>(rows.size());
	map.width = rows.empty() ? 0 : static_cast<int>(rows.front().size());
	for (std::string_view row : rows) {
		for (char cell : row)
			map.tiles.push_back({ cell != '#' });
	}
	return map;
}

iggy::npc_ai::NpcMovementPlan MoveTo(iggy::Vec2 destination)
{
	return { iggy::npc_ai::NpcMovementPlanType::MoveTo, destination };
}

void TestNoMovementPlan()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::Vec2 current { 1.5F, 0.5F };
	const iggy::npc_ai::NpcNavigationResult result = iggy::npc_ai::NpcNavigationController {}.step(map, current, {}, {}, 1.0F);

	Expect(result.status == iggy::npc_ai::NpcNavigationStatus::NoMovement, "no movement plan should return NoMovement");
	Expect(result.nextPosition == current, "no movement plan should preserve current position");
	Expect(result.requestStatus == iggy::navigation::NavigationRequestStatus::None, "no movement plan should preserve request status");
}

void TestBlockedDestinationRejected()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
	});
	const iggy::Vec2 current { 0.5F, 1.5F };
	const iggy::npc_ai::NpcNavigationResult result = iggy::npc_ai::NpcNavigationController {}.step(map, current, MoveTo({ 1.5F, 1.5F }), {}, 1.0F);

	Expect(result.status == iggy::npc_ai::NpcNavigationStatus::RequestRejected, "blocked destination should reject request");
	Expect(result.nextPosition == current, "blocked destination should preserve current position");
	Expect(result.requestStatus == iggy::navigation::NavigationRequestStatus::DestinationBlocked, "blocked destination should report request rejection reason");
}

void TestUnreachableDestinationPathNotFound()
{
	const iggy::LevelTileMap map = MapFromRows({
		"..#..",
		"..#..",
		"..#..",
	});
	const iggy::Vec2 current { 0.5F, 1.5F };
	const iggy::npc_ai::NpcNavigationResult result = iggy::npc_ai::NpcNavigationController {}.step(map, current, MoveTo({ 4.5F, 1.5F }), {}, 1.0F);

	Expect(result.status == iggy::npc_ai::NpcNavigationStatus::PathNotFound, "unreachable destination should report PathNotFound");
	Expect(result.nextPosition == current, "unreachable destination should preserve current position");
	Expect(result.pathStatus == iggy::navigation::NavigationPathStatus::NoPath, "unreachable destination should preserve path status");
}

void TestSameTileArrives()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::Vec2 current { 1.5F, 0.5F };
	const iggy::npc_ai::NpcNavigationResult result = iggy::npc_ai::NpcNavigationController {}.step(map, current, MoveTo({ 1.5F, 0.5F }), {}, 1.0F);

	Expect(result.status == iggy::npc_ai::NpcNavigationStatus::Arrived, "same-tile destination should arrive");
	Expect(result.nextPosition == current, "same-tile destination at current position should preserve position");
	Expect(result.followState.completed, "same-tile destination should complete follow state");
}

void TestSmallStepMovesTowardDestination()
{
	const iggy::LevelTileMap map = MapFromRows({
		"....",
		"....",
	});
	const iggy::npc_ai::NpcNavigationResult result = iggy::npc_ai::NpcNavigationController {}.step(map, { 0.5F, 1.5F }, MoveTo({ 3.5F, 1.5F }), {}, 0.25F);

	Expect(result.status == iggy::npc_ai::NpcNavigationStatus::Moving, "small step should keep moving");
	Expect(NearVec(result.nextPosition, { 0.75F, 1.5F }), "small step should move toward destination");
	Expect(result.followState.waypointIndex == 1, "small step should target next waypoint after start");
	Expect(!result.followState.completed, "small step should not complete");
}

void TestLargeStepArrives()
{
	const iggy::LevelTileMap map = MapFromRows({
		"....",
		"....",
	});
	const iggy::npc_ai::NpcNavigationResult result = iggy::npc_ai::NpcNavigationController {}.step(map, { 0.5F, 1.5F }, MoveTo({ 3.5F, 1.5F }), {}, 8.0F);

	Expect(result.status == iggy::npc_ai::NpcNavigationStatus::Arrived, "large step should arrive");
	Expect(result.nextPosition == iggy::Vec2 { 3.5F, 1.5F }, "large step should land at destination");
	Expect(result.followState.completed, "large step should complete follow state");
}

void TestOutOfBoundsDestinationPreservesCurrentPosition()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::Vec2 current { 1.5F, 0.5F };
	const iggy::npc_ai::NpcNavigationResult result = iggy::npc_ai::NpcNavigationController {}.step(map, current, MoveTo({ 4.5F, 0.5F }), {}, 1.0F);

	Expect(result.status == iggy::npc_ai::NpcNavigationStatus::RequestRejected, "out-of-bounds destination should reject request");
	Expect(result.nextPosition == current, "out-of-bounds destination should preserve current position");
	Expect(result.requestStatus == iggy::navigation::NavigationRequestStatus::DestinationOutOfBounds, "out-of-bounds destination should report request reason");
}

} // namespace

int main()
{
	TestNoMovementPlan();
	TestBlockedDestinationRejected();
	TestUnreachableDestinationPathNotFound();
	TestSameTileArrives();
	TestSmallStepMovesTowardDestination();
	TestLargeStepArrives();
	TestOutOfBoundsDestinationPreservesCurrentPosition();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
