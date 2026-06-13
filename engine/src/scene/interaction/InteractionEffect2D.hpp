#pragma once

#include <string>

#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class InteractionEffect2DType {
	None,
	InspectText,
	ToggleTarget,
	EmitEvent,
};

struct InteractionEffect2D {
	InteractionEffect2DType type = InteractionEffect2DType::None;
	ResourceId targetId;
	ResourceId eventId;
	std::string text;
	bool enabledValue = true;
};

enum class InteractionEffect2DStatus {
	Valid,
	MissingText,
	MissingTarget,
	MissingEvent,
};

[[nodiscard]] InteractionEffect2D noneInteractionEffect();
[[nodiscard]] InteractionEffect2D inspectTextInteractionEffect(ResourceId targetId, std::string text);
[[nodiscard]] InteractionEffect2D toggleTargetInteractionEffect(ResourceId targetId, bool enabledValue);
[[nodiscard]] InteractionEffect2D emitInteractionEventEffect(ResourceId targetId, ResourceId eventId);

[[nodiscard]] InteractionEffect2DStatus validate(const InteractionEffect2D &effect);
[[nodiscard]] bool valid(const InteractionEffect2D &effect);

} // namespace iggy
