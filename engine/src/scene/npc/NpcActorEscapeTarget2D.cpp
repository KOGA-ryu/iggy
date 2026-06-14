#include "scene/npc/NpcActorEscapeTarget2D.hpp"

#include <cstddef>

#include "scene/level/LevelGridQuery.hpp"

namespace {

float DistanceSquared(iggy::Vec2 left, iggy::Vec2 right)
{
	const iggy::Vec2 delta = left - right;
	return delta.lengthSquared();
}

bool ValidMapQuery(const iggy::LevelTileMap &map)
{
	if (map.width <= 0 || map.height <= 0) {
		return false;
	}

	const std::size_t expectedTileCount =
		static_cast<std::size_t>(map.width) * static_cast<std::size_t>(map.height);
	return map.tiles.size() == expectedTileCount;
}

void PreserveIntentFacts(iggy::NpcActorEscapeTarget2D &result, const iggy::NpcActorMovementIntent2D &intent)
{
	result.intent = intent;
	result.npcId = intent.npcId;
	result.startPosition = intent.startPosition;
	result.threatPosition = intent.targetPosition;
	result.startTile = iggy::tileForPoint(intent.startPosition);
}

bool IsBetterCandidate(
	const iggy::NpcActorEscapeTargetCandidate2D &candidate,
	const iggy::NpcActorEscapeTargetCandidate2D &selected)
{
	if (candidate.threatDistanceSquared != selected.threatDistanceSquared) {
		return candidate.threatDistanceSquared > selected.threatDistanceSquared;
	}

	return candidate.actorTravelDistanceSquared < selected.actorTravelDistanceSquared;
}

} // namespace

namespace iggy {

bool NpcActorEscapeTarget2D::ready() const
{
	return status == NpcActorEscapeTarget2DStatus::Ready && hasEscapeTarget;
}

NpcActorEscapeTarget2D NpcActorEscapeTargetProjector2D::project(
	const NpcActorMovementIntent2D &intent,
	const LevelTileMap &map,
	const NpcActorEscapeTarget2DConfig &config) const
{
	NpcActorEscapeTarget2D result;
	PreserveIntentFacts(result, intent);

	if (!ValidMapQuery(map)) {
		result.status = NpcActorEscapeTarget2DStatus::InvalidMapQuery;
		return result;
	}

	if (!intent.ready() || !intent.requestsMovement) {
		result.status = NpcActorEscapeTarget2DStatus::NoMovementIntent;
		return result;
	}

	if (intent.type != NpcActorMovementIntent2DType::MoveAwayFrom) {
		result.status = NpcActorEscapeTarget2DStatus::NotMoveAwayFrom;
		return result;
	}

	const float threatAtActorToleranceSquared = config.threatAtActorTolerance * config.threatAtActorTolerance;
	const float currentThreatDistanceSquared = DistanceSquared(intent.startPosition, intent.targetPosition);
	if (currentThreatDistanceSquared <= threatAtActorToleranceSquared) {
		result.status = NpcActorEscapeTarget2DStatus::ThreatAtActorPosition;
		return result;
	}

	const int searchRadius = config.searchRadius < 0 ? 0 : config.searchRadius;
	for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
		for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
			if (dx == 0 && dy == 0) {
				continue;
			}

			const TileCoord candidateTile {
				result.startTile.x + dx,
				result.startTile.y + dy,
			};
			if (!containsTile(map, candidateTile) || !isWalkable(map, candidateTile)) {
				continue;
			}

			const Vec2 candidatePosition = tileCenter(candidateTile);
			result.candidates.push_back({
				candidateTile,
				candidatePosition,
				DistanceSquared(candidatePosition, intent.targetPosition),
				DistanceSquared(candidatePosition, intent.startPosition),
			});
		}
	}

	if (result.candidates.empty()) {
		result.status = NpcActorEscapeTarget2DStatus::NoCandidates;
		return result;
	}

	std::size_t selectedIndex = 0;
	for (std::size_t index = 1; index < result.candidates.size(); ++index) {
		if (IsBetterCandidate(result.candidates[index], result.candidates[selectedIndex])) {
			selectedIndex = index;
		}
	}

	if (config.requireBetterThanCurrent
		&& result.candidates[selectedIndex].threatDistanceSquared <= currentThreatDistanceSquared) {
		result.status = NpcActorEscapeTarget2DStatus::NoBetterCandidate;
		return result;
	}

	result.status = NpcActorEscapeTarget2DStatus::Ready;
	result.selectedCandidateIndex = selectedIndex;
	result.escapeTile = result.candidates[selectedIndex].tile;
	result.escapePosition = result.candidates[selectedIndex].position;
	result.hasEscapeTarget = true;
	return result;
}

} // namespace iggy
