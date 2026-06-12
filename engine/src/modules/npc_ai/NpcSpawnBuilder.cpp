#include "modules/npc_ai/NpcSpawnBuilder.hpp"

namespace iggy::npc_ai {

namespace {

bool IsNpcSpawn(const BlueprintEntitySpawn &spawn)
{
	return spawn.type.namespaceName() == "npc" || spawn.type.namespaceName() == "enemy";
}

ResourceId AgentIdForSpawn(const BlueprintEntitySpawn &spawn)
{
	if (!spawn.id.empty())
		return spawn.id;
	return spawn.type;
}

Vec2 TileCenter(int x, int y)
{
	return { static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F };
}

void AddIssue(NpcSpawnBuildResult &result, NpcSpawnBuildIssueCode code, const BlueprintEntitySpawn &spawn)
{
	result.issues.push_back({ code, spawn.id, spawn.type, spawn.x, spawn.y });
}

} // namespace

NpcSpawnBuildResult NpcSpawnBuilder::build(const LevelBlueprint &blueprint) const
{
	NpcSpawnBuildResult result;

	for (const BlueprintEntitySpawn &spawn : blueprint.entitySpawns) {
		if (!IsNpcSpawn(spawn)) {
			AddIssue(result, NpcSpawnBuildIssueCode::NonNpcSpawnSkipped, spawn);
			continue;
		}
		if (!blueprint.bounds.contains(spawn.x, spawn.y)) {
			AddIssue(result, NpcSpawnBuildIssueCode::OutOfBoundsSpawnSkipped, spawn);
			continue;
		}

		NpcAgentState state;
		state.position = TileCenter(spawn.x, spawn.y);
		state.homeTile = { spawn.x, spawn.y };
		result.agents.push_back({ AgentIdForSpawn(spawn), state });
	}

	return result;
}

} // namespace iggy::npc_ai
