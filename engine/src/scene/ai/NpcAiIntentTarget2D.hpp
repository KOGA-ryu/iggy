#pragma once

#include <cstddef>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcAiBehaviorIntent2D.hpp"

namespace iggy {

enum class NpcAiIntentTarget2DStatus {
	Selected,
	NoIntent,
	NoCandidates,
};

struct NpcAiIntentTarget2DResult {
	NpcAiIntentTarget2DStatus status = NpcAiIntentTarget2DStatus::NoIntent;
	NpcAiBehaviorIntent2DResult intent;
	ResourceId selectedNodeId;
	Vec2 selectedPosition;
	AiMapNode2D selectedNode;
	std::size_t selectedCandidateIndex = 0;
	float selectedScore = 0.0F;
	bool selectedFromNode = false;

	[[nodiscard]] bool hasTarget() const;
};

class NpcAiIntentTargetSelector2D {
public:
	[[nodiscard]] NpcAiIntentTarget2DResult select(const NpcAiBehaviorIntent2DResult &intent) const;
};

} // namespace iggy
