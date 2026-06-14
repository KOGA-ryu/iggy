#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "scene/ai/NpcConstitutionPool.hpp"
#include "scene/ai/NpcTraitSet.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

enum class NpcConstitutionDrawStatus {
	Drawn,
	InvalidConstitution,
};

struct NpcConstitutionDrawEntry {
	NpcConstitutionEnt entry;
	std::size_t entryIndex = 0;
};

struct NpcConstitutionDrawResult {
	NpcConstitutionDrawStatus status = NpcConstitutionDrawStatus::Drawn;
	std::uint32_t constitution = 0;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	std::vector<NpcConstitutionDrawEntry> entries;

	[[nodiscard]] bool hasEntries() const;
};

class NpcConstitutionDraw {
public:
	[[nodiscard]] NpcConstitutionDrawResult draw(
		const NpcConstitutionPool &pool,
		std::uint32_t constitution,
		NpcBehaviorStateType behaviorState) const;

	[[nodiscard]] NpcConstitutionDrawResult draw(
		const NpcConstitutionPool &pool,
		const NpcTraitSet &traits,
		NpcBehaviorStateType behaviorState) const;
};

} // namespace iggy
