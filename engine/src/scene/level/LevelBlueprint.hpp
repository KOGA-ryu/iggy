#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy {

struct LevelBounds {
	int width = 0;
	int height = 0;

	[[nodiscard]] bool contains(int x, int y) const;
};

struct BlueprintTile {
	bool walkable = true;
};

struct BlueprintEntitySpawn {
	ResourceId type;
	int x = 0;
	int y = 0;
	ResourceId id;
};

struct PlayerStart {
	int x = 0;
	int y = 0;
};

struct LevelBlueprint {
	ResourceId id;
	LevelBounds bounds;
	std::vector<BlueprintTile> tiles;
	std::vector<BlueprintEntitySpawn> entitySpawns;
	std::vector<PlayerStart> playerStarts;
};

} // namespace iggy
