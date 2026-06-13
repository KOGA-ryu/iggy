#include <cstdlib>
#include <type_traits>
#include <utility>
#include <vector>

#include "runtime/RuntimeGameplayFrameStep.hpp"
#include "support/CommandFrameFixtures.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::CommandFrame;
using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:gameplay-frame-acceptance" };

template <typename T, typename = void>
struct HasInteractionMember : std::false_type {
};

template <typename T>
struct HasInteractionMember<T, std::void_t<decltype(std::declval<T>().interaction)>> : std::true_type {
};

template <typename T, typename = void>
struct HasInventoryMember : std::false_type {
};

template <typename T>
struct HasInventoryMember<T, std::void_t<decltype(std::declval<T>().inventory)>> : std::true_type {
};

template <typename T, typename = void>
struct HasCommandQueueMember : std::false_type {
};

template <typename T>
struct HasCommandQueueMember<T, std::void_t<decltype(std::declval<T>().commandQueue)>> : std::true_type {
};

static_assert(!HasInteractionMember<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasInventoryMember<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasCommandQueueMember<iggy::runtime::RuntimeSessionState>::value);

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerConfig()
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = 1.0F;
	return config;
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig()
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = 0.25F;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "...", "..." });
	session.level.map.id = Id("level:gameplay-frame-acceptance");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 21;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId) });
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { Id(id), kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "gameplay frame acceptance target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "gameplay frame acceptance effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "gameplay frame acceptance inventory fixture should build");
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
	return { Id(id), Id(itemId), count, position, pickupRadius, enabled };
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return { Inventory(stacks), iggy::LevelItemDrop2DRegistry { drops } };
}

iggy::runtime::RuntimeGameplayState GameplayState(
	iggy::runtime::RuntimeSessionState session,
	iggy::runtime::RuntimeInteractionState interaction = {},
	iggy::runtime::RuntimeInventoryState inventory = {},
	iggy::runtime::RuntimeCommandQueueState queue = {})
{
	return { session, queue, interaction, inventory };
}

iggy::runtime::RuntimeGameplayFrameInput Input(
	iggy::runtime::RuntimeGameplayState state,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::PlayerInputContext2D context = {},
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.playerInputContext = context;
	input.actorId = PlayerId;
	input.playerIntents = intents;
	input.commandQueueConfig = queueConfig;
	input.fallbackPlayerPosition = { 1.5F, 1.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	return input;
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "gameplay frame acceptance collision fixture should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({ Object(Id("wall:east"), { { 0.5F, -0.5F }, { 1.5F, 0.5F } }) });
}

void SetCollisionCache(iggy::runtime::RuntimeSessionState &session, const iggy::physics2d::CollisionWorld2D &world)
{
	session.derivedCaches.hasCollisionCache = true;
	session.derivedCaches.collision.world = world;
}

bool SameStacks(const std::vector<iggy::InventoryItemStack2D> &actual, const std::vector<iggy::InventoryItemStack2D> &expected)
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

void ExpectInventoryState(
	const iggy::runtime::RuntimeInventoryState &actual,
	const iggy::runtime::RuntimeInventoryState &expected,
	const char *message)
{
	Expect(SameStacks(actual.inventory.stacks, expected.inventory.stacks), message);
	Expect(actual.drops.drops.size() == expected.drops.drops.size(), message);
	if (actual.drops.drops.size() != expected.drops.drops.size())
		return;
	for (std::size_t index = 0; index < actual.drops.drops.size(); ++index)
		Expect(SameDrop(actual.drops.drops[index], expected.drops.drops[index]), message);
}

void ExpectTargetEnabled(
	const iggy::InteractionTarget2DRegistry &registry,
	const iggy::ResourceId &targetId,
	bool enabled,
	const char *message)
{
	const iggy::InteractionTarget2D *target = registry.find(targetId);
	Expect(target != nullptr, message);
	if (target != nullptr)
		Expect(target->enabled == enabled, message);
}

bool HasEvent(
	const std::vector<iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent> &events,
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent expected)
{
	for (const iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent event : events) {
		if (event == expected)
			return true;
	}
	return false;
}

void TestMoveOnlyUpdatesSessionAndLeavesExplicitStatesStable()
{
	const iggy::InteractionTarget2D target = Target("target:stable");
	const iggy::LevelItemDrop2D drop = Drop("drop:stable", "item:stable", 1);
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		SessionWithPlayer({ 0.0F, 0.0F }),
		{ Registry({ target }), Catalog({ Entry("target:stable", { iggy::inspectTextInteractionEffect(target.id, "Stable") }) }) },
		InventoryState({ Stack("item:owned", 2) }, { drop }));

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerMoveToPointIntent({ 1.0F, 0.0F }) }));

	Expect(NearVec(result.state.session.player.position, { 1.0F, 0.0F }), "move-only gameplay frame should update session player");
	Expect(result.state.interaction.targets.find(target.id) != nullptr, "move-only gameplay frame should preserve interaction targets");
	ExpectInventoryState(result.state.inventory, state.inventory, "move-only gameplay frame should preserve inventory/drop state");
	Expect(result.report.acceptedCommandCount == 1, "move-only gameplay frame report should count accepted command");
}

void TestInteractionToggleOnlyUpdatesInteractionStateAndReportsEvent()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:lever"),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:owned", 1) }, {});
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		SessionWithPlayer(),
		{
			Registry(targets),
			Catalog({ Entry("target:lever", { iggy::toggleTargetInteractionEffect(Id("target:door"), false) }) }),
		},
		inventory);

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(Id("target:lever")) }));

	ExpectTargetEnabled(result.state.interaction.targets, Id("target:door"), false, "toggle gameplay frame should update interaction state");
	ExpectInventoryState(result.state.inventory, inventory, "toggle gameplay frame should preserve inventory/drop state");
	Expect(result.report.interactionMutated, "toggle gameplay frame report should mark interaction mutation");
	Expect(result.report.interactionEventCount == 1, "toggle gameplay frame report should count local interaction event");
	Expect(HasEvent(result.report.events, iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::InteractionEventRecorded), "toggle gameplay frame report should include interaction event fact");
}

void TestPickupOnlyUpdatesInventoryStateAndReportsPickup()
{
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		SessionWithPlayer(),
		{
			Registry({ target }),
			Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
		},
		InventoryState({}, { drop }));

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }));

	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:potion", 2) }), "pickup gameplay frame should add item to inventory");
	Expect(result.state.inventory.drops.drops.size() == 1 && !result.state.inventory.drops.drops[0].enabled, "pickup gameplay frame should consume drop");
	Expect(result.state.interaction.targets.find(target.id) != nullptr, "pickup gameplay frame should preserve interaction target registry");
	Expect(result.report.pickedUpCount == 1 && result.report.inventoryChanged, "pickup gameplay frame report should expose pickup success");
}

void TestMoveThenPickupUsesPostMovePosition()
{
	const iggy::InteractionTarget2D target = Target("target:after_move", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:after_move", "item:after_move", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		SessionWithPlayer({ 0.0F, 0.0F }),
		{
			Registry({ target }),
			Catalog({ Entry("target:after_move", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
		},
		InventoryState({}, { drop }));

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(
			Input(state, {
				iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}));

	Expect(NearVec(result.state.session.player.position, { 1.0F, 0.0F }), "move-then-pickup gameplay frame should expose post-move player position");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:after_move", 1) }), "move-then-pickup gameplay frame should pick up after movement");
	if (!result.frame.pickup.entries.empty())
		Expect(NearVec(result.frame.pickup.entries[0].result.pickup.plan.actorPosition, { 1.0F, 0.0F }), "move-then-pickup should evaluate pickup from post-move position");
}

void TestContextBlockedInteractDoesNotMutateExplicitStates()
{
	iggy::PlayerInputContext2D context;
	context.worldInputEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:blocked", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:blocked", "item:blocked", 1);
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		SessionWithPlayer(),
		{
			Registry({ target }),
			Catalog({ Entry("target:blocked", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
		},
		InventoryState({}, { drop }));

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }, context));

	Expect(result.report.blockedIntentCount == 1, "context-blocked gameplay frame should report blocked intent");
	Expect(result.state.interaction.targets.find(target.id)->enabled == target.enabled, "context-blocked gameplay frame should preserve interaction state");
	ExpectInventoryState(result.state.inventory, state.inventory, "context-blocked gameplay frame should preserve inventory/drop state");
}

void TestQueueRejectedPreservesDiagnosticsWithoutMutation()
{
	const iggy::InteractionTarget2D target = Target("target:queued", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:queued", "item:queued", 1);
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		SessionWithPlayer(),
		{
			Registry({ target }),
			Catalog({ Entry("target:queued", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
		},
		InventoryState({}, { drop }),
		fullQueue);
	iggy::runtime::RuntimeCommandQueueConfig queueConfig;
	queueConfig.maxFrames = 1;

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }, {}, queueConfig));

	Expect(result.frame.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected gameplay frame should preserve nested rejection");
	Expect(result.frame.interaction.playerInput.command.intake.mapping.frame.commands.size() == 1, "queue-rejected gameplay frame should preserve mapped frame diagnostics");
	Expect(result.state.commandQueue.frames.size() == fullQueue.frames.size(), "queue-rejected gameplay frame should preserve queue state");
	ExpectInventoryState(result.state.inventory, state.inventory, "queue-rejected gameplay frame should not mutate inventory/drop state");
	Expect(result.report.pickedUpCount == 0 && !result.report.inventoryChanged, "queue-rejected gameplay frame report should not claim pickup mutation");
}

void TestExplicitCollisionWorldOverrideAffectsMovementReachAndPickup()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:explicit", "item:explicit", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		session,
		{
			Registry({ target }),
			Catalog({ Entry("target:explicit", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
		},
		InventoryState({}, { drop }));

	const iggy::runtime::RuntimeGameplayFrameResult derivedBlocked =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(
			Input(state, {
				iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}));
	const iggy::runtime::RuntimeGameplayFrameResult explicitAllowed =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(
			Input(state, {
				iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}),
			iggy::physics2d::CollisionWorld2D {});

	Expect(derivedBlocked.state.inventory.inventory.stacks.empty(), "session-derived collision cache should prevent reach/pickup in baseline");
	Expect(SameStacks(explicitAllowed.state.inventory.inventory.stacks, { Stack("item:explicit", 1) }), "explicit collision world should allow movement/reach/pickup");
	Expect(NearVec(explicitAllowed.state.session.player.position, { 1.0F, 0.0F }), "explicit collision world should control returned player position");
}

void TestInputGameplayStateIsNotMutatedAndReturnedStateCarriesUpdates()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:immutable", "item:immutable", 1);
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		SessionWithPlayer(),
		{
			Registry({ target }),
			Catalog({ Entry("target:immutable", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
		},
		InventoryState({}, { drop }));
	const iggy::runtime::RuntimeGameplayFrameInput input = Input(state, { iggy::playerInteractIntent(target.id) });

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(input);

	ExpectPlayerAgent(input.state.session.player, state.session.player, "gameplay frame acceptance should not mutate input session");
	Expect(input.state.interaction.targets.find(target.id)->enabled == target.enabled, "gameplay frame acceptance should not mutate input interaction state");
	ExpectInventoryState(input.state.inventory, state.inventory, "gameplay frame acceptance should not mutate input inventory/drop state");
	Expect(result.state.inventory.inventory.stacks.size() == 1, "gameplay frame acceptance returned state should carry inventory update");
}

} // namespace

int main()
{
	TestMoveOnlyUpdatesSessionAndLeavesExplicitStatesStable();
	TestInteractionToggleOnlyUpdatesInteractionStateAndReportsEvent();
	TestPickupOnlyUpdatesInventoryStateAndReportsPickup();
	TestMoveThenPickupUsesPostMovePosition();
	TestContextBlockedInteractDoesNotMutateExplicitStates();
	TestQueueRejectedPreservesDiagnosticsWithoutMutation();
	TestExplicitCollisionWorldOverrideAffectsMovementReachAndPickup();
	TestInputGameplayStateIsNotMutatedAndReturnedStateCarriesUpdates();

	return Failures;
}
