#pragma once

#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "scene/player/PlayerInputBinding2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductInputContextStatus {
	NotLoaded,
	LoadedWithoutPlayer,
	Projected,
};

struct RuntimeGameplayProductInputContextResult {
	RuntimeGameplayProductInputContextStatus status =
		RuntimeGameplayProductInputContextStatus::NotLoaded;
	PlayerInputBindingContext2D bindingContext;
};

class RuntimeGameplayProductInputContext {
public:
	[[nodiscard]] RuntimeGameplayProductInputContextResult build(
		const RuntimeGameplayProductPlayModeState &state) const;
};

} // namespace iggy::runtime
