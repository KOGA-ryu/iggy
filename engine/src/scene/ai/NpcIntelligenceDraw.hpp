#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "scene/ai/NpcIntelligencePool.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

enum class NpcIntelligenceDrawStatus {
	Drawn,
	InvalidIntelligence,
};

struct NpcIntelligenceDrawEntry {
	NpcIntelligenceEnt entry;
	std::size_t entryIndex = 0;
};

struct NpcIntelligenceDrawResult {
	NpcIntelligenceDrawStatus status = NpcIntelligenceDrawStatus::Drawn;
	std::uint32_t intelligence = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	std::vector<NpcIntelligenceDrawEntry> entries;

	[[nodiscard]] bool hasEntries() const;
};

class NpcIntelligenceDraw {
public:
	[[nodiscard]] NpcIntelligenceDrawResult draw(
		const NpcIntelligencePool &pool,
		std::uint32_t intelligence,
		NpcBehaviorStateType behaviorState) const;

	[[nodiscard]] NpcIntelligenceDrawResult draw(
		const NpcIntelligencePool &pool,
		const NpcTraitSet &traits,
		NpcBehaviorStateType behaviorState) const;
};

} // namespace iggy
