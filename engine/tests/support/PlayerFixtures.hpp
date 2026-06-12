#pragma once

#include <string>
#include <string_view>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "TestHarness.hpp"

namespace iggy::test {

inline PlayerAgentState PlayerAgent(
	ResourceId id = ResourceId { "player:one" },
	Vec2 position = { 1.25F, 2.75F },
	TileCoord spawnTile = { 1, 2 },
	PlayerMovementStatus movementStatus = PlayerMovementStatus::Idle,
	PlayerFacing2D facing = PlayerFacing2D::South)
{
	PlayerAgentState player;
	player.id = id;
	player.position = position;
	player.spawnTile = spawnTile;
	player.movementStatus = movementStatus;
	player.facing = facing;
	return player;
}

inline void ExpectPlayerAgent(const PlayerAgentState &actual, const PlayerAgentState &expected, std::string_view context)
{
	Expect(actual.id == expected.id, std::string(context) + " should preserve player id");
	Expect(NearVec(actual.position, expected.position), std::string(context) + " should preserve player position");
	Expect(actual.spawnTile == expected.spawnTile, std::string(context) + " should preserve player spawn tile");
	Expect(actual.movementStatus == expected.movementStatus, std::string(context) + " should preserve player movement status");
	Expect(actual.facing == expected.facing, std::string(context) + " should preserve player facing");
}

} // namespace iggy::test
