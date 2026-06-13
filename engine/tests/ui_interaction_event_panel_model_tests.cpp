#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionEvent2D.hpp"
#include "scene/ui/UiInteractionEventPanelModel.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

void TestEmptyEventsProduceEmptyModel()
{
	const iggy::ui::UiInteractionEventPanelModel model = iggy::ui::buildUiInteractionEventPanelModel({});

	Expect(model.empty(), "empty interaction event panel model should report empty");
	Expect(model.targetToggleCount == 0, "empty interaction event panel model should have zero toggles");
	Expect(model.inspectTextRequestCount == 0, "empty interaction event panel model should have zero inspect requests");
	Expect(model.emittedEventCount == 0, "empty interaction event panel model should have zero emitted events");
}

void TestTypeLabelsAreStable()
{
	Expect(iggy::ui::uiInteractionEventTypeLabel(iggy::InteractionEvent2DType::TargetToggled) == "target_toggled", "target toggled UI label should be stable");
	Expect(iggy::ui::uiInteractionEventTypeLabel(iggy::InteractionEvent2DType::InspectTextRequested) == "inspect_text_requested", "inspect text UI label should be stable");
	Expect(iggy::ui::uiInteractionEventTypeLabel(iggy::InteractionEvent2DType::EventEmitted) == "event_emitted", "event emitted UI label should be stable");
}

void TestModelProjectsRowsAndCounts()
{
	const std::vector<iggy::InteractionEvent2D> events {
		iggy::targetToggledInteractionEvent(Id("target:door"), false),
		iggy::inspectTextRequestedInteractionEvent(Id("target:sign"), "Hello"),
		iggy::interactionEventEmitted(Id("target:lever"), Id("event:lever_pulled")),
	};

	const iggy::ui::UiInteractionEventPanelModel model = iggy::ui::buildUiInteractionEventPanelModel(events);

	Expect(!model.empty(), "interaction event panel model should report non-empty rows");
	Expect(model.rows.size() == 3, "interaction event panel model should project one row per event");
	Expect(model.targetToggleCount == 1, "interaction event panel model should count target toggles");
	Expect(model.inspectTextRequestCount == 1, "interaction event panel model should count inspect requests");
	Expect(model.emittedEventCount == 1, "interaction event panel model should count emitted events");
	if (model.rows.size() == 3) {
		Expect(model.rows[0].typeLabel == "target_toggled" && model.rows[0].targetId == Id("target:door"), "interaction event panel row should preserve toggle target");
		Expect(model.rows[0].hasEnabledValue && !model.rows[0].enabledValue && model.rows[0].detail == "disabled", "interaction event panel toggle row should expose enabled value");
		Expect(model.rows[1].typeLabel == "inspect_text_requested" && model.rows[1].detail == "Hello", "interaction event panel inspect row should expose text detail");
		Expect(model.rows[2].typeLabel == "event_emitted" && model.rows[2].eventId == Id("event:lever_pulled") && model.rows[2].detail == "event:lever_pulled", "interaction event panel emitted row should expose event id detail");
	}
}

void TestProjectionDoesNotMutateEvents()
{
	std::vector<iggy::InteractionEvent2D> events {
		iggy::inspectTextRequestedInteractionEvent(Id("target:book"), "Read"),
	};

	const iggy::ui::UiInteractionEventPanelModel model = iggy::ui::buildUiInteractionEventPanelModel(events);

	Expect(model.rows.size() == 1, "interaction event panel immutability setup should project row");
	Expect(events.size() == 1 && events[0].targetId == Id("target:book") && events[0].text == "Read", "interaction event panel projection should not mutate source events");
}

} // namespace

int main()
{
	TestEmptyEventsProduceEmptyModel();
	TestTypeLabelsAreStable();
	TestModelProjectsRowsAndCounts();
	TestProjectionDoesNotMutateEvents();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
