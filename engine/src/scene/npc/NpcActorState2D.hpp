#pragma once

#include <cstddef>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"

namespace iggy {

struct NpcActorState2D {
	ResourceId npcId;
	ResourceId aiProfileId;
	ResourceId factionId;
	Vec2 position;
	ResourceId currentGoalId;
	bool present = true;
};

struct NpcActorState2DRegistry {
	std::vector<NpcActorState2D> actors;

	[[nodiscard]] const NpcActorState2D *find(const ResourceId &npcId) const;
	[[nodiscard]] bool contains(const ResourceId &npcId) const;
};

enum class NpcActorState2DIssueCode {
	EmptyNpcId,
	DuplicateNpcId,
	EmptyAiProfileId,
};

struct NpcActorState2DIssue {
	NpcActorState2DIssueCode code = NpcActorState2DIssueCode::EmptyNpcId;
	std::size_t actorIndex = 0;
	NpcActorState2D actor;
};

struct NpcActorState2DRegistryBuildResult {
	bool built = false;
	NpcActorState2DRegistry registry;
	std::vector<NpcActorState2DIssue> issues;
};

class NpcActorState2DRegistryBuilder {
public:
	[[nodiscard]] NpcActorState2DRegistryBuildResult build(const std::vector<NpcActorState2D> &actors) const;
};

} // namespace iggy
