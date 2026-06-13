#include "scene/interaction/InteractionEvent2D.hpp"

#include <utility>

namespace iggy {

InteractionEvent2D targetToggledInteractionEvent(ResourceId targetId, bool enabledValue)
{
	InteractionEvent2D event;
	event.type = InteractionEvent2DType::TargetToggled;
	event.targetId = std::move(targetId);
	event.enabledValue = enabledValue;
	return event;
}

InteractionEvent2D inspectTextRequestedInteractionEvent(ResourceId targetId, std::string text)
{
	InteractionEvent2D event;
	event.type = InteractionEvent2DType::InspectTextRequested;
	event.targetId = std::move(targetId);
	event.text = std::move(text);
	return event;
}

InteractionEvent2D interactionEventEmitted(ResourceId targetId, ResourceId eventId)
{
	InteractionEvent2D event;
	event.type = InteractionEvent2DType::EventEmitted;
	event.targetId = std::move(targetId);
	event.eventId = std::move(eventId);
	return event;
}

void recordInteractionEvent(InteractionEventRecorder2D &recorder, const InteractionEvent2D &event)
{
	recorder.events.push_back(event);
}

} // namespace iggy
