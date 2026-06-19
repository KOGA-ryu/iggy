#include <cstdlib>

#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"
#include "runtime/RuntimeGameplayProductInputFrameTargetContext.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectFrameReport.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEvent2D.hpp"
#include "scene/ui/UiFeatureContext.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestEmptyFeatureContextHasNoRuntimePointers()
{
	const iggy::ui::UiFeatureContext context;

	Expect(!iggy::ui::uiFeatureContextHasSession(context), "empty ui feature context should not have session");
	Expect(!iggy::ui::uiFeatureContextHasFrameReport(context), "empty ui feature context should not have frame report");
	Expect(!iggy::ui::uiFeatureContextHasInteractionEvents(context), "empty ui feature context should not have interaction events");
	Expect(!iggy::ui::uiFeatureContextHasAuthoringPreview(context), "empty ui feature context should not have authoring preview");
	Expect(!iggy::ui::uiFeatureContextHasProductPlayMode(context), "empty ui feature context should not have product play mode");
}

void TestFeatureContextReportsPresentRuntimePointers()
{
	iggy::runtime::RuntimeSessionState session;
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report;
	iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview;
	iggy::runtime::RuntimeGameplayProductPlayModeBuildResult productBuild;
	iggy::runtime::RuntimeGameplayProductPlayModeState productState;
	iggy::runtime::RuntimeGameplayProductPlayModeFrameResult productFrame;
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult targetContext;
	iggy::InteractionEventRecorder2D events;
	iggy::ui::UiFeatureContext context;
	context.session = &session;
	context.latestFrameReport = &report;
	context.interactionEvents = &events;
	context.authoringPreview = &preview;
	context.productPlayModeBuild = &productBuild;
	context.productPlayModeState = &productState;
	context.latestProductPlayModeFrame = &productFrame;
	context.latestProductInputFrameTargetContext = &targetContext;
	context.activeToolId = iggy::ResourceId { "tool:inspect" };
	context.selectedActorId = iggy::ResourceId { "actor:player" };
	context.selectedTargetId = iggy::ResourceId { "target:door" };

	Expect(iggy::ui::uiFeatureContextHasSession(context), "ui feature context should report session pointer");
	Expect(iggy::ui::uiFeatureContextHasFrameReport(context), "ui feature context should report frame report pointer");
	Expect(iggy::ui::uiFeatureContextHasInteractionEvents(context), "ui feature context should report interaction event pointer");
	Expect(iggy::ui::uiFeatureContextHasAuthoringPreview(context), "ui feature context should report authoring preview pointer");
	Expect(iggy::ui::uiFeatureContextHasProductPlayMode(context), "ui feature context should report product play mode pointer");
	Expect(context.activeToolId == iggy::ResourceId { "tool:inspect" }, "ui feature context should preserve active tool id");
	Expect(context.selectedActorId == iggy::ResourceId { "actor:player" }, "ui feature context should preserve selected actor id");
	Expect(context.selectedTargetId == iggy::ResourceId { "target:door" }, "ui feature context should preserve selected target id");
}

void TestProductPlayContextReportsAnyPresentPointer()
{
	iggy::runtime::RuntimeGameplayProductPlayModeBuildResult productBuild;
	iggy::runtime::RuntimeGameplayProductPlayModeState productState;
	iggy::runtime::RuntimeGameplayProductPlayModeFrameResult productFrame;
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult targetContext;

	iggy::ui::UiFeatureContext context;
	context.latestProductInputFrameTargetContext = &targetContext;
	Expect(!iggy::ui::uiFeatureContextHasProductPlayMode(context), "ui feature context should not report product play mode from diagnostics pointer alone");

	context.productPlayModeBuild = &productBuild;
	Expect(iggy::ui::uiFeatureContextHasProductPlayMode(context), "ui feature context should report product play build pointer");

	context.productPlayModeBuild = nullptr;
	context.latestProductInputFrameTargetContext = nullptr;
	context.productPlayModeState = &productState;
	Expect(iggy::ui::uiFeatureContextHasProductPlayMode(context), "ui feature context should report product play state pointer");

	context.productPlayModeState = nullptr;
	context.latestProductPlayModeFrame = &productFrame;
	Expect(iggy::ui::uiFeatureContextHasProductPlayMode(context), "ui feature context should report latest product play frame pointer");
}

void TestShellActionsAreOptionalCallables()
{
	iggy::ui::UiShellActions actions;
	bool selected = false;
	actions.selectTarget = [&selected](iggy::ResourceId targetId) {
		selected = targetId == iggy::ResourceId { "target:lever" };
	};

	Expect(!actions.queueCommandFrame, "ui shell actions should allow absent command queue hook");
	Expect(static_cast<bool>(actions.selectTarget), "ui shell actions should preserve supplied target selection hook");
	actions.selectTarget(iggy::ResourceId { "target:lever" });
	Expect(selected, "ui shell action callable should be invokable by host code");
}

} // namespace

int main()
{
	TestEmptyFeatureContextHasNoRuntimePointers();
	TestFeatureContextReportsPresentRuntimePointers();
	TestProductPlayContextReportsAnyPresentPointer();
	TestShellActionsAreOptionalCallables();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
