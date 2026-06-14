#include "scene/npc/NpcActorState2D.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcActorState2D> &actors, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (actors[index].npcId == actors[currentIndex].npcId)
			return true;
	}
	return false;
}

iggy::NpcActorState2DIssue Issue(
	iggy::NpcActorState2DIssueCode code,
	std::size_t actorIndex,
	const iggy::NpcActorState2D &actor)
{
	return {
		code,
		actorIndex,
		actor,
	};
}

} // namespace

namespace iggy {

const NpcActorState2D *NpcActorState2DRegistry::find(const ResourceId &npcId) const
{
	for (const NpcActorState2D &actor : actors) {
		if (actor.npcId == npcId)
			return &actor;
	}
	return nullptr;
}

bool NpcActorState2DRegistry::contains(const ResourceId &npcId) const
{
	return find(npcId) != nullptr;
}

NpcActorState2DRegistryBuildResult NpcActorState2DRegistryBuilder::build(
	const std::vector<NpcActorState2D> &actors) const
{
	NpcActorState2DRegistryBuildResult result;

	for (std::size_t index = 0; index < actors.size(); ++index) {
		const NpcActorState2D &actor = actors[index];
		if (actor.npcId.empty())
			result.issues.push_back(Issue(NpcActorState2DIssueCode::EmptyNpcId, index, actor));
		if (HasEarlierMatchingId(actors, index))
			result.issues.push_back(Issue(NpcActorState2DIssueCode::DuplicateNpcId, index, actor));
		if (actor.aiProfileId.empty())
			result.issues.push_back(Issue(NpcActorState2DIssueCode::EmptyAiProfileId, index, actor));
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.registry.actors = actors;
	return result;
}

} // namespace iggy
