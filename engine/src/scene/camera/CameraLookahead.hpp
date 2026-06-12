#pragma once

#include "core/math/Vec2.hpp"

namespace iggy {

struct CameraLookaheadConfig {
	float distance = 0.0F;
	float minSpeed = 0.0F;
};

struct CameraLookaheadResult {
	Vec2 target;
	bool applied = false;
};

class CameraLookahead {
public:
	[[nodiscard]] CameraLookaheadResult apply(Vec2 targetPosition, Vec2 targetVelocity, const CameraLookaheadConfig &config) const;
};

} // namespace iggy
