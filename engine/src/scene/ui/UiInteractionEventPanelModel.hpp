#pragma once

#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/interaction/InteractionEvent2D.hpp"

namespace iggy::ui {

struct UiInteractionEventRow {
	std::string typeLabel;
	ResourceId targetId;
	ResourceId eventId;
	std::string detail;
	bool hasEnabledValue = false;
	bool enabledValue = false;
};

struct UiInteractionEventPanelModel {
	std::vector<UiInteractionEventRow> rows;
	std::size_t targetToggleCount = 0;
	std::size_t inspectTextRequestCount = 0;
	std::size_t emittedEventCount = 0;

	[[nodiscard]] bool empty() const;
};

[[nodiscard]] std::string uiInteractionEventTypeLabel(InteractionEvent2DType type);
[[nodiscard]] UiInteractionEventPanelModel buildUiInteractionEventPanelModel(
	const std::vector<InteractionEvent2D> &events);

} // namespace iggy::ui
