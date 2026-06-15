#pragma once

#include "scene/npc/NpcActorMovementFrameReport2D.hpp"
#include "scene/npc/NpcActorMovementReservation2D.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"

namespace iggy {

enum class NpcActorMovementReservedFrameApply2DStatus {
	Applied,
	NoMovementsApplied,
};

struct NpcActorMovementReservedFrameApply2DConfig {
	NpcActorMovementReservation2DConfig reservation;
};

struct NpcActorMovementReservedFrameApply2DResult {
	NpcActorState2DRegistry inputRegistry;
	NpcActorOccupancy2D occupancy;
	NpcActorMovementReservation2DResult reservation;
	NpcActorMovementFrameApply2DResult apply;
	NpcActorMovementFrameReport2D report;
	NpcActorState2DRegistry registry;
	NpcActorMovementReservedFrameApply2DStatus status = NpcActorMovementReservedFrameApply2DStatus::NoMovementsApplied;
	std::size_t requestCount = 0;
	std::size_t acceptedRequestCount = 0;
	std::size_t reservationRejectedCount = 0;
	std::size_t movedCount = 0;
	std::size_t blockedCount = 0;
	bool changed = false;

	[[nodiscard]] bool applied() const;
};

class NpcActorMovementReservedFrameApplier2D {
public:
	[[nodiscard]] NpcActorMovementReservedFrameApply2DResult apply(
		const NpcActorState2DRegistry &registry,
		const std::vector<NpcActorMovementFrameApply2DRequest> &requests,
		const NpcActorOccupancy2D &occupancy,
		const NpcActorMovementReservedFrameApply2DConfig &config = {}) const;
};

} // namespace iggy
