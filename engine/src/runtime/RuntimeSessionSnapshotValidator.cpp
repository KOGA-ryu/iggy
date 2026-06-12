#include "runtime/RuntimeSessionSnapshotValidator.hpp"

#include "scene/level/LevelGridQuery.hpp"
#include "scene/player/PlayerAgentState.hpp"

namespace iggy::runtime {
namespace {

[[nodiscard]] bool hasPositiveMapDimensions(const LevelTileMap &map)
{
	return map.width > 0 && map.height > 0;
}

[[nodiscard]] bool pointInsideMap(const LevelTileMap &map, Vec2 position)
{
	return containsTile(map, tileForPoint(position));
}

[[nodiscard]] bool npcInsideMap(const LevelTileMap &map, const npc_ai::NpcAgentEntry &npc)
{
	return pointInsideMap(map, npc.state.position) && containsTile(map, npc.state.homeTile);
}

} // namespace

RuntimeSessionSnapshotValidationResult RuntimeSessionSnapshotValidator::validate(const RuntimeSessionSnapshot &snapshot) const
{
	RuntimeSessionSnapshotValidationResult result;
	const LevelTileMap &map = snapshot.level.map;
	if (!hasPositiveMapDimensions(map)) {
		result.issues.push_back({ RuntimeSessionSnapshotIssueCode::InvalidMapDimensions, 0 });
		result.valid = false;
		return result;
	}

	if (snapshot.hasPlayer && !pointInsideMap(map, snapshot.player.position))
		result.issues.push_back({ RuntimeSessionSnapshotIssueCode::PlayerOutOfBounds, 0 });

	for (std::size_t index = 0; index < snapshot.level.npcAgents.size(); ++index) {
		if (!npcInsideMap(map, snapshot.level.npcAgents[index]))
			result.issues.push_back({ RuntimeSessionSnapshotIssueCode::NpcOutOfBounds, index });
	}

	result.valid = result.issues.empty();
	return result;
}

} // namespace iggy::runtime
