#include <cstdlib>

#include "scene/interaction/InteractionEffect2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestDefaultNoneEffectIsValid()
{
	const iggy::InteractionEffect2D effect;

	Expect(effect.type == iggy::InteractionEffect2DType::None, "default interaction effect should be None");
	Expect(effect.targetId.empty(), "default interaction effect should have empty target id");
	Expect(effect.eventId.empty(), "default interaction effect should have empty event id");
	Expect(effect.dropId.empty(), "default interaction effect should have empty drop id");
	Expect(effect.text.empty(), "default interaction effect should have empty text");
	Expect(effect.enabledValue, "default interaction effect should preserve default enabled value");
	Expect(iggy::validate(effect) == iggy::InteractionEffect2DStatus::Valid, "default interaction effect should validate");
	Expect(iggy::valid(effect), "default interaction effect should be valid");

	const iggy::InteractionEffect2D none = iggy::noneInteractionEffect();
	Expect(none.type == iggy::InteractionEffect2DType::None, "none factory should produce None effect");
	Expect(iggy::valid(none), "none factory effect should be valid");
}

void TestInspectTextFactoryPreservesTargetAndText()
{
	const iggy::ResourceId targetId { "target:sign" };
	const iggy::InteractionEffect2D effect = iggy::inspectTextInteractionEffect(targetId, "Read me");

	Expect(effect.type == iggy::InteractionEffect2DType::InspectText, "inspect text effect should preserve type");
	Expect(effect.targetId == targetId, "inspect text effect should preserve target id");
	Expect(effect.text == "Read me", "inspect text effect should preserve text");
	Expect(effect.eventId.empty(), "inspect text effect should leave event id defaulted");
	Expect(effect.dropId.empty(), "inspect text effect should leave drop id defaulted");
	Expect(effect.enabledValue, "inspect text effect should leave enabled value defaulted");
	Expect(iggy::validate(effect) == iggy::InteractionEffect2DStatus::Valid, "inspect text with text should validate");
	Expect(iggy::valid(effect), "inspect text with text should be valid");
}

void TestInspectTextMissingTextInvalid()
{
	const iggy::InteractionEffect2D effect = iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:sign" }, "");

	Expect(effect.type == iggy::InteractionEffect2DType::InspectText, "empty inspect text effect should preserve type");
	Expect(iggy::validate(effect) == iggy::InteractionEffect2DStatus::MissingText, "inspect text without text should report MissingText");
	Expect(!iggy::valid(effect), "inspect text without text should not be valid");
}

void TestToggleTargetFactoryPreservesTargetAndEnabledValue()
{
	const iggy::ResourceId targetId { "target:door" };
	const iggy::InteractionEffect2D disabled = iggy::toggleTargetInteractionEffect(targetId, false);
	const iggy::InteractionEffect2D enabled = iggy::toggleTargetInteractionEffect(targetId, true);

	Expect(disabled.type == iggy::InteractionEffect2DType::ToggleTarget, "toggle effect should preserve type");
	Expect(disabled.targetId == targetId, "toggle effect should preserve target id");
	Expect(!disabled.enabledValue, "toggle effect should preserve disabled value");
	Expect(enabled.enabledValue, "toggle effect should preserve enabled value");
	Expect(disabled.eventId.empty(), "toggle effect should leave event id defaulted");
	Expect(disabled.dropId.empty(), "toggle effect should leave drop id defaulted");
	Expect(disabled.text.empty(), "toggle effect should leave text defaulted");
	Expect(iggy::valid(disabled), "toggle effect with target should be valid");
	Expect(iggy::valid(enabled), "toggle enabled effect with target should be valid");
}

void TestToggleTargetRequiresTarget()
{
	const iggy::InteractionEffect2D effect = iggy::toggleTargetInteractionEffect({}, false);

	Expect(effect.type == iggy::InteractionEffect2DType::ToggleTarget, "toggle effect without target should preserve type");
	Expect(iggy::validate(effect) == iggy::InteractionEffect2DStatus::MissingTarget, "toggle effect without target should report MissingTarget");
	Expect(!iggy::valid(effect), "toggle effect without target should not be valid");
}

void TestEmitEventFactoryPreservesTargetAndEvent()
{
	const iggy::ResourceId targetId { "target:lever" };
	const iggy::ResourceId eventId { "event:lever_pulled" };
	const iggy::InteractionEffect2D effect = iggy::emitInteractionEventEffect(targetId, eventId);

	Expect(effect.type == iggy::InteractionEffect2DType::EmitEvent, "emit event effect should preserve type");
	Expect(effect.targetId == targetId, "emit event effect should preserve target id");
	Expect(effect.eventId == eventId, "emit event effect should preserve event id");
	Expect(effect.dropId.empty(), "emit event effect should leave drop id defaulted");
	Expect(effect.text.empty(), "emit event effect should leave text defaulted");
	Expect(effect.enabledValue, "emit event effect should leave enabled value defaulted");
	Expect(iggy::validate(effect) == iggy::InteractionEffect2DStatus::Valid, "emit event effect with event id should validate");
	Expect(iggy::valid(effect), "emit event effect with event id should be valid");
}

void TestEmitEventRequiresEvent()
{
	const iggy::InteractionEffect2D effect = iggy::emitInteractionEventEffect(iggy::ResourceId { "target:lever" }, {});

	Expect(effect.type == iggy::InteractionEffect2DType::EmitEvent, "emit event effect without event should preserve type");
	Expect(iggy::validate(effect) == iggy::InteractionEffect2DStatus::MissingEvent, "emit event effect without event should report MissingEvent");
	Expect(!iggy::valid(effect), "emit event effect without event should not be valid");
}

void TestEmptyTargetAllowedForInspectTextAndEmitEvent()
{
	const iggy::InteractionEffect2D inspect = iggy::inspectTextInteractionEffect({}, "Loose note");
	const iggy::InteractionEffect2D event = iggy::emitInteractionEventEffect({}, iggy::ResourceId { "event:global" });

	Expect(inspect.targetId.empty(), "inspect effect should preserve empty target id as data");
	Expect(event.targetId.empty(), "emit event effect should preserve empty target id as data");
	Expect(iggy::valid(inspect), "inspect effect should allow empty target id");
	Expect(iggy::valid(event), "emit event effect should allow empty target id");
}

void TestPickupItemFactoryPreservesTargetAndDrop()
{
	const iggy::ResourceId targetId { "target:chest" };
	const iggy::ResourceId dropId { "drop:potion" };
	const iggy::InteractionEffect2D effect = iggy::pickupItemInteractionEffect(targetId, dropId);

	Expect(effect.type == iggy::InteractionEffect2DType::PickupItem, "pickup item effect should preserve type");
	Expect(effect.targetId == targetId, "pickup item effect should preserve target id");
	Expect(effect.dropId == dropId, "pickup item effect should preserve drop id");
	Expect(effect.eventId.empty(), "pickup item effect should leave event id defaulted");
	Expect(effect.text.empty(), "pickup item effect should leave text defaulted");
	Expect(effect.enabledValue, "pickup item effect should leave enabled value defaulted");
	Expect(iggy::validate(effect) == iggy::InteractionEffect2DStatus::Valid, "pickup item with drop id should validate");
	Expect(iggy::valid(effect), "pickup item with drop id should be valid");
}

void TestPickupItemRequiresDrop()
{
	const iggy::InteractionEffect2D effect = iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, {});

	Expect(effect.type == iggy::InteractionEffect2DType::PickupItem, "pickup item effect without drop should preserve type");
	Expect(iggy::validate(effect) == iggy::InteractionEffect2DStatus::MissingDropId, "pickup item without drop should report MissingDropId");
	Expect(!iggy::valid(effect), "pickup item without drop should not be valid");
}

void TestEmptyTargetAllowedForPickupItem()
{
	const iggy::InteractionEffect2D effect = iggy::pickupItemInteractionEffect({}, iggy::ResourceId { "drop:potion" });

	Expect(effect.targetId.empty(), "pickup item should preserve empty target id as data");
	Expect(effect.dropId == iggy::ResourceId { "drop:potion" }, "pickup item should preserve drop id with empty target");
	Expect(iggy::valid(effect), "pickup item should allow empty target id");
}

void TestValidMirrorsValidateStatus()
{
	const iggy::InteractionEffect2D validEffect = iggy::inspectTextInteractionEffect({}, "Text");
	const iggy::InteractionEffect2D missingText = iggy::inspectTextInteractionEffect({}, "");
	const iggy::InteractionEffect2D missingTarget = iggy::toggleTargetInteractionEffect({}, true);
	const iggy::InteractionEffect2D missingEvent = iggy::emitInteractionEventEffect({}, {});
	const iggy::InteractionEffect2D missingDrop = iggy::pickupItemInteractionEffect({}, {});

	Expect(iggy::valid(validEffect) == (iggy::validate(validEffect) == iggy::InteractionEffect2DStatus::Valid), "valid mirror should match valid inspect text status");
	Expect(iggy::valid(missingText) == (iggy::validate(missingText) == iggy::InteractionEffect2DStatus::Valid), "valid mirror should match missing text status");
	Expect(iggy::valid(missingTarget) == (iggy::validate(missingTarget) == iggy::InteractionEffect2DStatus::Valid), "valid mirror should match missing target status");
	Expect(iggy::valid(missingEvent) == (iggy::validate(missingEvent) == iggy::InteractionEffect2DStatus::Valid), "valid mirror should match missing event status");
	Expect(iggy::valid(missingDrop) == (iggy::validate(missingDrop) == iggy::InteractionEffect2DStatus::Valid), "valid mirror should match missing drop status");
}

} // namespace

int main()
{
	TestDefaultNoneEffectIsValid();
	TestInspectTextFactoryPreservesTargetAndText();
	TestInspectTextMissingTextInvalid();
	TestToggleTargetFactoryPreservesTargetAndEnabledValue();
	TestToggleTargetRequiresTarget();
	TestEmitEventFactoryPreservesTargetAndEvent();
	TestEmitEventRequiresEvent();
	TestEmptyTargetAllowedForInspectTextAndEmitEvent();
	TestPickupItemFactoryPreservesTargetAndDrop();
	TestPickupItemRequiresDrop();
	TestEmptyTargetAllowedForPickupItem();
	TestValidMirrorsValidateStatus();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
