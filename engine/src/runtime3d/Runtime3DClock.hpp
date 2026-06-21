#pragma once

namespace iggy::runtime3d {

enum class Runtime3DClockMode {
	Normal,
	Slow,
	Paused,
	StepRequested,
};

struct Runtime3DClockState {
	Runtime3DClockMode mode = Runtime3DClockMode::Normal;
	float timeScale = 1.0F;
	float accumulatedSeconds = 0.0F;
};

struct Runtime3DClockTickDecision {
	bool shouldRunTick = false;
	bool automaticTick = false;
	bool stepRequestConsumed = false;
};

[[nodiscard]] Runtime3DClockTickDecision DecideRuntime3DClockTick(Runtime3DClockState clock);

} // namespace iggy::runtime3d
