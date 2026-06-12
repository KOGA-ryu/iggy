#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "modules/blueprint/LevelBlueprintValidator.hpp"
#include "scene/level/LevelBlueprint.hpp"

namespace iggy {

struct LevelTile {
	bool walkable = true;
};

struct LevelEntitySpawn {
	ResourceId type;
	int x = 0;
	int y = 0;
	ResourceId id;
};

struct LevelTileMap {
	ResourceId id;
	int width = 0;
	int height = 0;
	std::vector<LevelTile> tiles;
	std::vector<LevelEntitySpawn> entitySpawns;
	PlayerStart playerStart;

	[[nodiscard]] bool contains(int x, int y) const;
	[[nodiscard]] const LevelTile *tileAt(int x, int y) const;
};

struct LevelTileMapBuildResult {
	bool built = false;
	LevelTileMap tileMap;
	LevelBlueprintValidationReport validation;
};

class LevelTileMapBuilder {
public:
	[[nodiscard]] LevelTileMapBuildResult build(const LevelBlueprint &blueprint) const;
};

} // namespace iggy
