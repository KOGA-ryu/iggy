#include "scene/level/LevelCollisionWorldBuilder.hpp"

#include <vector>

#include "scene/level/LevelGridQuery.hpp"
#include "servers/physics2d/CollisionShape2D.hpp"

namespace iggy {

namespace {

struct GeneratedCollisionTile {
	TileCoord tile;
	std::size_t tileIndex = 0;
};

} // namespace

LevelCollisionWorldBuildResult LevelCollisionWorldBuilder::build(const LevelTileMap &map) const
{
	LevelCollisionWorldBuildResult result;
	std::vector<physics2d::CollisionObject2D> objects;
	std::vector<GeneratedCollisionTile> generatedTiles;

	if (map.width <= 0 || map.height <= 0) {
		const physics2d::CollisionWorldBuildResult worldBuild = physics2d::CollisionWorld2DBuilder {}.build({});
		result.built = worldBuild.built;
		result.world = worldBuild.world;
		return result;
	}

	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			const TileCoord tile { x, y };
			const LevelTile *tileData = tileAt(map, tile);
			if (tileData == nullptr || tileData->walkable)
				continue;

			objects.push_back({ {}, physics2d::makeAabbShape(tileBounds(tile)), true });
			generatedTiles.push_back({ tile, tileIndex(map, tile) });
		}
	}

	const physics2d::CollisionWorldBuildResult worldBuild = physics2d::CollisionWorld2DBuilder {}.build(objects);
	if (worldBuild.built) {
		result.built = true;
		result.world = worldBuild.world;
		return result;
	}

	for (const physics2d::CollisionWorldBuildIssue &issue : worldBuild.issues) {
		LevelCollisionWorldBuildIssue levelIssue;
		if (issue.index < generatedTiles.size()) {
			levelIssue.tile = generatedTiles[issue.index].tile;
			levelIssue.tileIndex = generatedTiles[issue.index].tileIndex;
		}
		result.issues.push_back(levelIssue);
	}

	return result;
}

} // namespace iggy
