#include "scene/ai/NpcIntelligenceDraw.hpp"

namespace {

bool IntelligenceInRange(std::uint32_t value)
{
	return value <= static_cast<std::uint32_t>(iggy::NpcTraitSetMaxScore);
}

} // namespace

namespace iggy {

bool NpcIntelligenceDrawResult::hasEntries() const
{
	return !entries.empty();
}

NpcIntelligenceDrawResult NpcIntelligenceDraw::draw(
	const NpcIntelligencePool &pool,
	std::uint32_t intelligence,
	NpcBehaviorStateType behaviorState) const
{
	NpcIntelligenceDrawResult result;
	result.intelligence = intelligence;
	result.behaviorState = behaviorState;

	if (!IntelligenceInRange(intelligence)) {
		result.status = NpcIntelligenceDrawStatus::InvalidIntelligence;
		return result;
	}

	for (std::size_t index = 0; index < pool.entries.size(); ++index) {
		const NpcIntelligenceEnt &entry = pool.entries[index];
		if (entry.minimumIntelligence <= intelligence && entry.behaviorState == behaviorState)
			result.entries.push_back({ entry, index });
	}

	return result;
}

NpcIntelligenceDrawResult NpcIntelligenceDraw::draw(
	const NpcIntelligencePool &pool,
	const NpcTraitSet &traits,
	NpcBehaviorStateType behaviorState) const
{
	return draw(pool, static_cast<std::uint32_t>(traits.intelligence), behaviorState);
}

} // namespace iggy
