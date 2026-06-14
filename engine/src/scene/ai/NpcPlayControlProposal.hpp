#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcFold.hpp"
#include "scene/ai/NpcObjective.hpp"
#include "scene/npc/NpcBehaviorState.hpp"
#include "scene/npc/NpcMoveMode.hpp"

namespace iggy {

enum class NpcPlayControlProposalStatus {
	Proposed,
	NoKeptPlay,
	MissingTargetPosition,
	MissingTargetId,
	InvalidObjective,
	InvalidBehavior,
};

struct NpcPlayControlProposalContext {
	bool hasTargetPosition = false;
	Vec2 targetPosition;
	ResourceId targetId;
};

struct NpcPlayControlProposalConfig {
	NpcMoveMode idleMoveMode = NpcMoveMode::Still;
	NpcMoveMode waitingMoveMode = NpcMoveMode::Still;
	NpcMoveMode seekingMoveMode = NpcMoveMode::Walk;
	NpcMoveMode fleeingMoveMode = NpcMoveMode::Run;
	NpcMoveMode attackingMoveMode = NpcMoveMode::Still;
	NpcMoveMode interactingMoveMode = NpcMoveMode::Walk;
	NpcMoveMode stunnedMoveMode = NpcMoveMode::Still;
	NpcMoveMode disabledMoveMode = NpcMoveMode::Still;
};

struct NpcPlayControlProposal {
	NpcFold fold;
	NpcPlayControlProposalStatus status = NpcPlayControlProposalStatus::NoKeptPlay;
	ResourceId actionTag;
	NpcBehaviorStateType requestedBehaviorState = NpcBehaviorStateType::None;
	NpcObjective objective;
	NpcBehaviorState behavior;
	NpcMoveMode moveMode = NpcMoveMode::None;

	[[nodiscard]] bool hasProposal() const;
};

class NpcPlayControlProjector {
public:
	[[nodiscard]] NpcPlayControlProposal project(
		const NpcFold &fold,
		const NpcPlayControlProposalContext &context = {},
		const NpcPlayControlProposalConfig &config = {}) const;
};

} // namespace iggy
