#pragma once

#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"

namespace iggy::ui {

struct UiAuthoringPreviewPanelRow {
	std::string key;
	std::string value;
};

struct UiAuthoringPreviewTraceFrameView {
	std::string label;
	std::vector<UiAuthoringPreviewPanelRow> summary;
	std::vector<std::string> rows;
};

struct UiAuthoringPreviewPanelModel {
	bool present = false;
	bool ok = false;
	std::vector<UiAuthoringPreviewPanelRow> header;
	std::vector<UiAuthoringPreviewPanelRow> packageMetadata;
	std::vector<UiAuthoringPreviewPanelRow> status;
	std::vector<UiAuthoringPreviewPanelRow> summary;
	std::vector<std::string> diagnostics;
	std::vector<std::string> packageIssues;
	std::vector<std::string> finalRows;
	std::vector<UiAuthoringPreviewTraceFrameView> traceFrames;
	std::vector<UiAuthoringPreviewPanelRow> expectation;
};

[[nodiscard]] UiAuthoringPreviewPanelModel buildUiAuthoringPreviewPanelModel(
	const runtime::RuntimeGameplayAuthoringPreviewModel &preview);

} // namespace iggy::ui
