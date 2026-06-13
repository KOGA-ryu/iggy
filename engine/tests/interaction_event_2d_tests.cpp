#include <cstdlib>

#include "scene/interaction/InteractionEvent2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

bool SameEvent(const iggy::InteractionEvent2D &actual, const iggy::InteractionEvent2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

void TestTargetToggledFactoryPreservesPayload()
{
	const iggy::ResourceId targetId { "target:door" };
	const iggy::InteractionEvent2D disabled = iggy::targetToggledInteractionEvent(targetId, false);
	const iggy::InteractionEvent2D enabled = iggy::targetToggledInteractionEvent(targetId, true);

	Expect(disabled.type == iggy::InteractionEvent2DType::TargetToggled, "target toggled event should preserve type");
	Expect(disabled.targetId == targetId, "target toggled event should preserve target id");
	Expect(!disabled.enabledValue, "target toggled event should preserve disabled value");
	Expect(enabled.enabledValue, "target toggled event should preserve enabled value");
	Expect(disabled.eventId.empty(), "target toggled event should leave event id defaulted");
	Expect(disabled.text.empty(), "target toggled event should leave text defaulted");
}

void TestInspectTextRequestedFactoryPreservesPayload()
{
	const iggy::ResourceId targetId { "target:sign" };
	const iggy::InteractionEvent2D event = iggy::inspectTextRequestedInteractionEvent(targetId, "Read me");

	Expect(event.type == iggy::InteractionEvent2DType::InspectTextRequested, "inspect text event should preserve type");
	Expect(event.targetId == targetId, "inspect text event should preserve target id");
	Expect(event.text == "Read me", "inspect text event should preserve text");
	Expect(event.eventId.empty(), "inspect text event should leave event id defaulted");
	Expect(event.enabledValue, "inspect text event should leave enabled value defaulted");
}

void TestEventEmittedFactoryPreservesPayload()
{
	const iggy::ResourceId targetId { "target:lever" };
	const iggy::ResourceId eventId { "event:lever_pulled" };
	const iggy::InteractionEvent2D event = iggy::interactionEventEmitted(targetId, eventId);

	Expect(event.type == iggy::InteractionEvent2DType::EventEmitted, "event emitted interaction event should preserve type");
	Expect(event.targetId == targetId, "event emitted interaction event should preserve target id");
	Expect(event.eventId == eventId, "event emitted interaction event should preserve event id");
	Expect(event.text.empty(), "event emitted interaction event should leave text defaulted");
	Expect(event.enabledValue, "event emitted interaction event should leave enabled value defaulted");
}

void TestRecorderAppendsEventsInOrder()
{
	const iggy::InteractionEvent2D first = iggy::targetToggledInteractionEvent(iggy::ResourceId { "target:door" }, false);
	const iggy::InteractionEvent2D second = iggy::inspectTextRequestedInteractionEvent(iggy::ResourceId { "target:sign" }, "Read");
	const iggy::InteractionEvent2D third = iggy::interactionEventEmitted(iggy::ResourceId { "target:lever" }, iggy::ResourceId { "event:lever" });
	iggy::InteractionEventRecorder2D recorder;

	Expect(recorder.events.empty(), "interaction event recorder should start empty");
	iggy::recordInteractionEvent(recorder, first);
	iggy::recordInteractionEvent(recorder, second);
	iggy::recordInteractionEvent(recorder, third);

	Expect(recorder.events.size() == 3, "interaction event recorder should append events");
	if (recorder.events.size() == 3) {
		Expect(SameEvent(recorder.events[0], first), "interaction event recorder should preserve first event");
		Expect(SameEvent(recorder.events[1], second), "interaction event recorder should preserve second event");
		Expect(SameEvent(recorder.events[2], third), "interaction event recorder should preserve third event");
	}
}

void TestRecorderCopiesEventData()
{
	iggy::InteractionEvent2D source = iggy::inspectTextRequestedInteractionEvent(iggy::ResourceId { "target:note" }, "Original");
	iggy::InteractionEventRecorder2D recorder;

	iggy::recordInteractionEvent(recorder, source);
	source.targetId = iggy::ResourceId { "target:changed" };
	source.text = "Changed";
	source.enabledValue = false;

	Expect(recorder.events.size() == 1, "interaction event recorder copy setup should record one event");
	if (recorder.events.size() == 1) {
		Expect(recorder.events[0].targetId == iggy::ResourceId { "target:note" }, "interaction event recorder should copy target id");
		Expect(recorder.events[0].text == "Original", "interaction event recorder should copy text");
		Expect(recorder.events[0].enabledValue, "interaction event recorder should copy enabled value");
	}
}

void TestEmptyPayloadsArePreservedAsData()
{
	const iggy::InteractionEvent2D toggled = iggy::targetToggledInteractionEvent({}, false);
	const iggy::InteractionEvent2D inspect = iggy::inspectTextRequestedInteractionEvent({}, "");
	const iggy::InteractionEvent2D emitted = iggy::interactionEventEmitted({}, {});

	Expect(toggled.targetId.empty(), "target toggled event should preserve empty target id");
	Expect(!toggled.enabledValue, "target toggled event should preserve empty target enabled value");
	Expect(inspect.targetId.empty(), "inspect text event should preserve empty target id");
	Expect(inspect.text.empty(), "inspect text event should preserve empty text");
	Expect(emitted.targetId.empty(), "event emitted interaction event should preserve empty target id");
	Expect(emitted.eventId.empty(), "event emitted interaction event should preserve empty event id");
}

} // namespace

int main()
{
	TestTargetToggledFactoryPreservesPayload();
	TestInspectTextRequestedFactoryPreservesPayload();
	TestEventEmittedFactoryPreservesPayload();
	TestRecorderAppendsEventsInOrder();
	TestRecorderCopiesEventData();
	TestEmptyPayloadsArePreservedAsData();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
