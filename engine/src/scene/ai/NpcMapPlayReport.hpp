#pragma once

#include <cstddef>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/AiMapQuery2D.hpp"
#include "scene/ai/NpcFold.hpp"
#include "scene/ai/NpcHand.hpp"
#include "scene/ai/NpcMapRead.hpp"
#include "scene/ai/NpcRead.hpp"

namespace iggy {

struct NpcMapPlayReport {
	NpcHand hand;
	AiMapQuery2DResult map;
	NpcRead rawRead;
	NpcMapRead mapRead;
	NpcRead projectedMapRead;
	NpcPlay play;
	NpcTell tell;
	NpcFold fold;
	std::size_t rawRankedCount = 0;
	std::size_t mapRankedCount = 0;
	bool hasRawSelection = false;
	ResourceId rawSelectedActionTag;
	NpcBehaviorStateType rawSelectedBehaviorState = NpcBehaviorStateType::None;
	bool hasMapSelection = false;
	ResourceId mapSelectedActionTag;
	NpcBehaviorStateType mapSelectedBehaviorState = NpcBehaviorStateType::None;
	bool mapChangedSelection = false;

	[[nodiscard]] bool kept() const;
	[[nodiscard]] bool folded() const;
};

class NpcMapPlayReporter {
public:
	[[nodiscard]] NpcMapPlayReport report(
		const NpcHand &hand,
		const AiMapQuery2DResult &map,
		const NpcReadConfig &rawConfig = {},
		const NpcMapReadConfig &mapConfig = {},
		const NpcFoldConfig &foldConfig = {}) const;
};

} // namespace iggy
