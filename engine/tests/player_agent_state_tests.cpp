#include <cstdlib>

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId PlayerId { "player:one" };

void TestDefaultPlayerAgentState()
{
	const iggy::PlayerAgentState state;

	Expect(state.id.empty(), "default player state should have empty id");
	Expect(NearVec(state.position, { 0.0F, 0.0F }), "default player state should have zero position");
	Expect(state.spawnTile == iggy::TileCoord { -1, -1 }, "default player state should have default spawn tile");
	Expect(state.movementStatus == iggy::PlayerMovementStatus::Idle, "default player state should be idle");
	Expect(state.facing == iggy::PlayerFacing2D::None, "default player state should have no facing");
}

void TestExplicitStatePreservesFields()
{
	iggy::PlayerAgentState state;
	state.id = PlayerId;
	state.position = { 2.25F, 3.75F };
	state.spawnTile = { 2, 3 };
	state.movementStatus = iggy::PlayerMovementStatus::Moving;
	state.facing = iggy::PlayerFacing2D::East;

	Expect(state.id == PlayerId, "explicit player state should preserve id");
	Expect(NearVec(state.position, { 2.25F, 3.75F }), "explicit player state should preserve position");
	Expect(state.spawnTile == iggy::TileCoord { 2, 3 }, "explicit player state should preserve spawn tile");
	Expect(state.movementStatus == iggy::PlayerMovementStatus::Moving, "explicit player state should preserve movement status");
	Expect(state.facing == iggy::PlayerFacing2D::East, "explicit player state should preserve facing");
}

void TestPlayerTileUsesPositiveFloorSemantics()
{
	iggy::PlayerAgentState state;
	state.position = { 4.9F, 1.1F };

	Expect(iggy::playerTile(state) == iggy::TileCoord { 4, 1 }, "playerTile should floor positive world position");
}

void TestPlayerTileUsesNegativeFloorSemantics()
{
	iggy::PlayerAgentState state;
	state.position = { -0.25F, -1.1F };

	Expect(iggy::playerTile(state) == iggy::TileCoord { -1, -2 }, "playerTile should floor negative world position");
}

void TestEmptyIdAllowedAndPreserved()
{
	iggy::PlayerAgentState state;
	state.position = { 1.0F, 2.0F };

	Expect(state.id.empty(), "player state should allow empty id");
	Expect(NearVec(state.position, { 1.0F, 2.0F }), "player state with empty id should preserve other data");
}

void TestNegativeSpawnTileAllowedAndPreserved()
{
	iggy::PlayerAgentState state;
	state.spawnTile = { -4, -7 };

	Expect(state.spawnTile == iggy::TileCoord { -4, -7 }, "player state should allow negative spawn tile");
}

void TestMovementStatusAndFacingValuesCanBeStored()
{
	iggy::PlayerAgentState north;
	north.movementStatus = iggy::PlayerMovementStatus::Moving;
	north.facing = iggy::PlayerFacing2D::North;

	iggy::PlayerAgentState south;
	south.facing = iggy::PlayerFacing2D::South;

	iggy::PlayerAgentState east;
	east.facing = iggy::PlayerFacing2D::East;

	iggy::PlayerAgentState west;
	west.facing = iggy::PlayerFacing2D::West;

	Expect(north.movementStatus == iggy::PlayerMovementStatus::Moving && north.facing == iggy::PlayerFacing2D::North, "player state should store moving north");
	Expect(south.facing == iggy::PlayerFacing2D::South, "player state should store south facing");
	Expect(east.facing == iggy::PlayerFacing2D::East, "player state should store east facing");
	Expect(west.facing == iggy::PlayerFacing2D::West, "player state should store west facing");
}

void TestGameplayCommandConstructionDoesNotMutatePlayerState()
{
	iggy::PlayerAgentState state;
	state.id = PlayerId;
	state.position = { 6.0F, 7.0F };
	state.spawnTile = { 6, 7 };
	state.movementStatus = iggy::PlayerMovementStatus::Idle;
	state.facing = iggy::PlayerFacing2D::South;

	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(PlayerId, { 9.0F, 10.0F });

	Expect(iggy::runtime::valid(command), "gameplay command construction setup should validate");
	Expect(state.id == PlayerId, "constructing gameplay command should not mutate player id");
	Expect(NearVec(state.position, { 6.0F, 7.0F }), "constructing gameplay command should not mutate player position");
	Expect(state.spawnTile == iggy::TileCoord { 6, 7 }, "constructing gameplay command should not mutate player spawn tile");
	Expect(state.movementStatus == iggy::PlayerMovementStatus::Idle, "constructing gameplay command should not mutate movement status");
	Expect(state.facing == iggy::PlayerFacing2D::South, "constructing gameplay command should not mutate facing");
}

} // namespace

int main()
{
	TestDefaultPlayerAgentState();
	TestExplicitStatePreservesFields();
	TestPlayerTileUsesPositiveFloorSemantics();
	TestPlayerTileUsesNegativeFloorSemantics();
	TestEmptyIdAllowedAndPreserved();
	TestNegativeSpawnTileAllowedAndPreserved();
	TestMovementStatusAndFacingValuesCanBeStored();
	TestGameplayCommandConstructionDoesNotMutatePlayerState();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
