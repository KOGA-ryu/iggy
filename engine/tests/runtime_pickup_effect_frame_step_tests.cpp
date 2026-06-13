#include <cstdlib>
#include <vector>

#include "runtime/RuntimePickupEffectFrameStep.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { iggy::ResourceId { itemId }, count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime pickup effect frame inventory fixture should build");
	return result.inventory;
}

iggy::LevelItemDrop2D Drop(
	const char *id,
	const char *itemId = "item:potion",
	std::uint32_t count = 1,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float pickupRadius = 0.0F,
	bool enabled = true)
{
	return {
		iggy::ResourceId { id },
		iggy::ResourceId { itemId },
		count,
		position,
		pickupRadius,
		enabled,
	};
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return {
		Inventory(stacks),
		iggy::LevelItemDrop2DRegistry { drops },
	};
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = {
		iggy::ResourceId { "player:pickup-effect-frame" },
		position,
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::South,
	};
	session.tickIndex = 21;
	return session;
}

iggy::InteractionEffectPlanApplyEntry EffectEntry(std::size_t effectIndex, iggy::InteractionEffect2D effect)
{
	iggy::InteractionEffectPlanApplyEntry entry;
	entry.effectIndex = effectIndex;
	entry.result.status = iggy::InteractionEffectApplyStatus::Deferred;
	entry.result.effect = effect;
	return entry;
}

iggy::InteractionEffectPlanApplyResult PlanApplication(std::vector<iggy::InteractionEffectPlanApplyEntry> entries)
{
	iggy::InteractionEffectPlanApplyResult application;
	application.entries = entries;
	return application;
}

iggy::runtime::RuntimeInteractionEffectApplyFrameEntry InteractionEntry(
	std::size_t commandIndex,
	std::vector<iggy::InteractionEffectPlanApplyEntry> entries)
{
	iggy::runtime::RuntimeInteractionEffectApplyFrameEntry entry;
	entry.commandIndex = commandIndex;
	entry.result.application = PlanApplication(entries);
	return entry;
}

iggy::runtime::RuntimeInteractionEffectApplyFrameResult FrameApplication(
	std::vector<iggy::runtime::RuntimeInteractionEffectApplyFrameEntry> entries)
{
	iggy::runtime::RuntimeInteractionEffectApplyFrameResult application;
	application.entries = entries;
	return application;
}

bool SameStacks(
	const std::vector<iggy::InventoryItemStack2D> &actual,
	const std::vector<iggy::InventoryItemStack2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].itemId != expected[index].itemId || actual[index].count != expected[index].count)
			return false;
	}
	return true;
}

bool SameDrop(const iggy::LevelItemDrop2D &actual, const iggy::LevelItemDrop2D &expected)
{
	return actual.id == expected.id
		&& actual.itemId == expected.itemId
		&& actual.count == expected.count
		&& NearVec(actual.position, expected.position)
		&& actual.pickupRadius == expected.pickupRadius
		&& actual.enabled == expected.enabled;
}

bool SameDrops(
	const std::vector<iggy::LevelItemDrop2D> &actual,
	const std::vector<iggy::LevelItemDrop2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameDrop(actual[index], expected[index]))
			return false;
	}
	return true;
}

void ExpectInventoryState(
	const iggy::runtime::RuntimeInventoryState &actual,
	const iggy::runtime::RuntimeInventoryState &expected,
	const char *message)
{
	Expect(SameStacks(actual.inventory.stacks, expected.inventory.stacks), message);
	Expect(SameDrops(actual.drops.drops, expected.drops.drops), message);
}

void ExpectNoEvents(const iggy::runtime::RuntimePickupEffectFrameResult &result, const char *message)
{
	Expect(result.events.events.empty(), message);
}

void ExpectEvent(
	const iggy::InventoryEvent2D &event,
	iggy::InventoryEvent2DType type,
	const iggy::ResourceId &itemId,
	const iggy::ResourceId &dropId,
	std::uint32_t count,
	const char *message)
{
	Expect(event.type == type, message);
	Expect(event.itemId == itemId, message);
	Expect(event.dropId == dropId, message);
	Expect(event.count == count, message);
}

void ExpectSuccessfulPickupEventsAt(
	const iggy::runtime::RuntimePickupEffectFrameResult &result,
	std::size_t offset,
	const iggy::LevelItemDrop2D &drop,
	const char *message)
{
	Expect(result.events.events.size() >= offset + 3, message);
	if (result.events.events.size() >= offset + 3) {
		ExpectEvent(result.events.events[offset], iggy::InventoryEvent2DType::ItemAdded, drop.itemId, {}, drop.count, message);
		ExpectEvent(result.events.events[offset + 1], iggy::InventoryEvent2DType::DropConsumed, {}, drop.id, 0, message);
		ExpectEvent(result.events.events[offset + 2], iggy::InventoryEvent2DType::ItemPickedUp, drop.itemId, drop.id, drop.count, message);
	}
}

void TestNoInteractionEntriesOrNoPickupEffectsReturnsNoPickupEffects()
{
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult emptyApplication = FrameApplication({});
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult nonPickupApplication = FrameApplication({
		InteractionEntry(
			0,
			{ EffectEntry(4, iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:sign" }, "Read me")) }),
	});

	const iggy::runtime::RuntimePickupEffectFrameResult emptyResult =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, emptyApplication);
	const iggy::runtime::RuntimePickupEffectFrameResult nonPickupResult =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, nonPickupApplication);

	Expect(emptyResult.status == iggy::runtime::RuntimePickupEffectFrameStatus::NoPickupEffects, "empty pickup effect frame should have no pickup effects");
	Expect(emptyResult.entries.empty(), "empty pickup effect frame should produce no entries");
	Expect(!emptyResult.changed, "empty pickup effect frame should not change");
	ExpectInventoryState(emptyResult.inventory, inventory, "empty pickup effect frame should preserve inventory");
	ExpectNoEvents(emptyResult, "empty pickup effect frame should not record inventory events");
	Expect(nonPickupResult.status == iggy::runtime::RuntimePickupEffectFrameStatus::NoPickupEffects, "non-pickup effect frame should have no pickup effects");
	Expect(nonPickupResult.entries.empty(), "non-pickup effect frame should not append entries");
	ExpectInventoryState(nonPickupResult.inventory, inventory, "non-pickup effect frame should preserve inventory");
	ExpectNoEvents(nonPickupResult, "non-pickup effect frame should not record inventory events");
}

void TestSinglePickupItemPicksUpAndUpdatesInventory()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(
			7,
			{ EffectEntry(3, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, drop.id)) }),
	});

	const iggy::runtime::RuntimePickupEffectFrameResult result =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, application);

	Expect(result.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "single pickup effect should pick up");
	Expect(result.pickedUpCount == 1, "single pickup effect should count pickup");
	Expect(result.notReadyCount == 0, "single pickup effect should not count not-ready");
	Expect(result.failedCount == 0, "single pickup effect should not count failure");
	Expect(result.changed, "single pickup effect should mark changed");
	Expect(result.entries.size() == 1, "single pickup effect should append one entry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].interactionIndex == 0, "single pickup effect should preserve interaction entry index");
		Expect(result.entries[0].effectIndex == 3, "single pickup effect should preserve effect index");
		Expect(result.entries[0].result.status == iggy::runtime::RuntimePickupEffectStatus::PickedUp, "single pickup effect should preserve pickup result");
	}
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "single pickup effect should add item");
	Expect(result.inventory.drops.drops.size() == 1, "single pickup effect should keep disabled drop");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "single pickup effect should disable drop");
	}
	Expect(result.events.events.size() == 3, "single pickup effect should aggregate pickup events");
	ExpectSuccessfulPickupEventsAt(result, 0, drop, "single pickup effect should preserve pickup event order");
}

void TestMultiplePickupEffectsCarryInventoryForwardInOrder()
{
	const iggy::LevelItemDrop2D potion = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2D key = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 1) }, { potion, key });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(
			2,
			{ EffectEntry(0, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, potion.id)) }),
		InteractionEntry(
			4,
			{ EffectEntry(8, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:crate" }, key.id)) }),
	});

	const iggy::runtime::RuntimePickupEffectFrameResult result =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, application);

	Expect(result.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "multiple pickup effects should pick up");
	Expect(result.pickedUpCount == 2, "multiple pickup effects should count both pickups");
	Expect(result.entries.size() == 2, "multiple pickup effects should append entries in order");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].interactionIndex == 0 && result.entries[0].effectIndex == 0, "first pickup effect should preserve indexes");
		Expect(result.entries[1].interactionIndex == 1 && result.entries[1].effectIndex == 8, "second pickup effect should preserve indexes");
	}
	Expect(SameStacks(
			   result.inventory.inventory.stacks,
			   {
				   Stack("item:potion", 3),
				   Stack("item:key", 1),
			   }),
		"multiple pickup effects should carry inventory forward");
	Expect(result.inventory.drops.drops.size() == 2, "multiple pickup effects should preserve disabled drops");
	if (result.inventory.drops.drops.size() == 2) {
		Expect(!result.inventory.drops.drops[0].enabled, "first pickup effect should disable first drop");
		Expect(!result.inventory.drops.drops[1].enabled, "second pickup effect should disable second drop");
	}
	Expect(result.events.events.size() == 6, "multiple pickup effects should aggregate all pickup events");
	ExpectSuccessfulPickupEventsAt(result, 0, potion, "multiple pickup effects should preserve first pickup event order");
	ExpectSuccessfulPickupEventsAt(result, 3, key, "multiple pickup effects should preserve second pickup event order");
}

void TestNotReadyPickupEffectsContinueToLaterPickups()
{
	const iggy::LevelItemDrop2D disabled = Drop("drop:disabled", "item:old", 1, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::LevelItemDrop2D far = Drop("drop:far", "item:far", 1, { 5.0F, 0.0F }, 1.0F, true);
	const iggy::LevelItemDrop2D ready = Drop("drop:ready", "item:ready", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { disabled, far, ready });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(
			1,
			{ EffectEntry(0, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:disabled" }, disabled.id)) }),
		InteractionEntry(
			2,
			{ EffectEntry(1, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:far" }, far.id)) }),
		InteractionEntry(
			3,
			{ EffectEntry(2, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:ready" }, ready.id)) }),
	});

	const iggy::runtime::RuntimePickupEffectFrameResult result =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, application);

	Expect(result.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "not-ready pickup effects should continue to later ready pickup");
	Expect(result.pickedUpCount == 1, "not-ready pickup effects should count later pickup");
	Expect(result.notReadyCount == 2, "not-ready pickup effects should count not-ready entries");
	Expect(result.entries.size() == 3, "not-ready pickup effects should preserve all pickup entries");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:ready", 2) }), "not-ready pickup effects should only add ready item");
	Expect(result.events.events.size() == 5, "not-ready pickup effects should aggregate not-ready and later pickup events");
	if (result.events.events.size() >= 2) {
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, disabled.id, 0, "disabled pickup should record not-ready event first");
		ExpectEvent(result.events.events[1], iggy::InventoryEvent2DType::PickupNotReady, {}, far.id, 0, "out-of-range pickup should record not-ready event second");
	}
	ExpectSuccessfulPickupEventsAt(result, 2, ready, "later ready pickup should append success events after not-ready events");
}

void TestMissingPlayerProducesNotReadyEntriesWithoutMutation()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(
			0,
			{ EffectEntry(0, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, drop.id)) }),
	});

	const iggy::runtime::RuntimePickupEffectFrameResult result =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(session, inventory, application);

	Expect(result.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady, "missing player pickup effect frame should report PickupNotReady");
	Expect(result.notReadyCount == 1, "missing player pickup effect frame should count not-ready");
	Expect(result.entries.size() == 1, "missing player pickup effect frame should append pickup entry");
	if (result.entries.size() == 1)
		Expect(result.entries[0].result.status == iggy::runtime::RuntimePickupEffectStatus::MissingPlayer, "missing player pickup entry should preserve MissingPlayer");
	Expect(!result.changed, "missing player pickup effect frame should not change");
	ExpectInventoryState(result.inventory, inventory, "missing player pickup effect frame should preserve inventory");
	ExpectNoEvents(result, "missing player pickup effect frame should preserve empty pickup events");
}

void TestTransferFailureStopsLaterPickups()
{
	const iggy::LevelItemDrop2D invalid = Drop("drop:invalid", "", 2);
	const iggy::LevelItemDrop2D later = Drop("drop:later", "item:later", 1);
	iggy::LevelItemDrop2DRegistry drops;
	drops.drops = { invalid, later };
	const iggy::runtime::RuntimeInventoryState inventory {
		{},
		drops,
	};
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(
			0,
			{
				EffectEntry(0, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:bad" }, invalid.id)),
				EffectEntry(1, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:later" }, later.id)),
			}),
	});

	const iggy::runtime::RuntimePickupEffectFrameResult result =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, application);

	Expect(result.status == iggy::runtime::RuntimePickupEffectFrameStatus::Failed, "transfer failure should fail frame pickup effects");
	Expect(result.failedCount == 1, "transfer failure should count one failure");
	Expect(result.entries.size() == 1, "transfer failure should stop later pickup effects");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].effectIndex == 0, "transfer failure should preserve failed effect index");
		Expect(result.entries[0].result.status == iggy::runtime::RuntimePickupEffectStatus::TransferFailed, "transfer failure should preserve pickup effect status");
	}
	Expect(!result.changed, "transfer failure before mutation should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "transfer failure should preserve returned inventory state");
	Expect(result.events.events.size() == 1, "transfer failure should aggregate failure event and stop later events");
	if (result.events.events.size() == 1)
		ExpectEvent(
			result.events.events[0],
			iggy::InventoryEvent2DType::InventoryAddFailed,
			{},
			{},
			2,
			"transfer failure should preserve inventory add failure event");
}

void TestPlanApplicationOverloadUsesInteractionIndexZero()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::InteractionEffectPlanApplyResult application = PlanApplication({
		EffectEntry(5, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, drop.id)),
	});

	const iggy::runtime::RuntimePickupEffectFrameResult result =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, application);

	Expect(result.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "plan application pickup effect should pick up");
	Expect(result.entries.size() == 1, "plan application pickup effect should append entry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].interactionIndex == 0, "plan application pickup effect should use interaction index zero");
		Expect(result.entries[0].effectIndex == 5, "plan application pickup effect should preserve effect index");
	}
	Expect(result.events.events.size() == 3, "plan application pickup effect should aggregate events");
	ExpectSuccessfulPickupEventsAt(result, 0, drop, "plan application pickup effect should preserve pickup events");
}

void TestOriginalInputsAreNotMutated()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 1) }, { drop });
	const iggy::runtime::RuntimeInventoryState inventoryBefore = inventory;
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(
			0,
			{ EffectEntry(0, iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, drop.id)) }),
	});
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult applicationBefore = application;

	const iggy::runtime::RuntimePickupEffectFrameResult result =
		iggy::runtime::RuntimePickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, application);

	Expect(result.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "immutability pickup effect frame setup should pick up");
	ExpectInventoryState(inventory, inventoryBefore, "pickup effect frame should not mutate input inventory");
	Expect(application.entries.size() == applicationBefore.entries.size(), "pickup effect frame should not mutate application entry count");
	if (!application.entries.empty() && !application.entries[0].result.application.entries.empty()) {
		Expect(
			application.entries[0].result.application.entries[0].result.effect.dropId == drop.id,
			"pickup effect frame should not mutate application effect payload");
	}
}

} // namespace

int main()
{
	TestNoInteractionEntriesOrNoPickupEffectsReturnsNoPickupEffects();
	TestSinglePickupItemPicksUpAndUpdatesInventory();
	TestMultiplePickupEffectsCarryInventoryForwardInOrder();
	TestNotReadyPickupEffectsContinueToLaterPickups();
	TestMissingPlayerProducesNotReadyEntriesWithoutMutation();
	TestTransferFailureStopsLaterPickups();
	TestPlanApplicationOverloadUsesInteractionIndexZero();
	TestOriginalInputsAreNotMutated();

	return Failures;
}
