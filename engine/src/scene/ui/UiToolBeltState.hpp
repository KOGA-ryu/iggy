#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy::ui {

struct UiToolBeltState {
	int rows = 1;
	int columns = 1;
	int activeRow = 0;
	int activeColumn = 0;
};

struct UiToolBeltLayout {
	int rows = 1;
	int columns = 1;
	std::vector<ResourceId> itemIds;
	std::vector<int> pinnedRows;
};

struct UiToolBeltRowEntry {
	int row = 0;
	int leadColumn = 0;
};

struct UiToolBeltItemEntry {
	int column = 0;
	bool active = false;
};

struct UiToolBeltView {
	std::vector<UiToolBeltRowEntry> rows;
	int activeRowPosition = -1;
	std::vector<UiToolBeltItemEntry> activeRowItems;
};

[[nodiscard]] bool validUiToolBeltState(const UiToolBeltState &state);
[[nodiscard]] int uiToolBeltSlotIndex(const UiToolBeltState &state, int row, int column);
[[nodiscard]] int uiToolBeltActiveIndex(const UiToolBeltState &state);
[[nodiscard]] bool uiToolBeltCellOccupied(
	const UiToolBeltState &state,
	const std::vector<ResourceId> &itemIds,
	int row,
	int column);
[[nodiscard]] int uiToolBeltRowLeadColumn(
	const UiToolBeltState &state,
	const std::vector<ResourceId> &itemIds,
	int row);
[[nodiscard]] UiToolBeltState normalizeUiToolBeltToOccupied(
	UiToolBeltState state,
	const std::vector<ResourceId> &itemIds);
[[nodiscard]] UiToolBeltState stepUiToolBeltRow(UiToolBeltState state, int delta);
[[nodiscard]] UiToolBeltState stepUiToolBeltColumn(UiToolBeltState state, int delta);
[[nodiscard]] UiToolBeltState stepUiToolBeltRowOccupied(
	UiToolBeltState state,
	int delta,
	const std::vector<ResourceId> &itemIds);
[[nodiscard]] UiToolBeltState stepUiToolBeltColumnOccupied(
	UiToolBeltState state,
	int delta,
	const std::vector<ResourceId> &itemIds);
[[nodiscard]] UiToolBeltView uiToolBeltView(
	const UiToolBeltState &state,
	const std::vector<ResourceId> &itemIds);
[[nodiscard]] std::vector<int> pinUiToolBeltRow(std::vector<int> pins, int row);
[[nodiscard]] std::vector<int> unpinUiToolBeltRow(std::vector<int> pins, int row);
[[nodiscard]] std::vector<int> pruneUiToolBeltPins(
	std::vector<int> pins,
	const UiToolBeltState &state,
	const std::vector<ResourceId> &itemIds);

} // namespace iggy::ui
