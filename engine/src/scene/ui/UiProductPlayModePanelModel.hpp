#pragma once

#include <string>
#include <vector>

#include "runtime/RuntimeGameplayProductPlayMode.hpp"

namespace iggy::ui {

struct UiProductPlayModePanelRow {
	std::string key;
	std::string value;
};

struct UiProductPlayModePanelModelInput {
	const runtime::RuntimeGameplayProductPlayModeBuildResult *build = nullptr;
	const runtime::RuntimeGameplayProductPlayModeState *state = nullptr;
	const runtime::RuntimeGameplayProductPlayModeFrameResult *latestFrame = nullptr;
};

struct UiProductPlayModePanelModel {
	bool present = false;
	std::vector<UiProductPlayModePanelRow> buildStatus;
	std::vector<UiProductPlayModePanelRow> identity;
	std::vector<UiProductPlayModePanelRow> state;
	std::vector<UiProductPlayModePanelRow> latestFrame;
	std::vector<UiProductPlayModePanelRow> adapter;
	std::vector<UiProductPlayModePanelRow> binding;
	std::vector<UiProductPlayModePanelRow> step;
	std::vector<UiProductPlayModePanelRow> presentation;
};

[[nodiscard]] UiProductPlayModePanelModel buildUiProductPlayModePanelModel(
	const UiProductPlayModePanelModelInput &input);

} // namespace iggy::ui
