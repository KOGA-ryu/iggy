#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionEffectFrameReport.hpp"
#include "runtime/RuntimeSessionState.hpp"

namespace iggy::ui {

struct UiRuntimeFrameInspectorRow {
	std::string key;
	std::string value;
};

struct UiRuntimeFrameInspectorModel {
	std::size_t tickIndex = 0;
	bool hasSession = false;
	bool hasPlayer = false;
	bool hasRenderCache = false;
	std::size_t acceptedCommandCount = 0;
	std::size_t blockedIntentCount = 0;
	std::size_t rejectedIntentCount = 0;
	std::size_t readyInteractionCount = 0;
	std::size_t noEffectInteractionCount = 0;
	std::size_t blockedInteractionCount = 0;
	std::size_t requestedEffectCount = 0;
	std::vector<UiRuntimeFrameInspectorRow> rows;
};

[[nodiscard]] std::string uiRuntimeInteractionEffectFrameEventLabel(
	runtime::RuntimePlayerInputInteractionEffectFrameEvent event);
[[nodiscard]] UiRuntimeFrameInspectorModel buildUiRuntimeFrameInspectorModel(
	const runtime::RuntimeSessionState *session,
	const runtime::RuntimePlayerInputInteractionEffectFrameReport *report);

} // namespace iggy::ui
