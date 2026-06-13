#include <cstdlib>
#include <type_traits>
#include <utility>
#include <vector>

#include "runtime/RuntimeGameplayFrameRunner.hpp"
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

const iggy::ResourceId PlayerId { "player:gameplay-frame-runner-acceptance" };

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
	session.level.map.id = Id("level:gameplay-frame-runner-acceptance");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 41;
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
	Expect(result.built, "gameplay frame runner acceptance target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "gameplay frame runner acceptance effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "gameplay frame runner acceptance inventory fixture should build");
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

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "gameplay frame runner acceptance collision fixture should build");
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

void TestMultiFrameMovementCarriesStateForward()
{
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer());
	const std::vector<iggy::runtime::RuntimeGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerMoveToPointIntent({ 1.0F, 0.0F }) }),
		Frame({ iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
		Frame({ iggy::playerMoveToPointIntent({ 3.0F, 0.0F }) }),
	};

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, frames });

	Expect(result.ticks.size() == frames.size(), "movement acceptance runner should produce one tick per frame");
	Expect(NearVec(result.ticks[0].frame.state.session.player.position, { 1.0F, 0.0F }), "first movement tick should advance from initial position");
	Expect(NearVec(result.ticks[1].frame.state.session.player.position, { 2.0F, 0.0F }), "second movement tick should carry first output");
	Expect(NearVec(result.ticks[2].frame.state.session.player.position, { 3.0F, 0.0F }), "third movement tick should carry second output");
	Expect(NearVec(result.finalState.session.player.position, { 3.0F, 0.0F }), "movement acceptance runner should return accumulated final position");
	Expect(result.ticks[0].report.acceptedCommandCount == 1 && result.ticks[1].report.acceptedCommandCount == 1 && result.ticks[2].report.acceptedCommandCount == 1, "movement reports should stay in frame order");
}

void TestToggleCarriesAcrossFrames()
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

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({
			initial,
			{
				Frame({ iggy::playerInteractIntent(Id("target:lever")) }),
				Frame({ iggy::playerInteractIntent(Id("target:door")) }),
			},
		});

	Expect(result.ticks.size() == 2, "toggle acceptance runner should produce two ticks");
	ExpectTargetEnabled(result.ticks[0].frame.state.interaction.targets, Id("target:door"), false, "first frame should toggle target B");
	ExpectTargetEnabled(result.ticks[1].frame.state.interaction.targets, Id("target:door"), false, "second frame should observe carried target B state");
	Expect(result.ticks[0].report.interactionChanged, "first toggle report should record interaction change");
	Expect(!result.ticks[1].report.interactionChanged, "second disabled-target report should not invent another interaction change");
}

void TestPickupConsumesDropAcrossFrames()
{
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({
			initial,
			{
				Frame({ iggy::playerInteractIntent(target.id) }),
				Frame({ iggy::playerInteractIntent(target.id) }),
			},
		});

	Expect(result.ticks.size() == 2, "pickup acceptance runner should produce two ticks");
	Expect(result.ticks[0].report.pickedUpCount == 1, "first pickup frame should pick up drop");
	Expect(result.ticks[1].report.pickedUpCount == 0, "second pickup frame should not pick up consumed drop again");
	Expect(result.ticks[1].report.pickupNotReadyCount == 1, "second pickup frame should report consumed drop as not ready");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:potion", 2) }), "pickup acceptance runner should carry picked-up stack");
	Expect(result.finalState.inventory.drops.drops.size() == 1 && !result.finalState.inventory.drops.drops[0].enabled, "pickup acceptance runner should carry disabled drop");
}

void TestMixedInteractionAndPickupCarryBothStatePackets()
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
			{
				Frame({ iggy::playerInteractIntent(Id("target:combo")) }),
				Frame({ iggy::playerInteractIntent(Id("target:door")) }),
			},
		});

	Expect(result.ticks.size() == 2, "mixed acceptance runner should produce two ticks");
	ExpectTargetEnabled(result.finalState.interaction.targets, Id("target:door"), false, "mixed runner should carry toggled interaction target");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:key", 1) }), "mixed runner should carry picked-up inventory");
	Expect(result.ticks[0].report.interactionChanged && result.ticks[0].report.pickedUpCount == 1, "first mixed report should include interaction and pickup facts");
	Expect(!result.ticks[1].report.interactionChanged && result.ticks[1].report.pickedUpCount == 0, "second mixed report should observe carried state without extra mutation");
}

void TestQueueRejectionInMiddleFrameDoesNotMutateAndLaterFramesRun()
{
	const iggy::InteractionTarget2D target = Target("target:queued", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:queued", "item:queued", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:queued", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }), fullQueue);
	iggy::runtime::RuntimeCommandQueueConfig fullQueueConfig;
	fullQueueConfig.maxFrames = 1;

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({
			initial,
			{
				Frame({ iggy::playerMoveToPointIntent({ 1.0F, 0.0F }) }, {}, fullQueueConfig),
				Frame({ iggy::playerInteractIntent(target.id) }, {}, fullQueueConfig),
				Frame({ iggy::playerInteractIntent(target.id) }),
			},
		});

	Expect(result.ticks.size() == 3, "queue rejection acceptance runner should keep one tick per input frame");
	Expect(result.ticks[1].frame.frame.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "middle frame should preserve queue rejection diagnostics");
	Expect(NearVec(result.ticks[1].frame.state.session.player.position, { 0.0F, 0.0F }), "queue-rejected middle frame should not move player");
	Expect(result.ticks[1].report.pickedUpCount == 0 && !result.ticks[1].report.inventoryChanged, "queue-rejected middle frame should not mutate inventory");
	Expect(result.ticks[2].report.pickedUpCount == 1, "later unbounded frame should still run from carried state");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:queued", 1) }), "later frame should carry pickup into final state");
}

void TestExplicitCollisionWorldAppliesAcrossMovementAndPickupFrames()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:explicit", "item:explicit", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:explicit", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(session, interaction, InventoryState({}, { drop }));
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run(
			{
				initial,
				{
					Frame({ iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
					Frame({ iggy::playerInteractIntent(target.id) }),
				},
			},
			emptyWorld);

	Expect(result.ticks.size() == 2, "explicit-world acceptance runner should produce one tick per frame");
	Expect(NearVec(result.ticks[0].frame.state.session.player.position, { 1.0F, 0.0F }), "explicit world should apply to movement frame");
	Expect(result.ticks[1].report.pickedUpCount == 1, "explicit world should allow later pickup frame to use carried post-move position");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:explicit", 1) }), "explicit-world runner should carry pickup into final state");
}

void TestEmptyFrameListReturnsInitialStateAndNoTicks()
{
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer({ 4.0F, 5.0F }), {}, InventoryState({ Stack("item:held", 3) }, {}));

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, {} });

	Expect(result.ticks.empty(), "empty acceptance runner should produce no ticks");
	ExpectPlayerAgent(result.finalState.session.player, initial.session.player, "empty acceptance runner should preserve session player");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:held", 3) }), "empty acceptance runner should preserve explicit inventory");
}

void TestInitialStateAndFramesAreNotMutated()
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

	Expect(result.ticks.size() == 1 && result.ticks[0].report.pickedUpCount == 1, "immutability acceptance setup should pick up item");
	ExpectPlayerAgent(input.initialState.session.player, initial.session.player, "acceptance runner should not mutate input session");
	Expect(input.initialState.inventory.inventory.stacks.empty(), "acceptance runner should not mutate input inventory");
	Expect(input.initialState.inventory.drops.drops.size() == 1 && input.initialState.inventory.drops.drops[0].enabled, "acceptance runner should not mutate input drops");
	Expect(input.frames.size() == 1 && input.frames[0].playerIntents.size() == 1, "acceptance runner should not mutate input frame vector");
	Expect(input.frames[0].playerIntents[0].targetId == target.id, "acceptance runner should not mutate input intent payload");
}

} // namespace

int main()
{
	TestMultiFrameMovementCarriesStateForward();
	TestToggleCarriesAcrossFrames();
	TestPickupConsumesDropAcrossFrames();
	TestMixedInteractionAndPickupCarryBothStatePackets();
	TestQueueRejectionInMiddleFrameDoesNotMutateAndLaterFramesRun();
	TestExplicitCollisionWorldAppliesAcrossMovementAndPickupFrames();
	TestEmptyFrameListReturnsInitialStateAndNoTicks();
	TestInitialStateAndFramesAreNotMutated();

	return Failures;
}
