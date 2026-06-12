#include "modules/npc_ai/AwarenessSensor.hpp"

#include "modules/line_of_sight/LineOfSight.hpp"

namespace iggy::npc_ai {

namespace {

AwarenessEvent BuildEvent(AwarenessEventType type, const AwarenessState &state, line_of_sight::TileCoord playerTile)
{
	return { type, state.playerVisible, state.alerted, playerTile, state.lastSeenTile };
}

} // namespace

AwarenessSensor::AwarenessSensor(AwarenessSensorConfig config)
    : config_(config)
{
}

AwarenessEvent AwarenessSensor::observe(const LevelTileMap &map, Vec2 npcPosition, Vec2 playerPosition, AwarenessState &state) const
{
	const bool wasVisible = state.playerVisible;
	const bool wasAlerted = state.alerted;
	const float distance = (playerPosition - npcPosition).length();
	line_of_sight::LineOfSightTrace trace;

	if (distance <= config_.visionRange)
		trace = line_of_sight::Trace(map, npcPosition, playerPosition);

	if (distance <= config_.visionRange && trace.visible()) {
		state.playerVisible = true;
		state.alerted = true;
		state.alertTicksRemaining = config_.alertMemoryTicks;
		state.lastSeenTile = trace.endTile;
		state.lastSeenPosition = playerPosition;
		return BuildEvent(AwarenessEventType::PlayerSeen, state, trace.endTile);
	}

	state.playerVisible = false;
	if (state.alertTicksRemaining > 0) {
		--state.alertTicksRemaining;
		state.alerted = true;
	} else {
		state.alerted = false;
	}

	if (wasVisible)
		return BuildEvent(AwarenessEventType::PlayerLost, state, trace.endTile);
	if (wasAlerted && !state.alerted)
		return BuildEvent(AwarenessEventType::AlertExpired, state, trace.endTile);
	return BuildEvent(AwarenessEventType::None, state, trace.endTile);
}

} // namespace iggy::npc_ai
