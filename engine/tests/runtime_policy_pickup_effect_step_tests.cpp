#include <cstdlib>
#include <vector>

#include "runtime/RuntimePolicyPickupEffectStep.hpp"
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
	Expect(result.built, "runtime policy pickup effect inventory fixture should build");
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

iggy::LevelItemDrop2DRegistry Drops(std::vector<iggy::LevelItemDrop2D> drops)
{
	return { drops };
}

iggy::ItemDefinition2D Definition(
	const char *itemId,
	const char *displayName = "Potion",
	std::uint32_t maxStackCount = 10,
	iggy::ItemDefinition2DKind kind = iggy::ItemDefinition2DKind::Consumable)
{
	return {
		Id(itemId),
		displayName,
		maxStackCount,
		kind,
	};
}

iggy::ItemDefinition2DCatalog Catalog(std::vector<iggy::ItemDefinition2D> definitions)
{
	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);
	Expect(result.built, "runtime policy pickup effect catalog fixture should build");
	return result.catalog;
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return {
		Inventory(stacks),
		Drops(drops),
	};
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = {
		Id("player:policy-pickup-effect"),
		position,
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::South,
	};
	session.tickIndex = 23;
	return session;
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

void ExpectNoEvents(const iggy::runtime::RuntimePolicyPickupEffectResult &result, const char *message)
{
	Expect(result.events.events.empty(), message);
}

void ExpectPickupNotReadyEvent(
	const iggy::runtime::RuntimePolicyPickupEffectResult &result,
	const iggy::ResourceId &dropId,
	const char *message)
{
	Expect(result.events.events.size() == 1, message);
	if (result.events.events.size() == 1)
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, dropId, 0, message);
}

void ExpectSuccessfulPickupEvents(
	const iggy::runtime::RuntimePolicyPickupEffectResult &result,
	const iggy::LevelItemDrop2D &drop,
	const char *message)
{
	Expect(result.events.events.size() == 3, message);
	if (result.events.events.size() == 3) {
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::ItemAdded, drop.itemId, {}, drop.count, message);
		ExpectEvent(result.events.events[1], iggy::InventoryEvent2DType::DropConsumed, {}, drop.id, 0, message);
		ExpectEvent(result.events.events[2], iggy::InventoryEvent2DType::ItemPickedUp, drop.itemId, drop.id, drop.count, message);
	}
}

void TestNonPickupValidEffectIsIgnored()
{
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::InteractionEffect2D effect = iggy::inspectTextInteractionEffect(Id("target:sign"), "Read me");

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(SessionWithPlayer(), inventory, catalog, effect);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::NotPickupEffect, "valid non-pickup effect should report NotPickupEffect");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::Valid, "valid non-pickup effect should preserve validation status");
	Expect(SameEffect(result.effect, effect), "valid non-pickup effect should preserve effect payload");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupStatus::PickupNotReady, "valid non-pickup effect should not run policy pickup");
	Expect(!result.changed, "valid non-pickup effect should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "valid non-pickup effect should preserve inventory");
	ExpectNoEvents(result, "valid non-pickup effect should not record inventory events");
}

void TestInvalidPickupItemReturnsInvalidEffect()
{
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::InteractionEffect2D effect = iggy::pickupItemInteractionEffect(Id("target:chest"), {});

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(SessionWithPlayer(), inventory, catalog, effect);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::InvalidEffect, "invalid PickupItem should report InvalidEffect");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::MissingDropId, "invalid PickupItem should preserve validation status");
	Expect(SameEffect(result.effect, effect), "invalid PickupItem should preserve effect payload");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupStatus::PickupNotReady, "invalid PickupItem should not run policy pickup");
	Expect(!result.changed, "invalid PickupItem should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "invalid PickupItem should preserve inventory");
	ExpectNoEvents(result, "invalid PickupItem should not record inventory events");
}

void TestSuccessfulPickupWithinMax()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::InteractionEffect2D effect = iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id);

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(SessionWithPlayer(), inventory, catalog, effect);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickedUp, "policy pickup effect within max should pick up");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupStatus::PickedUp, "policy pickup effect should preserve nested pickup status");
	Expect(result.pickup.transfer.status == iggy::PickupPolicyTransfer2DStatus::Transferred, "policy pickup effect should preserve transfer status");
	Expect(result.changed, "policy pickup effect should mark changed");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "policy pickup effect should add item");
	Expect(result.inventory.drops.drops.size() == 1, "policy pickup effect disable mode should keep drop");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "policy pickup effect should disable drop");
	}
	ExpectSuccessfulPickupEvents(result, drop, "policy pickup effect should expose inventory events");
}

void TestExistingStackExactlyToMaxSucceeds()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 3) }, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
			SessionWithPlayer(),
			inventory,
			catalog,
			iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id));

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickedUp, "existing stack policy pickup effect exactly to max should pick up");
	Expect(result.pickup.transfer.add.plan.currentCount == 3, "existing stack policy pickup effect should preserve current count");
	Expect(result.pickup.transfer.add.plan.maxStackCount == 5, "existing stack policy pickup effect should preserve max count");
	Expect(result.pickup.transfer.add.plan.resultingCount == 5, "existing stack policy pickup effect should preserve resulting count");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 5) }), "existing stack policy pickup effect should increment to max");
	ExpectSuccessfulPickupEvents(result, drop, "existing stack policy pickup effect should expose inventory events");
}

void TestOverCapacityReturnsTransferFailed()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 4) }, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
			SessionWithPlayer(),
			inventory,
			catalog,
			iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id));

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::TransferFailed, "over-capacity policy pickup effect should fail transfer");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupStatus::TransferFailed, "over-capacity policy pickup effect should preserve pickup failure");
	Expect(result.pickup.transfer.status == iggy::PickupPolicyTransfer2DStatus::InventoryAddFailed, "over-capacity policy pickup effect should preserve transfer add failure");
	Expect(result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "over-capacity policy pickup effect should preserve policy diagnostics");
	Expect(!result.changed, "over-capacity policy pickup effect should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "over-capacity policy pickup effect should not mutate inventory state");
	ExpectNoEvents(result, "over-capacity policy pickup effect should not record picked-up event");
}

void TestMissingItemDefinitionReturnsTransferFailed()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:scroll", "Scroll", 5) });

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
			SessionWithPlayer(),
			inventory,
			catalog,
			iggy::pickupItemInteractionEffect(Id("target:chest"), drop.id));

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::TransferFailed, "missing definition policy pickup effect should fail transfer");
	Expect(result.pickup.transfer.status == iggy::PickupPolicyTransfer2DStatus::InventoryAddFailed, "missing definition policy pickup effect should preserve transfer failure");
	Expect(result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing definition policy pickup effect should preserve definition diagnostics");
	ExpectInventoryState(result.inventory, inventory, "missing definition policy pickup effect should preserve inventory");
	ExpectNoEvents(result, "missing definition policy pickup effect should not record events");
}

void TestMissingPlayerPropagatesMissingPlayer()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
			session,
			inventory,
			catalog,
			iggy::pickupItemInteractionEffect(Id("target:chest"), Id("drop:potion")));

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::MissingPlayer, "missing-player policy pickup effect should report MissingPlayer");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupStatus::MissingPlayer, "missing-player policy pickup effect should preserve pickup status");
	Expect(!result.changed, "missing-player policy pickup effect should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "missing-player policy pickup effect should preserve inventory");
	ExpectNoEvents(result, "missing-player policy pickup effect should not record events");
}

void TestPickupNotReadyStatusesPropagate()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	const iggy::runtime::RuntimeInventoryState disabled =
		InventoryState({}, { Drop("drop:disabled", "item:potion", 1, { 0.0F, 0.0F }, 1.0F, false) });
	const iggy::runtime::RuntimeInventoryState outOfRange =
		InventoryState({}, { Drop("drop:far", "item:potion", 1, { 4.0F, 0.0F }, 1.0F, true) });

	const iggy::runtime::RuntimePolicyPickupEffectResult missingResult =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
			session,
			InventoryState({}, {}),
			catalog,
			iggy::pickupItemInteractionEffect(Id("target:chest"), Id("drop:missing")));
	const iggy::runtime::RuntimePolicyPickupEffectResult disabledResult =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
			session,
			disabled,
			catalog,
			iggy::pickupItemInteractionEffect(Id("target:chest"), Id("drop:disabled")));
	const iggy::runtime::RuntimePolicyPickupEffectResult outOfRangeResult =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
			session,
			outOfRange,
			catalog,
			iggy::pickupItemInteractionEffect(Id("target:chest"), Id("drop:far")));

	Expect(missingResult.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickupNotReady, "missing drop policy pickup effect should not be ready");
	Expect(missingResult.pickup.plan.status == iggy::PickupPlan2DStatus::DropNotFound, "missing drop policy pickup effect should preserve plan status");
	Expect(disabledResult.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickupNotReady, "disabled drop policy pickup effect should not be ready");
	Expect(disabledResult.pickup.plan.status == iggy::PickupPlan2DStatus::DropDisabled, "disabled drop policy pickup effect should preserve plan status");
	Expect(outOfRangeResult.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickupNotReady, "out-of-range policy pickup effect should not be ready");
	Expect(outOfRangeResult.pickup.plan.status == iggy::PickupPlan2DStatus::OutOfRange, "out-of-range policy pickup effect should preserve plan status");
	ExpectPickupNotReadyEvent(missingResult, Id("drop:missing"), "missing drop policy pickup effect should record not-ready event");
	ExpectPickupNotReadyEvent(disabledResult, Id("drop:disabled"), "disabled drop policy pickup effect should record not-ready event");
	ExpectPickupNotReadyEvent(outOfRangeResult, Id("drop:far"), "out-of-range policy pickup effect should record not-ready event");
}

void TestRemoveModeAndExactIds()
{
	const iggy::LevelItemDrop2D unqualified = Drop("potion", "potion", 1, { 1.0F, 0.0F }, 2.0F, true);
	const iggy::LevelItemDrop2D namespaced = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("potion", 1) }, { unqualified, namespaced });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({
		Definition("potion", "Unqualified Potion", 1),
		Definition("item:potion", "Potion", 3),
	});
	iggy::runtime::RuntimePolicyPickupConfig config;
	config.transfer.consumeMode = iggy::LevelItemDropConsume2DMode::Remove;

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(
			SessionWithPlayer(),
			inventory,
			catalog,
			iggy::pickupItemInteractionEffect(Id("target:chest"), namespaced.id),
			config);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickedUp, "namespaced remove-mode policy pickup effect should pick up");
	Expect(result.pickup.transfer.consume.mode == iggy::LevelItemDropConsume2DMode::Remove, "namespaced policy pickup effect should preserve consume mode");
	Expect(SameStacks(
			   result.inventory.inventory.stacks,
			   {
				   Stack("potion", 1),
				   Stack("item:potion", 2),
			   }),
		"namespaced policy pickup effect should preserve distinct item ids");
	Expect(result.inventory.drops.drops.size() == 1, "namespaced policy pickup effect should remove requested drop only");
	if (result.inventory.drops.drops.size() == 1)
		Expect(result.inventory.drops.drops[0].id == Id("potion"), "unqualified drop should remain");
	ExpectSuccessfulPickupEvents(result, namespaced, "namespaced remove-mode policy pickup effect should expose inventory events");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 1) }, { Drop("drop:potion", "item:potion", 2) });
	const iggy::runtime::RuntimeInventoryState inventoryBefore = inventory;
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	iggy::InteractionEffect2D effect = iggy::pickupItemInteractionEffect(Id("target:chest"), Id("drop:potion"));
	const iggy::InteractionEffect2D effectBefore = effect;

	const iggy::runtime::RuntimePolicyPickupEffectResult result =
		iggy::runtime::RuntimePolicyPickupEffectStep {}.apply(session, inventory, catalog, effect);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupEffectStatus::PickedUp, "immutability policy pickup effect setup should pick up");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "policy pickup effect should not mutate session player presence");
	Expect(NearVec(session.player.position, sessionBefore.player.position), "policy pickup effect should not mutate session position");
	Expect(session.tickIndex == sessionBefore.tickIndex, "policy pickup effect should not mutate session tick");
	ExpectInventoryState(inventory, inventoryBefore, "policy pickup effect should not mutate inventory state");
	Expect(SameEffect(effect, effectBefore), "policy pickup effect should not mutate effect");
}

} // namespace

int main()
{
	TestNonPickupValidEffectIsIgnored();
	TestInvalidPickupItemReturnsInvalidEffect();
	TestSuccessfulPickupWithinMax();
	TestExistingStackExactlyToMaxSucceeds();
	TestOverCapacityReturnsTransferFailed();
	TestMissingItemDefinitionReturnsTransferFailed();
	TestMissingPlayerPropagatesMissingPlayer();
	TestPickupNotReadyStatusesPropagate();
	TestRemoveModeAndExactIds();
	TestInputsAreNotMutated();

	return Failures;
}
