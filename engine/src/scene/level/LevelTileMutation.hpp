#pragma once

#include <cstddef>
#include <vector>

#include "scene/level/LevelTileMap.hpp"
#include "scene/level/TileCoord.hpp"

namespace iggy {

struct LevelTileEdit {
	TileCoord tile;
	bool walkable = true;
};

enum class LevelTileEditIssueCode {
	OutOfBounds,
	MissingTileStorage,
};

struct LevelTileEditIssue {
	LevelTileEditIssueCode code = LevelTileEditIssueCode::OutOfBounds;
	std::size_t editIndex = 0;
	LevelTileEdit edit;
};

struct LevelTileMutationResult {
	bool mutated = false;
	LevelTileMap map;
	std::vector<TileCoord> changedTiles;
	std::vector<LevelTileEditIssue> issues;
};

class LevelTileMutation {
public:
	[[nodiscard]] LevelTileMutationResult apply(const LevelTileMap &map, const std::vector<LevelTileEdit> &edits) const;
};

} // namespace iggy
