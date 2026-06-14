#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcObjective.hpp"
#include "scene/npc/NpcBehaviorState.hpp"
#include "scene/npc/NpcMoveMode.hpp"

namespace iggy {

struct NpcActorControlState2D {
	ResourceId npcId;
	NpcObjective objective;
	NpcBehaviorState behavior;
	NpcMoveMode moveMode = NpcMoveMode::None;
};

struct NpcActorControlState2DRegistry {
	std::vector<NpcActorControlState2D> entries;

	[[nodiscard]] const NpcActorControlState2D *find(const ResourceId &npcId) const;
	[[nodiscard]] bool contains(const ResourceId &npcId) const;
};

enum class NpcActorControlState2DIssueCode {
	EmptyNpcId,
	DuplicateNpcId,
	InvalidObjective,
	InvalidBehavior,
};

struct NpcActorControlState2DIssue {
	NpcActorControlState2DIssueCode code = NpcActorControlState2DIssueCode::EmptyNpcId;
	std::size_t controlIndex = 0;
	NpcActorControlState2D control;
	NpcObjectiveValidationResult objectiveValidation;
	NpcBehaviorStateValidationResult behaviorValidation;
};

struct NpcActorControlState2DRegistryBuildResult {
	bool built = false;
	NpcActorControlState2DRegistry registry;
	std::vector<NpcActorControlState2DIssue> issues;
};

class NpcActorControlState2DRegistryBuilder {
public:
	[[nodiscard]] NpcActorControlState2DRegistryBuildResult build(
		const std::vector<NpcActorControlState2D> &controls) const;
};

} // namespace iggy
