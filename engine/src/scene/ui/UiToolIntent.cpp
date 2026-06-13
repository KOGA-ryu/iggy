#include "scene/ui/UiToolIntent.hpp"

#include <utility>

namespace iggy::ui {
namespace {

ResourceId Id(const char *value)
{
	return ResourceId { value };
}

ResourceId IntentToolId(const UiToolIntent &intent, const UiFeatureContext &context)
{
	return intent.toolId.empty() ? context.activeToolId : intent.toolId;
}

ResourceId IntentActorId(const UiToolIntent &intent, const UiFeatureContext &context)
{
	return intent.actorId.empty() ? context.selectedActorId : intent.actorId;
}

void AddStatus(UiActionPlan &plan, std::string text)
{
	UiAction action;
	action.type = UiActionType::SetStatusText;
	action.text = std::move(text);
	plan.actions.push_back(std::move(action));
}

void AddResourceAction(UiActionPlan &plan, UiActionType type, ResourceId id)
{
	UiAction action;
	action.type = type;
	action.resourceId = std::move(id);
	plan.actions.push_back(std::move(action));
}

void AddCommand(UiActionPlan &plan, runtime::GameplayCommand2D command)
{
	UiAction action;
	action.type = UiActionType::QueueCommandFrame;
	action.commandFrame.commands.push_back(std::move(command));
	plan.actions.push_back(std::move(action));
}

bool KnownTool(const UiToolInventory &inventory, const ResourceId &toolId)
{
	return !toolId.empty() && findUiTool(inventory, toolId) != nullptr;
}

void BuildPointToolPlan(
	UiActionPlan &plan,
	const UiToolIntent &intent,
	const UiFeatureContext &context,
	const ResourceId &toolId)
{
	if (toolId != Id("tool:move"))
		return;

	const ResourceId actorId = IntentActorId(intent, context);
	if (actorId.empty()) {
		AddStatus(plan, "Select an actor before moving.");
		return;
	}

	AddCommand(plan, runtime::GameplayCommand2DFactory {}.moveToPoint(actorId, intent.targetPoint));
}

void BuildTileToolPlan(
	UiActionPlan &plan,
	const UiToolIntent &intent,
	const UiFeatureContext &context,
	const ResourceId &toolId)
{
	if (toolId != Id("tool:move"))
		return;

	const ResourceId actorId = IntentActorId(intent, context);
	if (actorId.empty()) {
		AddStatus(plan, "Select an actor before moving.");
		return;
	}

	AddCommand(plan, runtime::GameplayCommand2DFactory {}.moveToTile(actorId, intent.targetTile));
}

void BuildTargetToolPlan(
	UiActionPlan &plan,
	const UiToolIntent &intent,
	const UiFeatureContext &context,
	const ResourceId &toolId)
{
	if (toolId == Id("tool:select") || toolId == Id("tool:inspect")) {
		if (intent.targetId.empty()) {
			AddStatus(plan, "Select a target first.");
			return;
		}
		AddResourceAction(plan, UiActionType::SelectTarget, intent.targetId);
		return;
	}

	if (toolId != Id("tool:interact") && toolId != Id("tool:pickup"))
		return;

	const ResourceId actorId = IntentActorId(intent, context);
	if (actorId.empty()) {
		AddStatus(plan, "Select an actor before interacting.");
		return;
	}
	if (intent.targetId.empty()) {
		AddStatus(plan, "Select a target before interacting.");
		return;
	}

	AddCommand(plan, runtime::GameplayCommand2DFactory {}.interact(actorId, intent.targetId));
}

} // namespace

UiActionPlan buildUiToolIntentActionPlan(
	const UiToolIntent &intent,
	const UiFeatureContext &context,
	const UiToolInventory &inventory)
{
	UiActionPlan plan;
	switch (intent.type) {
	case UiToolIntentType::None:
		return plan;
	case UiToolIntentType::ActivateTool:
		if (KnownTool(inventory, intent.toolId)) {
			AddResourceAction(plan, UiActionType::SetActiveTool, intent.toolId);
		} else {
			AddStatus(plan, "Unknown tool.");
		}
		return plan;
	case UiToolIntentType::SelectActor:
		if (!intent.actorId.empty())
			AddResourceAction(plan, UiActionType::SelectActor, intent.actorId);
		return plan;
	case UiToolIntentType::SelectTarget:
		if (!intent.targetId.empty())
			AddResourceAction(plan, UiActionType::SelectTarget, intent.targetId);
		return plan;
	case UiToolIntentType::UseActiveToolOnPoint: {
		const ResourceId toolId = IntentToolId(intent, context);
		if (!KnownTool(inventory, toolId)) {
			AddStatus(plan, "Unknown tool.");
			return plan;
		}
		BuildPointToolPlan(plan, intent, context, toolId);
		return plan;
	}
	case UiToolIntentType::UseActiveToolOnTile: {
		const ResourceId toolId = IntentToolId(intent, context);
		if (!KnownTool(inventory, toolId)) {
			AddStatus(plan, "Unknown tool.");
			return plan;
		}
		BuildTileToolPlan(plan, intent, context, toolId);
		return plan;
	}
	case UiToolIntentType::UseActiveToolOnTarget: {
		const ResourceId toolId = IntentToolId(intent, context);
		if (!KnownTool(inventory, toolId)) {
			AddStatus(plan, "Unknown tool.");
			return plan;
		}
		BuildTargetToolPlan(plan, intent, context, toolId);
		return plan;
	}
	case UiToolIntentType::RequestSave: {
		UiAction action;
		action.type = UiActionType::RequestSave;
		plan.actions.push_back(action);
		return plan;
	}
	}
	return plan;
}

void executeUiActionPlan(const UiActionPlan &plan, const UiShellActions &actions)
{
	for (const UiAction &action : plan.actions) {
		switch (action.type) {
		case UiActionType::SetActiveTool:
			if (actions.setActiveTool)
				actions.setActiveTool(action.resourceId);
			break;
		case UiActionType::SelectActor:
			if (actions.selectActor)
				actions.selectActor(action.resourceId);
			break;
		case UiActionType::SelectTarget:
			if (actions.selectTarget)
				actions.selectTarget(action.resourceId);
			break;
		case UiActionType::QueueCommandFrame:
			if (actions.queueCommandFrame)
				actions.queueCommandFrame(action.commandFrame);
			break;
		case UiActionType::SetStatusText:
			if (actions.setStatusText)
				actions.setStatusText(action.text);
			break;
		case UiActionType::RequestSave:
			if (actions.requestSave)
				actions.requestSave();
			break;
		}
	}
}

} // namespace iggy::ui
