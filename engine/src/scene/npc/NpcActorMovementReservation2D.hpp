#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/npc/NpcActorMovementFrameApply2D.hpp"
#include "scene/npc/NpcActorOccupancyPolicy2D.hpp"

namespace iggy {

enum class NpcActorMovementReservation2DStatus {
	Reserved,
	NoAcceptedRequests,
};

enum class NpcActorMovementReservationEntry2DStatus {
	Accepted,
	ReservationBlocked,
};

struct NpcActorMovementReservation2DConfig {
	NpcActorOccupancyPolicy2DConfig policy;
};

struct NpcActorMovementReservation2DEntry {
	std::size_t requestIndex = 0;
	NpcActorMovementFrameApply2DRequest request;
	NpcActorMovementReservationEntry2DStatus status = NpcActorMovementReservationEntry2DStatus::Accepted;
	std::optional<std::size_t> acceptedRequestIndex;
	std::optional<std::size_t> blockingRequestIndex;
	ResourceId blockingNpcId;
	TileCoord tile;
	bool movementRequest = false;
};

struct NpcActorMovementReservation2DResult {
	std::vector<NpcActorMovementFrameApply2DRequest> inputRequests;
	std::vector<NpcActorMovementReservation2DEntry> entries;
	std::vector<NpcActorMovementFrameApply2DRequest> acceptedRequests;
	NpcActorMovementReservation2DStatus status = NpcActorMovementReservation2DStatus::NoAcceptedRequests;
	std::size_t requestCount = 0;
	std::size_t acceptedCount = 0;
	std::size_t rejectedCount = 0;
	std::size_t movementRequestCount = 0;
	std::size_t reservedMovementCount = 0;
	std::size_t reservationBlockedCount = 0;

	[[nodiscard]] bool hasAcceptedRequests() const;
};

class NpcActorMovementReservationProjector2D {
public:
	[[nodiscard]] NpcActorMovementReservation2DResult reserve(
		const std::vector<NpcActorMovementFrameApply2DRequest> &requests,
		const NpcActorMovementReservation2DConfig &config = {}) const;
};

} // namespace iggy
