#pragma once

#include <cstdint>

#include "core/math/Vec2.hpp"

namespace iggy {

struct CameraShakeState {
	float remaining = 0.0F;
	float elapsed = 0.0F;
	std::uint32_t seed = 0;
};

struct CameraShakeConfig {
	float amplitude = 0.0F;
	float frequency = 0.0F;
	float decay = 0.0F;
};

struct CameraShakeStepInput {
	CameraShakeState state;
	float delta = 0.0F;
	CameraShakeConfig config;
};

struct CameraShakeResult {
	CameraShakeState state;
	Vec2 offset;
	bool active = false;
};

class CameraShake {
public:
	[[nodiscard]] CameraShakeResult step(const CameraShakeStepInput &input) const;
};

} // namespace iggy
