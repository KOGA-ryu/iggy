#include "scene/ui/UiInteractionEventPanelModel.hpp"

namespace iggy::ui {

bool UiInteractionEventPanelModel::empty() const
{
	return rows.empty();
}

std::string uiInteractionEventTypeLabel(InteractionEvent2DType type)
{
	switch (type) {
	case InteractionEvent2DType::TargetToggled:
		return "target_toggled";
	case InteractionEvent2DType::InspectTextRequested:
		return "inspect_text_requested";
	case InteractionEvent2DType::EventEmitted:
		return "event_emitted";
	}
	return "unknown";
}

UiInteractionEventPanelModel buildUiInteractionEventPanelModel(const std::vector<InteractionEvent2D> &events)
{
	UiInteractionEventPanelModel model;
	model.rows.reserve(events.size());
	for (const InteractionEvent2D &event : events) {
		UiInteractionEventRow row;
		row.typeLabel = uiInteractionEventTypeLabel(event.type);
		row.targetId = event.targetId;
		row.eventId = event.eventId;
		switch (event.type) {
		case InteractionEvent2DType::TargetToggled:
			row.hasEnabledValue = true;
			row.enabledValue = event.enabledValue;
			row.detail = event.enabledValue ? "enabled" : "disabled";
			++model.targetToggleCount;
			break;
		case InteractionEvent2DType::InspectTextRequested:
			row.detail = event.text;
			++model.inspectTextRequestCount;
			break;
		case InteractionEvent2DType::EventEmitted:
			row.detail = std::string(event.eventId.value());
			++model.emittedEventCount;
			break;
		}
		model.rows.push_back(row);
	}
	return model;
}

} // namespace iggy::ui
