#pragma once

#include <optional>

#include "app/RuntimeInputFocusResolver.hpp"
#include "app/RuntimeInputTypes.hpp"
#include "focus/InputFocus.hpp"
#include "player/Player.hpp"
#include "player/PlayerActionGate.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

struct RuntimeMovementInputContext {
	const SimulationWorld *world = nullptr;
	const Player *player = nullptr;
	FocusState focusState;
	PlayerActionContext actionContext;
	PlayerActionBlockReason blockReason = PlayerActionBlockReason::None;
};

class RuntimeMovementInputContextBuilder {
public:
	[[nodiscard]] std::optional<RuntimeMovementInputContext> build(const RuntimeInputContext &context) const;

private:
	RuntimeInputFocusResolver focusResolver_;
};

} // namespace dev
