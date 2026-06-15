#include "scene/npc/NpcActorMovementReservation2D.hpp"

namespace {

struct ReservationClaim {
	iggy::TileCoord tile;
	std::vector<iggy::ResourceId> npcIds;
	std::vector<std::size_t> requestIndexes;
};

ReservationClaim *FindClaim(std::vector<ReservationClaim> &claims, iggy::TileCoord tile)
{
	for (ReservationClaim &claim : claims) {
		if (claim.tile == tile) {
			return &claim;
		}
	}
	return nullptr;
}

bool ContainsNpcId(const std::vector<iggy::ResourceId> &npcIds, const iggy::ResourceId &npcId)
{
	for (const iggy::ResourceId &existing : npcIds) {
		if (existing == npcId) {
			return true;
		}
	}
	return false;
}

} // namespace

namespace iggy {

bool NpcActorMovementReservation2DResult::hasAcceptedRequests() const
{
	return !acceptedRequests.empty();
}

NpcActorMovementReservation2DResult NpcActorMovementReservationProjector2D::reserve(
	const std::vector<NpcActorMovementFrameApply2DRequest> &requests,
	const NpcActorMovementReservation2DConfig &config) const
{
	NpcActorMovementReservation2DResult result;
	result.inputRequests = requests;
	result.requestCount = requests.size();
	std::vector<ReservationClaim> claims;

	for (std::size_t requestIndex = 0; requestIndex < requests.size(); ++requestIndex) {
		const NpcActorMovementFrameApply2DRequest &request = requests[requestIndex];
		NpcActorMovementReservation2DEntry entry;
		entry.requestIndex = requestIndex;
		entry.request = request;
		entry.tile = request.filter.step.proposedTile;
		entry.movementRequest = request.filter.allowed();

		if (!entry.movementRequest) {
			entry.acceptedRequestIndex = result.acceptedRequests.size();
			result.acceptedRequests.push_back(request);
			++result.acceptedCount;
			result.entries.push_back(entry);
			continue;
		}

		++result.movementRequestCount;
		ReservationClaim *claim = FindClaim(claims, request.filter.step.proposedTile);
		if (claim == nullptr) {
			if (config.policy.maxOccupantsPerTile == 0) {
				entry.status = NpcActorMovementReservationEntry2DStatus::ReservationBlocked;
				++result.rejectedCount;
				++result.reservationBlockedCount;
				result.entries.push_back(entry);
				continue;
			}

			claims.push_back({ request.filter.step.proposedTile, { request.filter.step.npcId }, { requestIndex } });
			entry.acceptedRequestIndex = result.acceptedRequests.size();
			result.acceptedRequests.push_back(request);
			++result.acceptedCount;
			++result.reservedMovementCount;
			result.entries.push_back(entry);
			continue;
		}

		const bool sameNpcAlreadyClaimed = ContainsNpcId(claim->npcIds, request.filter.step.npcId);
		const std::size_t projectedOccupants = claim->npcIds.size() + (sameNpcAlreadyClaimed ? 0U : 1U);
		if (!request.filter.step.npcId.empty() && projectedOccupants <= config.policy.maxOccupantsPerTile) {
			if (!sameNpcAlreadyClaimed) {
				claim->npcIds.push_back(request.filter.step.npcId);
			}
			claim->requestIndexes.push_back(requestIndex);
			entry.acceptedRequestIndex = result.acceptedRequests.size();
			result.acceptedRequests.push_back(request);
			++result.acceptedCount;
			++result.reservedMovementCount;
			result.entries.push_back(entry);
			continue;
		}

		entry.status = NpcActorMovementReservationEntry2DStatus::ReservationBlocked;
		if (!claim->requestIndexes.empty()) {
			entry.blockingRequestIndex = claim->requestIndexes.front();
		}
		if (!claim->npcIds.empty()) {
			entry.blockingNpcId = claim->npcIds.front();
		}
		++result.rejectedCount;
		++result.reservationBlockedCount;
		result.entries.push_back(entry);
	}

	if (!result.acceptedRequests.empty()) {
		result.status = NpcActorMovementReservation2DStatus::Reserved;
	}
	return result;
}

} // namespace iggy
