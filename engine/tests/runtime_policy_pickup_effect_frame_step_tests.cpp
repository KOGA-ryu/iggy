#include <cstdlib>
#include <vector>

#include "runtime/RuntimePolicyPickupEffectFrameStep.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime policy pickup effect frame inventory fixture should build");
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
		Id(id),
		Id(itemId),
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

iggy::ItemDefinition2D Definition(const char *itemId, const char *displayName = "Item", std::uint32_t maxStackCount = 10)
{
	return {
		Id(itemId),
		displayName,
		maxStackCount,
		iggy::ItemDefinition2DKind::Material,
	};
}

iggy::ItemDefinition2DCatalog Catalog(std::vector<iggy::ItemDefinition2D> definitions)
{
	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);
	Expect(result.built, "runtime policy pickup effect frame catalog fixture should build");
	return result.catalog;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = {
		Id("player:policy-pickup-effect-frame"),
		position,
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::South,
	};
	session.tickIndex = 29;
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

bool SameDrops(const std::vector<iggy::LevelItemDrop2D> &actual, const std::vector<iggy::LevelItemDrop2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameDrop(actual[index], expected[index]))
			return false;
	}
	return true;
}

bool SameEffect(const iggy::InteractionEffect2D &actual, const iggy::InteractionEffect2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.dropId == expected.dropId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

void ExpectInventoryState(
	const iggy::runtime::RuntimeInventoryState &actual,
	const iggy::runtime::RuntimeInventoryState &expected,
	const char *message)
{
	Expect(SameStacks(actual.inventory.stacks, expected.inventory.stacks), message);
	Expect(SameDrops(actual.drops.drops, expected.drops.drops), message);
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

void ExpectNoEvents(const iggy::runtime::RuntimePolicyPickupEffectFrameResult &result, const char *message)
{
	Expect(result.events.events.empty(), message);
}

void ExpectSuccessfulPickupEventsAt(
	const iggy::runtime::RuntimePolicyPickupEffectFrameResult &result,
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

void TestEmptyAndNoPickupFramesAreNoOp()
{
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult noPickup = FrameApplication({
		InteractionEntry(0, { EffectEntry(4, iggy::inspectTextInteractionEffect(Id("target:sign"), "Read me")) }),
	});

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult emptyResult =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, FrameApplication({}));
	const iggy::runtime::RuntimePolicyPickupEffectFrameResult noPickupResult =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, noPickup);

	Expect(emptyResult.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::NoPickupEffects, "empty policy pickup effect frame should have no pickup effects");
	Expect(emptyResult.entries.empty(), "empty policy pickup effect frame should have no entries");
	ExpectInventoryState(emptyResult.inventory, inventory, "empty policy pickup effect frame should preserve inventory");
	ExpectNoEvents(emptyResult, "empty policy pickup effect frame should have no events");
	Expect(noPickupResult.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::NoPickupEffects, "non-pickup policy effect frame should have no pickup effects");
	Expect(noPickupResult.entries.empty(), "non-pickup policy effect frame should not append entries");
	ExpectInventoryState(noPickupResult.inventory, inventory, "non-pickup policy effect frame should preserve inventory");
	ExpectNoEvents(noPickupResult, "non-pickup policy effect frame should have no events");
}

void TestSingleSuccessfulPickupWithinMax()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::InteractionEffect2D effect = iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id);
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application =
		FrameApplication({ InteractionEntry(7, { EffectEntry(3, effect) }) });

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "single policy pickup effect should pick up");
	Expect(result.pickedUpCount == 1, "single policy pickup effect should count pickup");
	Expect(result.notReadyCount == 0 && result.failedCount == 0, "single policy pickup effect should not count not-ready or failure");
	Expect(result.changed, "single policy pickup effect should mark changed");
	Expect(result.entries.size() == 1, "single policy pickup effect should append one entry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].interactionIndex == 0, "single policy pickup effect should preserve interaction entry index");
		Expect(result.entries[0].effectIndex == 3, "single policy pickup effect should preserve effect index");
		Expect(SameEffect(result.entries[0].effect, effect), "single policy pickup effect should preserve effect payload");
		Expect(result.entries[0].result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickedUp, "single policy pickup effect should preserve effect result");
	}
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "single policy pickup effect should add stack");
	Expect(result.inventory.drops.drops.size() == 1, "single policy pickup effect should keep disabled drop");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "single policy pickup effect should disable drop");
	}
	Expect(result.events.events.size() == 3, "single policy pickup effect should aggregate success events");
	ExpectSuccessfulPickupEventsAt(result, 0, drop, "single policy pickup effect should preserve event order");
}

void TestMultiplePickupsCarryInventoryForward()
{
	const iggy::LevelItemDrop2D potion = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2D key = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 1) }, { potion, key });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({
		Definition("item:potion", "Potion", 5),
		Definition("item:key", "Key", 1),
	});
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(2, { EffectEntry(0, iggy::pickupItemInteractionEffect(Id("target:chest"), potion.id)) }),
		InteractionEntry(4, { EffectEntry(8, iggy::pickupItemInteractionEffect(Id("target:key"), key.id)) }),
	});

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "multiple policy pickups should pick up");
	Expect(result.pickedUpCount == 2, "multiple policy pickups should count both pickups");
	Expect(result.entries.size() == 2, "multiple policy pickups should append entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].interactionIndex == 0 && result.entries[0].effectIndex == 0, "first policy pickup should preserve indexes");
		Expect(result.entries[1].interactionIndex == 1 && result.entries[1].effectIndex == 8, "second policy pickup should preserve indexes");
	}
	Expect(SameStacks(
			   result.inventory.inventory.stacks,
			   {
				   Stack("item:potion", 3),
				   Stack("item:key", 1),
			   }),
		"multiple policy pickups should carry inventory forward");
	Expect(result.events.events.size() == 6, "multiple policy pickups should aggregate all events");
	ExpectSuccessfulPickupEventsAt(result, 0, potion, "multiple policy pickups should preserve first event order");
	ExpectSuccessfulPickupEventsAt(result, 3, key, "multiple policy pickups should preserve second event order");
}

void TestExistingStackExactlyToMaxSucceeds()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 3) }, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application =
		FrameApplication({ InteractionEntry(0, { EffectEntry(0, iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id)) }) });

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "existing stack policy frame pickup exactly to max should pick up");
	Expect(result.entries.size() == 1, "existing stack policy frame pickup should append one entry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].result.pickup.transfer.add.plan.currentCount == 3, "existing stack policy frame pickup should preserve current count");
		Expect(result.entries[0].result.pickup.transfer.add.plan.resultingCount == 5, "existing stack policy frame pickup should preserve resulting count");
	}
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 5) }), "existing stack policy frame pickup should increment to max");
}

void TestOverCapacityFailsAndStopsLaterPickups()
{
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2D later = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 4) }, { first, later });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({
		Definition("item:potion", "Potion", 5),
		Definition("item:key", "Key", 1),
	});
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(
			0,
			{
				EffectEntry(0, iggy::pickupItemInteractionEffect(Id("target:chest"), first.id)),
				EffectEntry(1, iggy::pickupItemInteractionEffect(Id("target:key"), later.id)),
			}),
	});

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::Failed, "over-capacity policy frame pickup should fail");
	Expect(result.failedCount == 1, "over-capacity policy frame pickup should count failure");
	Expect(result.entries.size() == 1, "over-capacity policy frame pickup should stop later pickup effects");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::TransferFailed, "over-capacity policy frame pickup should preserve transfer failure");
		Expect(result.entries[0].result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "over-capacity policy frame pickup should preserve stack diagnostics");
	}
	Expect(!result.changed, "over-capacity policy frame pickup should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "over-capacity policy frame pickup should not consume drop");
	ExpectNoEvents(result, "over-capacity policy frame pickup should not record events");
}

void TestMissingDefinitionFailsAndStopsLaterPickups()
{
	const iggy::LevelItemDrop2D missingDefinition = Drop("drop:potion", "item:potion", 1);
	const iggy::LevelItemDrop2D later = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { missingDefinition, later });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:key", "Key", 1) });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(
			0,
			{
				EffectEntry(0, iggy::pickupItemInteractionEffect(Id("target:chest"), missingDefinition.id)),
				EffectEntry(1, iggy::pickupItemInteractionEffect(Id("target:key"), later.id)),
			}),
	});

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::Failed, "missing definition policy frame pickup should fail");
	Expect(result.failedCount == 1, "missing definition policy frame pickup should count failure");
	Expect(result.entries.size() == 1, "missing definition policy frame pickup should stop later pickup effects");
	if (result.entries.size() == 1)
		Expect(result.entries[0].result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing definition policy frame pickup should preserve definition diagnostics");
	ExpectInventoryState(result.inventory, inventory, "missing definition policy frame pickup should not consume drops");
	ExpectNoEvents(result, "missing definition policy frame pickup should not record events");
}

void TestPickupNotReadyContinuesAndAggregatesEvent()
{
	const iggy::LevelItemDrop2D disabled = Drop("drop:disabled", "item:potion", 1, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::LevelItemDrop2D ready = Drop("drop:ready", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { disabled, ready });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application = FrameApplication({
		InteractionEntry(0, { EffectEntry(0, iggy::pickupItemInteractionEffect(Id("target:disabled"), disabled.id)) }),
		InteractionEntry(1, { EffectEntry(1, iggy::pickupItemInteractionEffect(Id("target:ready"), ready.id)) }),
	});

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "not-ready policy pickup should continue to later pickup");
	Expect(result.notReadyCount == 1 && result.pickedUpCount == 1, "not-ready policy pickup should count both outcomes");
	Expect(result.entries.size() == 2, "not-ready policy pickup should keep both entries");
	Expect(result.events.events.size() == 4, "not-ready policy pickup should aggregate not-ready and success events");
	if (!result.events.events.empty())
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, disabled.id, 0, "not-ready policy pickup should record PickupNotReady first");
	ExpectSuccessfulPickupEventsAt(result, 1, ready, "not-ready policy pickup should append later success events");
}

void TestMissingPlayerProducesNotReadyEntry()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application =
		FrameApplication({ InteractionEntry(0, { EffectEntry(0, iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id)) }) });

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(session, inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickupNotReady, "missing-player policy pickup frame should report PickupNotReady");
	Expect(result.notReadyCount == 1, "missing-player policy pickup frame should count not-ready");
	Expect(result.entries.size() == 1, "missing-player policy pickup frame should append entry");
	if (result.entries.size() == 1)
		Expect(result.entries[0].result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::MissingPlayer, "missing-player policy pickup frame should preserve single-effect status");
	ExpectInventoryState(result.inventory, inventory, "missing-player policy pickup frame should preserve inventory");
	ExpectNoEvents(result, "missing-player policy pickup frame should not record events");
}

void TestPlanApplicationOverloadUsesInteractionIndexZero()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::InteractionEffectPlanApplyResult application =
		PlanApplication({ EffectEntry(5, iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id)) });

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "plan application policy pickup should pick up");
	Expect(result.entries.size() == 1, "plan application policy pickup should append entry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].interactionIndex == 0, "plan application policy pickup should use interaction index zero");
		Expect(result.entries[0].effectIndex == 5, "plan application policy pickup should preserve effect index");
	}
	ExpectSuccessfulPickupEventsAt(result, 0, drop, "plan application policy pickup should aggregate events");
}

void TestOriginalInputsAreNotMutated()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 1) }, { drop });
	const iggy::runtime::RuntimeInventoryState inventoryBefore = inventory;
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult application =
		FrameApplication({ InteractionEntry(0, { EffectEntry(0, iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id)) }) });
	const iggy::runtime::RuntimeInteractionEffectApplyFrameResult applicationBefore = application;

	const iggy::runtime::RuntimePolicyPickupEffectFrameResult result =
		iggy::runtime::RuntimePolicyPickupEffectFrameStep {}.apply(SessionWithPlayer(), inventory, catalog, application);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "immutability policy pickup frame setup should pick up");
	ExpectInventoryState(inventory, inventoryBefore, "policy pickup frame should not mutate input inventory");
	Expect(application.entries.size() == applicationBefore.entries.size(), "policy pickup frame should not mutate application entry count");
	if (!application.entries.empty() && !application.entries[0].result.application.entries.empty())
		Expect(application.entries[0].result.application.entries[0].result.effect.dropId == drop.id, "policy pickup frame should not mutate application effect payload");
}

} // namespace

int main()
{
	TestEmptyAndNoPickupFramesAreNoOp();
	TestSingleSuccessfulPickupWithinMax();
	TestMultiplePickupsCarryInventoryForward();
	TestExistingStackExactlyToMaxSucceeds();
	TestOverCapacityFailsAndStopsLaterPickups();
	TestMissingDefinitionFailsAndStopsLaterPickups();
	TestPickupNotReadyContinuesAndAggregatesEvent();
	TestMissingPlayerProducesNotReadyEntry();
	TestPlanApplicationOverloadUsesInteractionIndexZero();
	TestOriginalInputsAreNotMutated();

	return Failures;
}
