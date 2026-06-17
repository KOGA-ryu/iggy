#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace iggy::test {

enum class CanonicalAuthoringFixtureCategory {
	Movement,
	PlayerMovement,
	Interaction,
	Pickup,
	Mixed,
	Collision,
};

enum class CanonicalAuthoringFixtureExpectedResult {
	Success,
	Failure,
};

enum class CanonicalAuthoringFixtureMode : std::uint32_t {
	None = 0,
	Run = 1U << 0U,
	Trace = 1U << 1U,
	Lint = 1U << 2U,
	Check = 1U << 3U,
};

inline CanonicalAuthoringFixtureMode operator|(
	CanonicalAuthoringFixtureMode left,
	CanonicalAuthoringFixtureMode right)
{
	return static_cast<CanonicalAuthoringFixtureMode>(
		static_cast<std::uint32_t>(left) |
		static_cast<std::uint32_t>(right));
}

inline bool HasFixtureMode(
	CanonicalAuthoringFixtureMode modes,
	CanonicalAuthoringFixtureMode mode)
{
	return (static_cast<std::uint32_t>(modes) &
		static_cast<std::uint32_t>(mode)) != 0U;
}

struct CanonicalAuthoringFixture {
	const char *name = "";
	const char *label = "";
	CanonicalAuthoringFixtureCategory category =
		CanonicalAuthoringFixtureCategory::Movement;
	CanonicalAuthoringFixtureExpectedResult expectedResult =
		CanonicalAuthoringFixtureExpectedResult::Success;
	CanonicalAuthoringFixtureMode modes =
		CanonicalAuthoringFixtureMode::None;
	int frameCount = 0;
	int acceptedCommandCount = 0;
	int pickedUpCount = 0;
	bool interactionChanged = false;
	int npcMovedCount = 0;
	int npcBlockedMovementCount = 0;
	std::vector<std::string> finalRows;
};

inline CanonicalAuthoringFixtureMode StandardSuccessModes()
{
	return CanonicalAuthoringFixtureMode::Run |
		CanonicalAuthoringFixtureMode::Trace |
		CanonicalAuthoringFixtureMode::Lint;
}

inline CanonicalAuthoringFixtureMode ExpectedSuccessModes()
{
	return StandardSuccessModes() | CanonicalAuthoringFixtureMode::Check;
}

inline const std::vector<CanonicalAuthoringFixture> &CanonicalAuthoringFixtures()
{
	static const std::vector<CanonicalAuthoringFixture> fixtures {
		{
			"moving_guard_room.toml",
			"movement only",
			CanonicalAuthoringFixtureCategory::Movement,
			CanonicalAuthoringFixtureExpectedResult::Success,
			StandardSuccessModes(),
			1,
			0,
			0,
			false,
			1,
			0,
			{ "#######", "#.A..@#", "#.....#", "#######" },
		},
		{
			"multi_frame_guard_room.toml",
			"multi-frame movement",
			CanonicalAuthoringFixtureCategory::Movement,
			CanonicalAuthoringFixtureExpectedResult::Success,
			StandardSuccessModes(),
			2,
			0,
			0,
			false,
			2,
			0,
			{ "#######", "#..A.@#", "#.....#", "#######" },
		},
		{
			"player_and_guard_room.toml",
			"player and NPC movement",
			CanonicalAuthoringFixtureCategory::PlayerMovement,
			CanonicalAuthoringFixtureExpectedResult::Success,
			StandardSuccessModes(),
			1,
			1,
			0,
			false,
			1,
			0,
			{ "#######", "#.A.@.#", "#.....#", "#######" },
		},
		{
			"player_interacts_guard_room.toml",
			"interaction toggle",
			CanonicalAuthoringFixtureCategory::Interaction,
			CanonicalAuthoringFixtureExpectedResult::Success,
			StandardSuccessModes(),
			1,
			1,
			0,
			true,
			0,
			0,
			{ "#######", "#A..@.#", "#.....#", "#######" },
		},
		{
			"player_picks_up_item_room.toml",
			"pickup",
			CanonicalAuthoringFixtureCategory::Pickup,
			CanonicalAuthoringFixtureExpectedResult::Success,
			StandardSuccessModes(),
			2,
			2,
			1,
			false,
			0,
			0,
			{ "#######", "#A.@..#", "#.....#", "#######" },
		},
		{
			"mixed_mini_scenario.toml",
			"mixed mini scenario",
			CanonicalAuthoringFixtureCategory::Mixed,
			CanonicalAuthoringFixtureExpectedResult::Success,
			StandardSuccessModes(),
			3,
			3,
			1,
			true,
			1,
			0,
			{ "#########", "#.A@....#", "#.......#", "#########" },
		},
		{
			"mixed_progression_room.toml",
			"mixed progression room",
			CanonicalAuthoringFixtureCategory::Mixed,
			CanonicalAuthoringFixtureExpectedResult::Success,
			ExpectedSuccessModes(),
			4,
			3,
			1,
			true,
			1,
			0,
			{ "##########", "#.A@.....#", "#........#", "##########" },
		},
		{
			"locked_door_key_room.toml",
			"locked door with key",
			CanonicalAuthoringFixtureCategory::Interaction,
			CanonicalAuthoringFixtureExpectedResult::Success,
			ExpectedSuccessModes(),
			2,
			2,
			1,
			true,
			0,
			0,
			{ "#######", "#@....#", "#.....#", "#######" },
		},
		{
			"locked_door_without_key_room.toml",
			"locked door without key",
			CanonicalAuthoringFixtureCategory::Interaction,
			CanonicalAuthoringFixtureExpectedResult::Success,
			ExpectedSuccessModes(),
			1,
			1,
			0,
			false,
			0,
			0,
			{ "#######", "#@....#", "#.....#", "#######" },
		},
		{
			"npc_blocked_guard_room.toml",
			"NPC blocked movement",
			CanonicalAuthoringFixtureCategory::Collision,
			CanonicalAuthoringFixtureExpectedResult::Success,
			ExpectedSuccessModes(),
			1,
			0,
			0,
			false,
			0,
			1,
			{ "#######", "#AB.@.#", "#.....#", "#######" },
		},
		{
			"npc_reservation_guard_room.toml",
			"NPC shared target movement",
			CanonicalAuthoringFixtureCategory::Collision,
			CanonicalAuthoringFixtureExpectedResult::Success,
			ExpectedSuccessModes(),
			1,
			0,
			0,
			false,
			2,
			0,
			{ "########", "#.B.@..#", "#......#", "########" },
		},
	};
	return fixtures;
}

} // namespace iggy::test
