#include <cstdlib>
#include <vector>

#include "runtime/RuntimeGameplayFrameRunner.hpp"
#include "runtime/RuntimeGameplayFrameReporter.hpp"
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

const iggy::ResourceId PlayerId { "player:gameplay-frame-runner" };

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
	session.level.map.id = Id("level:gameplay-frame-runner");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 31;
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
	Expect(result.built, "runtime gameplay frame runner target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime gameplay frame runner effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime gameplay frame runner inventory fixture should build");
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

iggy::runtime::RuntimeGameplayFrameRunnerFrame Frame(
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::PlayerInputContext2D context = {},
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	iggy::runtime::RuntimeGameplayFrameRunnerFrame frame;
	frame.playerInputContext = context;
	frame.actorId = PlayerId;
	frame.playerIntents = intents;
	frame.commandQueueConfig = queueConfig;
	frame.fallbackPlayerPosition = { 1.5F, 1.5F };
	frame.playerCommandConfig = PlayerConfig();
	frame.npcConfig = NpcConfig();
	return frame;
}

iggy::runtime::RuntimeGameplayFrameInput FrameInput(
	iggy::runtime::RuntimeGameplayState state,
	const iggy::runtime::RuntimeGameplayFrameRunnerFrame &frame)
{
	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.playerInputContext = frame.playerInputContext;
	input.actorId = frame.actorId;
	input.playerIntents = frame.playerIntents;
	input.commandQueueConfig = frame.commandQueueConfig;
	input.fallbackPlayerPosition = frame.fallbackPlayerPosition;
	input.playerCommandConfig = frame.playerCommandConfig;
	input.npcConfig = frame.npcConfig;
	input.interactionReach = frame.interactionReach;
	input.pickup = frame.pickup;
	return input;
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "runtime gameplay frame runner collision fixture should build");
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

void ExpectInventoryEvent(
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

void TestEmptyFramesReturnInitialStateUnchanged()
{
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer({ 2.0F, 3.0F }), {}, {}, { { WaitFrame() } });
	const iggy::runtime::RuntimeGameplayFrameRunnerInput input { initial, {} };

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run(input);

	Expect(result.ticks.empty(), "empty gameplay frame runner input should produce no ticks");
	ExpectPlayerAgent(result.finalState.session.player, initial.session.player, "empty gameplay frame runner should preserve player state");
	Expect(result.finalState.commandQueue.frames.size() == 1, "empty gameplay frame runner should preserve command queue");
	Expect(result.finalState.interaction.targets.targets().empty(), "empty gameplay frame runner should preserve interaction state");
	Expect(result.finalState.inventory.inventory.stacks.empty(), "empty gameplay frame runner should preserve inventory state");
	Expect(result.inventoryEvents.events.empty(), "empty gameplay frame runner should aggregate no inventory events");
}

void TestOneFrameMatchesGameplayFrameStepBehavior()
{
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer({ 0.0F, 0.0F }));
	const iggy::runtime::RuntimeGameplayFrameRunnerFrame frame =
		Frame({ iggy::playerMoveToPointIntent({ 1.0F, 0.0F }) });
	const iggy::runtime::RuntimeGameplayFrameResult stepResult =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(FrameInput(initial, frame));

	const iggy::runtime::RuntimeGameplayFrameRunnerResult runnerResult =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, { frame } });

	Expect(runnerResult.ticks.size() == 1, "single gameplay frame runner input should produce one tick");
	ExpectPlayerAgent(runnerResult.finalState.session.player, stepResult.state.session.player, "single gameplay frame runner should match frame step player result");
	ExpectPlayerAgent(runnerResult.ticks[0].frame.state.session.player, stepResult.state.session.player, "runner tick should preserve full frame result");
	Expect(runnerResult.ticks[0].report.acceptedCommandCount == 1, "runner tick should include top-level gameplay frame report");
}

void TestMultipleMovementFramesCarryPlayerPositionForward()
{
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer({ 0.0F, 0.0F }));
	const std::vector<iggy::runtime::RuntimeGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerMoveToPointIntent({ 1.0F, 0.0F }) }),
		Frame({ iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
	};

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, frames });

	Expect(result.ticks.size() == 2, "movement runner should produce one tick per frame");
	Expect(NearVec(result.ticks[0].frame.state.session.player.position, { 1.0F, 0.0F }), "first movement frame should advance player");
	Expect(NearVec(result.ticks[1].frame.state.session.player.position, { 2.0F, 0.0F }), "second movement frame should start from carried player position");
	Expect(NearVec(result.finalState.session.player.position, { 2.0F, 0.0F }), "movement runner should carry final player position");
	Expect(result.ticks[0].report.acceptedCommandCount == 1 && result.ticks[1].report.acceptedCommandCount == 1, "movement runner should collect per-frame reports");
}

void TestInteractionToggleCarriesIntoLaterFrames()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:lever"),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry(targets),
		Catalog({ Entry("target:lever", { iggy::toggleTargetInteractionEffect(Id("target:door"), false) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer(), interaction);
	const std::vector<iggy::runtime::RuntimeGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerInteractIntent(Id("target:lever")) }),
		Frame({ iggy::playerInteractIntent(Id("target:door")) }),
	};

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, frames });

	Expect(result.ticks.size() == 2, "toggle runner should produce one tick per interaction frame");
	ExpectTargetEnabled(result.ticks[0].frame.state.interaction.targets, Id("target:door"), false, "first frame should toggle carried target");
	ExpectTargetEnabled(result.finalState.interaction.targets, Id("target:door"), false, "final carried interaction state should preserve toggle");
	Expect(result.ticks[0].report.interactionChanged, "first toggle frame should report interaction change");
	Expect(!result.ticks[1].report.interactionChanged, "later disabled-target frame should observe carried state without mutating it again");
}

void TestPickupCarriesIntoLaterFrames()
{
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));
	const std::vector<iggy::runtime::RuntimeGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerInteractIntent(target.id) }),
		Frame({ iggy::playerInteractIntent(target.id) }),
	};

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, frames });

	Expect(result.ticks.size() == 2, "pickup runner should produce one tick per frame");
	Expect(result.ticks[0].report.pickedUpCount == 1, "first pickup frame should report pickup");
	Expect(result.ticks[1].report.pickupNotReadyCount == 1, "second pickup frame should see carried disabled drop");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:potion", 2) }), "pickup runner should carry inventory forward");
	Expect(result.finalState.inventory.drops.drops.size() == 1 && !result.finalState.inventory.drops.drops[0].enabled, "pickup runner should carry consumed drop forward");
	Expect(result.inventoryEvents.events.size() == 4, "pickup runner should aggregate pickup and later not-ready inventory events");
	if (result.inventoryEvents.events.size() == 4) {
		ExpectInventoryEvent(result.inventoryEvents.events[0], iggy::InventoryEvent2DType::ItemAdded, Id("item:potion"), {}, 2, "pickup runner should aggregate item-added event first");
		ExpectInventoryEvent(result.inventoryEvents.events[1], iggy::InventoryEvent2DType::DropConsumed, {}, drop.id, 0, "pickup runner should aggregate drop-consumed event second");
		ExpectInventoryEvent(result.inventoryEvents.events[2], iggy::InventoryEvent2DType::ItemPickedUp, Id("item:potion"), drop.id, 2, "pickup runner should aggregate item-picked-up event third");
		ExpectInventoryEvent(result.inventoryEvents.events[3], iggy::InventoryEvent2DType::PickupNotReady, {}, drop.id, 0, "pickup runner should aggregate later pickup-not-ready event");
	}
}

void TestReportsCollectedPerFrameWithExpectedFacts()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:combo"),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::LevelItemDrop2D drop = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry(targets),
		Catalog({
			Entry("target:combo", {
				iggy::toggleTargetInteractionEffect(Id("target:door"), false),
				iggy::pickupItemInteractionEffect(Id("target:combo"), drop.id),
			}),
		}),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({
			initial,
			{ Frame({ iggy::playerInteractIntent(Id("target:combo")) }) },
		});

	Expect(result.ticks.size() == 1, "combo runner should produce a report tick");
	Expect(result.ticks[0].report.acceptedCommandCount == 1, "combo runner report should count accepted command");
	Expect(result.ticks[0].report.interactionChanged, "combo runner report should mark interaction change");
	Expect(result.ticks[0].report.inventoryChanged, "combo runner report should mark inventory change");
	Expect(result.ticks[0].report.pickedUpCount == 1, "combo runner report should count pickup");
}

void TestExplicitWorldOverloadAppliesToEveryFrame()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(session);
	const std::vector<iggy::runtime::RuntimeGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
		Frame({ iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
	};
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, frames }, emptyWorld);

	Expect(result.ticks.size() == 2, "explicit-world runner should produce one tick per frame");
	Expect(NearVec(result.ticks[0].frame.state.session.player.position, { 1.0F, 0.0F }), "explicit world should affect first runner frame");
	Expect(NearVec(result.ticks[1].frame.state.session.player.position, { 2.0F, 0.0F }), "explicit world should affect later runner frames");
	Expect(NearVec(result.finalState.session.player.position, { 2.0F, 0.0F }), "explicit-world runner should carry movement across frames");
}

void TestInputStateAndFrameVectorAreNotMutated()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:immutable", "item:immutable", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:immutable", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));
	const std::vector<iggy::runtime::RuntimeGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerInteractIntent(target.id) }),
	};
	const iggy::runtime::RuntimeGameplayFrameRunnerInput input { initial, frames };

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run(input);

	Expect(result.ticks.size() == 1 && result.ticks[0].report.pickedUpCount == 1, "immutability setup should pick up item");
	ExpectPlayerAgent(input.initialState.session.player, initial.session.player, "runner should not mutate input session player");
	Expect(input.initialState.inventory.inventory.stacks.empty(), "runner should not mutate input inventory");
	Expect(input.initialState.inventory.drops.drops.size() == 1 && input.initialState.inventory.drops.drops[0].enabled, "runner should not mutate input drops");
	Expect(input.frames.size() == 1 && input.frames[0].playerIntents.size() == 1, "runner should not mutate input frame vector");
	Expect(input.frames[0].playerIntents[0].targetId == target.id, "runner should not mutate input player intent payload");
}

} // namespace

int main()
{
	TestEmptyFramesReturnInitialStateUnchanged();
	TestOneFrameMatchesGameplayFrameStepBehavior();
	TestMultipleMovementFramesCarryPlayerPositionForward();
	TestInteractionToggleCarriesIntoLaterFrames();
	TestPickupCarriesIntoLaterFrames();
	TestReportsCollectedPerFrameWithExpectedFacts();
	TestExplicitWorldOverloadAppliesToEveryFrame();
	TestInputStateAndFrameVectorAreNotMutated();

	return Failures;
}
