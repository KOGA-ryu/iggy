#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "scene/ai/NpcDexterityPool.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

enum class NpcDexterityDrawStatus {
	Drawn,
	InvalidDexterity,
};

struct NpcDexterityDrawEntry {
	NpcDexterityEnt entry;
	std::size_t entryIndex = 0;
};

struct NpcDexterityDrawResult {
	NpcDexterityDrawStatus status = NpcDexterityDrawStatus::Drawn;
	std::uint32_t dexterity = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	std::vector<NpcDexterityDrawEntry> entries;

	[[nodiscard]] bool hasEntries() const;
};

class NpcDexterityDraw {
public:
	[[nodiscard]] NpcDexterityDrawResult draw(
		const NpcDexterityPool &pool,
		std::uint32_t dexterity,
		NpcBehaviorStateType behaviorState) const;

	[[nodiscard]] NpcDexterityDrawResult draw(
		const NpcDexterityPool &pool,
		const NpcTraitSet &traits,
		NpcBehaviorStateType behaviorState) const;
};

} // namespace iggy
