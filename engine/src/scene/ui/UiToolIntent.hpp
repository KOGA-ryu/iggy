#pragma once

#include <string>
#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/level/TileCoord.hpp"
#include "scene/ui/UiFeatureContext.hpp"
#include "scene/ui/UiToolInventory.hpp"

namespace iggy::ui {

enum class UiToolIntentType {
	None,
	ActivateTool,
	SelectActor,
	SelectTarget,
	UseActiveToolOnPoint,
	UseActiveToolOnTile,
	UseActiveToolOnTarget,
	RequestSave,
};

struct UiToolIntent {
	UiToolIntentType type = UiToolIntentType::None;
	ResourceId toolId;
	ResourceId actorId;
	ResourceId targetId;
	Vec2 targetPoint;
	TileCoord targetTile;
};

enum class UiActionType {
	SetActiveTool,
	SelectActor,
	SelectTarget,
	QueueCommandFrame,
	SetStatusText,
	RequestSave,
};

struct UiAction {
	UiActionType type = UiActionType::SetStatusText;
	ResourceId resourceId;
	runtime::GameplayCommandFrame2D commandFrame;
	std::string text;
};

struct UiActionPlan {
	std::vector<UiAction> actions;
};

[[nodiscard]] UiActionPlan buildUiToolIntentActionPlan(
	const UiToolIntent &intent,
	const UiFeatureContext &context,
	const UiToolInventory &inventory);
void executeUiActionPlan(const UiActionPlan &plan, const UiShellActions &actions);

} // namespace iggy::ui
