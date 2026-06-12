#include "scene/camera/CameraShake.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace iggy {

namespace {

constexpr float Pi = 3.14159265358979323846F;

std::uint32_t Hash(std::uint32_t value)
{
	value ^= value >> 16;
	value *= 0x7feb352dU;
	value ^= value >> 15;
	value *= 0x846ca68bU;
	value ^= value >> 16;
	return value;
}

float UnitFloat(std::uint32_t value)
{
	constexpr float denominator = 4294967295.0F;
	return static_cast<float>(value) / denominator;
}

Vec2 DirectionFor(std::uint32_t seed, std::uint32_t sampleIndex)
{
	const float angle = UnitFloat(Hash(seed ^ Hash(sampleIndex))) * 2.0F * Pi;
	return { std::cos(angle), std::sin(angle) };
}

} // namespace

CameraShakeResult CameraShake::step(const CameraShakeStepInput &input) const
{
	if (input.state.remaining <= 0.0F || input.delta <= 0.0F)
		return { input.state, {}, false };

	CameraShakeState nextState = input.state;
	nextState.elapsed += input.delta;
	nextState.remaining = std::max(0.0F, input.state.remaining - input.delta);

	if (nextState.remaining <= 0.0F || input.config.amplitude <= 0.0F || input.config.frequency <= 0.0F)
		return { nextState, {}, false };

	float effectiveAmplitude = input.config.amplitude;
	if (input.config.decay > 0.0F)
		effectiveAmplitude *= std::min(1.0F, nextState.remaining / input.config.decay);

	const std::uint32_t sampleIndex = static_cast<std::uint32_t>(std::floor(nextState.elapsed * input.config.frequency));
	return { nextState, DirectionFor(input.state.seed, sampleIndex) * effectiveAmplitude, true };
}

} // namespace iggy
