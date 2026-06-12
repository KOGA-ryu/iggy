#include "scene/level/LevelTileMap.hpp"

namespace iggy {

bool LevelTileMap::contains(int x, int y) const
{
	return x >= 0 && y >= 0 && x < width && y < height;
}

const LevelTile *LevelTileMap::tileAt(int x, int y) const
{
	if (!contains(x, y))
		return nullptr;
	const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
	if (index >= tiles.size())
		return nullptr;
	return &tiles[index];
}

LevelTileMapBuildResult LevelTileMapBuilder::build(const LevelBlueprint &blueprint) const
{
	LevelTileMapBuildResult result;
	result.validation = LevelBlueprintValidator {}.validate(blueprint);
	if (!result.validation.valid)
		return result;

	result.built = true;
	result.tileMap.id = blueprint.id;
	result.tileMap.width = blueprint.bounds.width;
	result.tileMap.height = blueprint.bounds.height;
	result.tileMap.playerStart = blueprint.playerStarts.front();
	result.tileMap.tiles.reserve(static_cast<std::size_t>(blueprint.bounds.width) * static_cast<std::size_t>(blueprint.bounds.height));

	if (blueprint.tiles.empty()) {
		result.tileMap.tiles.resize(static_cast<std::size_t>(blueprint.bounds.width) * static_cast<std::size_t>(blueprint.bounds.height));
	} else {
		for (const BlueprintTile &tile : blueprint.tiles)
			result.tileMap.tiles.push_back({ tile.walkable });
	}

	result.tileMap.entitySpawns.reserve(blueprint.entitySpawns.size());
	for (const BlueprintEntitySpawn &spawn : blueprint.entitySpawns)
		result.tileMap.entitySpawns.push_back({ spawn.type, spawn.x, spawn.y, spawn.id });

	return result;
}

} // namespace iggy
