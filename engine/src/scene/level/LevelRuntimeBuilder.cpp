#include "scene/level/LevelRuntimeBuilder.hpp"

namespace iggy {

LevelRuntimeBuildResult LevelRuntimeBuilder::build(const LevelBlueprint &blueprint) const
{
	LevelRuntimeBuildResult result;

	const LevelTileMapBuildResult tileMapResult = LevelTileMapBuilder {}.build(blueprint);
	result.validation = tileMapResult.validation;
	if (!tileMapResult.built)
		return result;

	const npc_ai::NpcSpawnBuildResult spawnResult = npc_ai::NpcSpawnBuilder {}.build(blueprint);
	result.built = true;
	result.tileMap = tileMapResult.tileMap;
	result.npcAgents = spawnResult.agents;
	result.spawnIssues = spawnResult.issues;
	return result;
}

} // namespace iggy
