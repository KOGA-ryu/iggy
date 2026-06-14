#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcCharismaDraw.hpp"
#include "scene/ai/NpcConstitutionDraw.hpp"
#include "scene/ai/NpcDexterityDraw.hpp"
#include "scene/ai/NpcIntelligenceDraw.hpp"
#include "scene/ai/NpcStrengthDraw.hpp"
#include "scene/ai/NpcWisdomDraw.hpp"
#include "scene/npc/NpcBehaviorState.hpp"

namespace iggy {

enum class NpcHandTraitSource {
	Strength,
	Dexterity,
	Constitution,
	Intelligence,
	Wisdom,
	Charisma,
};

enum class NpcHandIssueCode {
	InvalidDraw,
};

struct NpcHandEnt {
	NpcHandTraitSource source = NpcHandTraitSource::Strength;
	ResourceId entryId;
	ResourceId actionTag;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	float weight = 0.0F;
	std::vector<ResourceId> mapTags;
	std::size_t drawEntryIndex = 0;
};

struct NpcHandIssue {
	NpcHandIssueCode code = NpcHandIssueCode::InvalidDraw;
	NpcHandTraitSource source = NpcHandTraitSource::Strength;
};

struct NpcHand {
	std::vector<NpcHandEnt> ents;
	std::vector<NpcHandIssue> issues;

	[[nodiscard]] bool hasEnts() const;
	[[nodiscard]] bool hasIssues() const;
};

class NpcHandAssembler {
public:
	[[nodiscard]] NpcHand assemble(
		const NpcStrengthDrawResult &strength,
		const NpcDexterityDrawResult &dexterity,
		const NpcConstitutionDrawResult &constitution,
		const NpcIntelligenceDrawResult &intelligence,
		const NpcWisdomDrawResult &wisdom,
		const NpcCharismaDrawResult &charisma) const;
};

} // namespace iggy
