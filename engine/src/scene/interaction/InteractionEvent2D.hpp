#pragma once

#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class InteractionEvent2DType {
	TargetToggled,
	InspectTextRequested,
	EventEmitted,
};

struct InteractionEvent2D {
	InteractionEvent2DType type = InteractionEvent2DType::TargetToggled;
	ResourceId targetId;
	ResourceId eventId;
	std::string text;
	bool enabledValue = true;
};

[[nodiscard]] InteractionEvent2D targetToggledInteractionEvent(ResourceId targetId, bool enabledValue);
[[nodiscard]] InteractionEvent2D inspectTextRequestedInteractionEvent(ResourceId targetId, std::string text);
[[nodiscard]] InteractionEvent2D interactionEventEmitted(ResourceId targetId, ResourceId eventId);

struct InteractionEventRecorder2D {
	std::vector<InteractionEvent2D> events;
};

void recordInteractionEvent(InteractionEventRecorder2D &recorder, const InteractionEvent2D &event);

} // namespace iggy
