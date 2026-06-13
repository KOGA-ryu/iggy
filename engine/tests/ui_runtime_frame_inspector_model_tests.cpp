#include <cstdlib>

#include "runtime/RuntimePlayerInputInteractionEffectFrameReport.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/ui/UiRuntimeFrameInspectorModel.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestEventLabelsAreStable()
{
	Expect(iggy::ui::uiRuntimeInteractionEffectFrameEventLabel(iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::PlayerCommandAccepted) == "player_command_accepted", "runtime inspector event label should name accepted command");
	Expect(iggy::ui::uiRuntimeInteractionEffectFrameEventLabel(iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::InteractionReadyWithEffects) == "interaction_ready_with_effects", "runtime inspector event label should name ready interaction with effects");
	Expect(iggy::ui::uiRuntimeInteractionEffectFrameEventLabel(iggy::runtime::RuntimePlayerInputInteractionEffectFrameEvent::EffectsRequested) == "effects_requested", "runtime inspector event label should name requested effects");
}

void TestNullInputsProduceEmptyButDisplayableRows()
{
	const iggy::ui::UiRuntimeFrameInspectorModel model =
		iggy::ui::buildUiRuntimeFrameInspectorModel(nullptr, nullptr);

	Expect(!model.hasSession, "runtime inspector should report missing session");
	Expect(model.tickIndex == 0, "runtime inspector should default tick index");
	Expect(model.rows.size() == 11, "runtime inspector should still produce display rows for missing inputs");
	if (!model.rows.empty())
		Expect(model.rows[0].key == "has_session" && model.rows[0].value == "false", "runtime inspector first row should expose session presence");
}

void TestModelProjectsSessionAndFrameCounts()
{
	iggy::runtime::RuntimeSessionState session;
	session.tickIndex = 42;
	session.hasPlayer = true;
	session.hasRenderCache = true;
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report;
	report.acceptedCommandCount = 3;
	report.blockedIntentCount = 1;
	report.rejectedIntentCount = 2;
	report.readyInteractionCount = 4;
	report.noEffectInteractionCount = 5;
	report.blockedInteractionCount = 6;
	report.requestedEffectCount = 7;

	const iggy::ui::UiRuntimeFrameInspectorModel model =
		iggy::ui::buildUiRuntimeFrameInspectorModel(&session, &report);

	Expect(model.hasSession, "runtime inspector should report session presence");
	Expect(model.tickIndex == 42, "runtime inspector should preserve tick index");
	Expect(model.hasPlayer, "runtime inspector should preserve player presence");
	Expect(model.hasRenderCache, "runtime inspector should preserve render cache presence");
	Expect(model.acceptedCommandCount == 3, "runtime inspector should preserve accepted command count");
	Expect(model.blockedIntentCount == 1, "runtime inspector should preserve blocked intent count");
	Expect(model.rejectedIntentCount == 2, "runtime inspector should preserve rejected intent count");
	Expect(model.readyInteractionCount == 4, "runtime inspector should preserve ready interaction count");
	Expect(model.noEffectInteractionCount == 5, "runtime inspector should preserve no-effect interaction count");
	Expect(model.blockedInteractionCount == 6, "runtime inspector should preserve blocked interaction count");
	Expect(model.requestedEffectCount == 7, "runtime inspector should preserve requested effect count");
}

void TestProjectionDoesNotMutateInputs()
{
	iggy::runtime::RuntimeSessionState session;
	session.tickIndex = 9;
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report;
	report.acceptedCommandCount = 1;

	(void)iggy::ui::buildUiRuntimeFrameInspectorModel(&session, &report);

	Expect(session.tickIndex == 9, "runtime inspector projection should not mutate session");
	Expect(report.acceptedCommandCount == 1, "runtime inspector projection should not mutate report");
}

} // namespace

int main()
{
	TestEventLabelsAreStable();
	TestNullInputsProduceEmptyButDisplayableRows();
	TestModelProjectsSessionAndFrameCounts();
	TestProjectionDoesNotMutateInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
