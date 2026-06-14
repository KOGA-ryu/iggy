#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "scene/ai/NpcStrengthPool.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

enum class NpcStrengthDrawStatus {
	Drawn,
	InvalidStrength,
};

struct NpcStrengthDrawEntry {
	NpcStrengthEnt entry;
	std::size_t entryIndex = 0;
};

struct NpcStrengthDrawResult {
	NpcStrengthDrawStatus status = NpcStrengthDrawStatus::Drawn;
	std::uint32_t strength = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	std::vector<NpcStrengthDrawEntry> entries;

	[[nodiscard]] bool hasEntries() const;
};

class NpcStrengthDraw {
public:
	[[nodiscard]] NpcStrengthDrawResult draw(
		const NpcStrengthPool &pool,
		std::uint32_t strength,
		NpcBehaviorStateType behaviorState) const;

	[[nodiscard]] NpcStrengthDrawResult draw(
		const NpcStrengthPool &pool,
		const NpcTraitSet &traits,
		NpcBehaviorStateType behaviorState) const;
};

} // namespace iggy
