#pragma once

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeGameplayProductInteractionTargetQuery.hpp"
#include "scene/player/PlayerInputBinding2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductInputTargetContextStatus {
	Unchanged,
	TargetProjected,
};

struct RuntimeGameplayProductInputTargetContextInput {
	PlayerInputBindingContext2D base;
	RuntimeGameplayProductInteractionTargetQueryResult target;
};

struct RuntimeGameplayProductInputTargetContextResult {
	RuntimeGameplayProductInputTargetContextStatus status =
		RuntimeGameplayProductInputTargetContextStatus::Unchanged;
	PlayerInputBindingContext2D bindingContext;
	ResourceId hoveredTargetId;
};

class RuntimeGameplayProductInputTargetContext {
public:
	[[nodiscard]] RuntimeGameplayProductInputTargetContextResult project(
		const RuntimeGameplayProductInputTargetContextInput &input) const;
};

} // namespace iggy::runtime
