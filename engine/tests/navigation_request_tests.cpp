#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include "servers/navigation/NavigationGridValidator.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
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

void TestNoMovementProducesNoRequest()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::npc_ai::NpcMovementPlan plan;
	const iggy::navigation::NavigationRequest request = iggy::navigation::NavigationGridValidator {}.validate(map, plan);

	Expect(request.status == iggy::navigation::NavigationRequestStatus::None, "no movement plan should produce no navigation request");
	Expect(!request.accepted(), "no movement request should not be accepted");
	Expect(!request.destination.has_value(), "no movement request should not keep a destination");
}

void TestMissingDestinationIsRejected()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::npc_ai::NpcMovementPlan plan { iggy::npc_ai::NpcMovementPlanType::MoveTo, {} };
	const iggy::navigation::NavigationRequest request = iggy::navigation::NavigationGridValidator {}.validate(map, plan);

	Expect(request.status == iggy::navigation::NavigationRequestStatus::MissingDestination, "move plan without destination should be rejected");
	Expect(!request.accepted(), "missing destination request should not be accepted");
}

void TestOutOfBoundsDestinationIsRejected()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		"...",
	});
	const iggy::npc_ai::NpcMovementPlan plan { iggy::npc_ai::NpcMovementPlanType::MoveTo, iggy::Vec2 { 3.5F, 1.5F } };
	const iggy::navigation::NavigationRequest request = iggy::navigation::NavigationGridValidator {}.validate(map, plan);

	Expect(request.status == iggy::navigation::NavigationRequestStatus::DestinationOutOfBounds, "out-of-bounds destination should be rejected");
	Expect(!request.accepted(), "out-of-bounds destination should not be accepted");
	Expect(request.destinationTileX == 3 && request.destinationTileY == 1, "out-of-bounds request should report destination tile");
}

void TestBlockedDestinationIsRejected()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
	});
	const iggy::npc_ai::NpcMovementPlan plan { iggy::npc_ai::NpcMovementPlanType::MoveTo, iggy::Vec2 { 1.5F, 1.5F } };
	const iggy::navigation::NavigationRequest request = iggy::navigation::NavigationGridValidator {}.validate(map, plan);

	Expect(request.status == iggy::navigation::NavigationRequestStatus::DestinationBlocked, "blocked destination should be rejected");
	Expect(!request.accepted(), "blocked destination should not be accepted");
	Expect(request.destination.has_value() && *request.destination == iggy::Vec2 { 1.5F, 1.5F }, "blocked request should preserve destination");
	Expect(request.destinationTileX == 1 && request.destinationTileY == 1, "blocked request should report destination tile");
}

void TestWalkableDestinationIsAccepted()
{
	const iggy::LevelTileMap map = MapFromRows({
		"...",
		".#.",
	});
	const iggy::npc_ai::NpcMovementPlan plan { iggy::npc_ai::NpcMovementPlanType::MoveTo, iggy::Vec2 { 2.5F, 1.5F } };
	const iggy::navigation::NavigationRequest request = iggy::navigation::NavigationGridValidator {}.validate(map, plan);

	Expect(request.status == iggy::navigation::NavigationRequestStatus::Accepted, "walkable destination should be accepted");
	Expect(request.accepted(), "accepted destination should report accepted");
	Expect(request.destination.has_value() && *request.destination == iggy::Vec2 { 2.5F, 1.5F }, "accepted request should preserve destination");
	Expect(request.destinationTileX == 2 && request.destinationTileY == 1, "accepted request should report destination tile");
}

} // namespace

int main()
{
	TestNoMovementProducesNoRequest();
	TestMissingDestinationIsRejected();
	TestOutOfBoundsDestinationIsRejected();
	TestBlockedDestinationIsRejected();
	TestWalkableDestinationIsAccepted();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
