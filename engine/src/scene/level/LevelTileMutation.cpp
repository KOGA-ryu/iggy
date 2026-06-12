#include "scene/level/LevelTileMutation.hpp"

#include "scene/level/LevelGridQuery.hpp"

namespace iggy {

LevelTileMutationResult LevelTileMutation::apply(const LevelTileMap &map, const std::vector<LevelTileEdit> &edits) const
{
	LevelTileMutationResult result;
	result.map = map;

	for (std::size_t editIndex = 0; editIndex < edits.size(); ++editIndex) {
		const LevelTileEdit &edit = edits[editIndex];
		if (!containsTile(result.map, edit.tile)) {
			result.issues.push_back({ LevelTileEditIssueCode::OutOfBounds, editIndex, edit });
			continue;
		}

		const std::size_t index = tileIndex(result.map, edit.tile);
		if (index >= result.map.tiles.size()) {
			result.issues.push_back({ LevelTileEditIssueCode::MissingTileStorage, editIndex, edit });
			continue;
		}

		LevelTile &tile = result.map.tiles[index];
		if (tile.walkable == edit.walkable)
			continue;

		tile.walkable = edit.walkable;
		result.mutated = true;
		result.changedTiles.push_back(edit.tile);
	}

	return result;
}

} // namespace iggy
