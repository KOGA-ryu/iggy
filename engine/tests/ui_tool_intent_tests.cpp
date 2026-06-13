#include <cstdlib>
#include <string>

#include "runtime/GameplayCommand2D.hpp"
#include "scene/ui/UiToolIntent.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::ui::UiFeatureContext ContextWithPlayer(iggy::ResourceId activeToolId)
{
	iggy::ui::UiFeatureContext context;
	context.activeToolId = activeToolId;
	context.selectedActorId = Id("actor:player");
	return context;
}

void TestActivateToolPlansSetActiveTool()
{
	iggy::ui::UiToolIntent intent;
	intent.type = iggy::ui::UiToolIntentType::ActivateTool;
	intent.toolId = Id("tool:interact");

	const iggy::ui::UiActionPlan plan = iggy::ui::buildUiToolIntentActionPlan(
		intent,
		{},
		iggy::ui::defaultUiToolInventory());

	Expect(plan.actions.size() == 1, "activating a known tool should emit one action");
	if (plan.actions.size() == 1) {
		Expect(plan.actions[0].type == iggy::ui::UiActionType::SetActiveTool, "known tool activation should set active tool");
		Expect(plan.actions[0].resourceId == Id("tool:interact"), "known tool activation should preserve tool id");
	}
}

void TestUnknownToolPlansStatusOnly()
{
	iggy::ui::UiToolIntent intent;
	intent.type = iggy::ui::UiToolIntentType::ActivateTool;
	intent.toolId = Id("tool:missing");

	const iggy::ui::UiActionPlan plan = iggy::ui::buildUiToolIntentActionPlan(
		intent,
		{},
		iggy::ui::defaultUiToolInventory());

	Expect(plan.actions.size() == 1, "unknown tool should emit one diagnostic action");
	if (plan.actions.size() == 1) {
		Expect(plan.actions[0].type == iggy::ui::UiActionType::SetStatusText, "unknown tool should not mutate active tool");
		Expect(!plan.actions[0].text.empty(), "unknown tool status should include text");
	}
}

void TestMoveToolOnTilePlansGameplayCommandFrame()
{
	iggy::ui::UiToolIntent intent;
	intent.type = iggy::ui::UiToolIntentType::UseActiveToolOnTile;
	intent.targetTile = { 7, 9 };

	const iggy::ui::UiActionPlan plan = iggy::ui::buildUiToolIntentActionPlan(
		intent,
		ContextWithPlayer(Id("tool:move")),
		iggy::ui::defaultUiToolInventory());

	Expect(plan.actions.size() == 1, "move tool on tile should emit one command-frame action");
	if (plan.actions.size() == 1) {
		Expect(plan.actions[0].type == iggy::ui::UiActionType::QueueCommandFrame, "move tool on tile should queue commands");
		Expect(plan.actions[0].commandFrame.commands.size() == 1, "move tool should queue one command");
		if (!plan.actions[0].commandFrame.commands.empty()) {
			const iggy::runtime::GameplayCommand2D &command = plan.actions[0].commandFrame.commands[0];
			Expect(command.type == iggy::runtime::GameplayCommand2DType::MoveToTile, "move tile intent should map to MoveToTile");
			Expect(command.actorId == Id("actor:player"), "move tile intent should use selected actor");
			Expect(command.targetTile == iggy::TileCoord { 7, 9 }, "move tile intent should preserve tile target");
		}
	}
}

void TestMoveToolOnPointCanUseExplicitActor()
{
	iggy::ui::UiFeatureContext context;
	context.activeToolId = Id("tool:move");
	iggy::ui::UiToolIntent intent;
	intent.type = iggy::ui::UiToolIntentType::UseActiveToolOnPoint;
	intent.actorId = Id("actor:npc");
	intent.targetPoint = { 3.0F, 4.0F };

	const iggy::ui::UiActionPlan plan = iggy::ui::buildUiToolIntentActionPlan(
		intent,
		context,
		iggy::ui::defaultUiToolInventory());

	Expect(plan.actions.size() == 1, "move point intent should emit one command-frame action");
	if (plan.actions.size() == 1 && !plan.actions[0].commandFrame.commands.empty()) {
		const iggy::runtime::GameplayCommand2D &command = plan.actions[0].commandFrame.commands[0];
		Expect(command.type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "move point intent should map to MoveToPoint");
		Expect(command.actorId == Id("actor:npc"), "explicit actor should override context actor");
		Expect(NearVec(command.targetPoint, { 3.0F, 4.0F }), "move point intent should preserve point target");
	}
}

void TestInteractToolOnTargetPlansGameplayCommandFrame()
{
	iggy::ui::UiToolIntent intent;
	intent.type = iggy::ui::UiToolIntentType::UseActiveToolOnTarget;
	intent.targetId = Id("target:door");

	const iggy::ui::UiActionPlan plan = iggy::ui::buildUiToolIntentActionPlan(
		intent,
		ContextWithPlayer(Id("tool:interact")),
		iggy::ui::defaultUiToolInventory());

	Expect(plan.actions.size() == 1, "interact tool on target should emit one command-frame action");
	if (plan.actions.size() == 1 && !plan.actions[0].commandFrame.commands.empty()) {
		const iggy::runtime::GameplayCommand2D &command = plan.actions[0].commandFrame.commands[0];
		Expect(command.type == iggy::runtime::GameplayCommand2DType::Interact, "interact target intent should map to Interact");
		Expect(command.actorId == Id("actor:player"), "interact target intent should use selected actor");
		Expect(command.targetId == Id("target:door"), "interact target intent should preserve target id");
	}
}

void TestInspectToolOnTargetPlansSelectionOnly()
{
	iggy::ui::UiToolIntent intent;
	intent.type = iggy::ui::UiToolIntentType::UseActiveToolOnTarget;
	intent.targetId = Id("target:chest");

	const iggy::ui::UiActionPlan plan = iggy::ui::buildUiToolIntentActionPlan(
		intent,
		ContextWithPlayer(Id("tool:inspect")),
		iggy::ui::defaultUiToolInventory());

	Expect(plan.actions.size() == 1, "inspect tool on target should emit one selection action");
	if (plan.actions.size() == 1) {
		Expect(plan.actions[0].type == iggy::ui::UiActionType::SelectTarget, "inspect tool should select target");
		Expect(plan.actions[0].resourceId == Id("target:chest"), "inspect tool should preserve selected target");
		Expect(plan.actions[0].commandFrame.commands.empty(), "inspect tool should not queue gameplay commands");
	}
}

void TestMissingActorProducesStatusInsteadOfCommand()
{
	iggy::ui::UiFeatureContext context;
	context.activeToolId = Id("tool:move");
	iggy::ui::UiToolIntent intent;
	intent.type = iggy::ui::UiToolIntentType::UseActiveToolOnTile;
	intent.targetTile = { 1, 2 };

	const iggy::ui::UiActionPlan plan = iggy::ui::buildUiToolIntentActionPlan(
		intent,
		context,
		iggy::ui::defaultUiToolInventory());

	Expect(plan.actions.size() == 1, "missing actor should emit one diagnostic action");
	if (plan.actions.size() == 1) {
		Expect(plan.actions[0].type == iggy::ui::UiActionType::SetStatusText, "missing actor should not queue command");
		Expect(plan.actions[0].commandFrame.commands.empty(), "missing actor action should not contain commands");
	}
}

void TestExecuteActionPlanCallsAvailableShellActions()
{
	iggy::ui::UiActionPlan plan;
	iggy::ui::UiAction activeTool;
	activeTool.type = iggy::ui::UiActionType::SetActiveTool;
	activeTool.resourceId = Id("tool:pickup");
	plan.actions.push_back(activeTool);

	iggy::ui::UiAction save;
	save.type = iggy::ui::UiActionType::RequestSave;
	plan.actions.push_back(save);

	iggy::ResourceId selectedTool;
	bool saved = false;
	iggy::ui::UiShellActions actions;
	actions.setActiveTool = [&selectedTool](iggy::ResourceId toolId) {
		selectedTool = toolId;
	};
	actions.requestSave = [&saved]() {
		saved = true;
	};

	iggy::ui::executeUiActionPlan(plan, actions);

	Expect(selectedTool == Id("tool:pickup"), "executor should call active tool hook");
	Expect(saved, "executor should call save hook");
}

} // namespace

int main()
{
	TestActivateToolPlansSetActiveTool();
	TestUnknownToolPlansStatusOnly();
	TestMoveToolOnTilePlansGameplayCommandFrame();
	TestMoveToolOnPointCanUseExplicitActor();
	TestInteractToolOnTargetPlansGameplayCommandFrame();
	TestInspectToolOnTargetPlansSelectionOnly();
	TestMissingActorProducesStatusInsteadOfCommand();
	TestExecuteActionPlanCallsAvailableShellActions();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
