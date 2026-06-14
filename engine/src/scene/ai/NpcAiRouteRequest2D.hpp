#pragma once

#include "core/math/Vec2.hpp"
#include "scene/ai/NpcAiDecision2D.hpp"

namespace iggy {

enum class NpcAiRouteRequest2DStatus {
	Requested,
	NoDecision,
	HoldPosition,
	NoMovementNeeded,
};

struct NpcAiRouteRequest2DConfig {
	float arrivalTolerance = 0.001F;
};

struct NpcAiRouteRequest2DResult {
	NpcAiRouteRequest2DStatus status = NpcAiRouteRequest2DStatus::NoDecision;
	NpcAiDecision2DResult decision;
	Vec2 startPosition;
	Vec2 targetPosition;
	NpcAiBehaviorIntent2DType intentType = NpcAiBehaviorIntent2DType::None;
	bool requestsRoute = false;

	[[nodiscard]] bool hasRouteRequest() const;
};

class NpcAiRouteRequestBuilder2D {
public:
	[[nodiscard]] NpcAiRouteRequest2DResult build(
		const NpcAiDecision2DResult &decision,
		const NpcAiRouteRequest2DConfig &config = {}) const;
};

} // namespace iggy
