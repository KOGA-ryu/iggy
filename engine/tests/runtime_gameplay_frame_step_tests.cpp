#include <cstdlib>
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

const iggy::ResourceId PlayerId { "player:gameplay-frame" };

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
	session.level.map.id = Id("level:gameplay-frame");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 13;
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
	Expect(result.built, "runtime gameplay frame target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime gameplay frame effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime gameplay frame inventory fixture should build");
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
	Expect(build.built, "runtime gameplay frame collision fixture should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({
		Object(Id("wall:east"), { { 0.5F, -0.5F }, { 1.5F, 0.5F } }),
	});
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

void TestEmptyInputReturnsGameplayStateThroughExistingSemanticsAndReport()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer());

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, {}));

	Expect(result.frame.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::Ran, "empty gameplay frame should run existing player input path");
	Expect(result.report.acceptedCommandCount == 0, "empty gameplay frame report should have no accepted commands");
	Expect(result.report.pickedUpCount == 0, "empty gameplay frame report should have no pickups");
	Expect(result.state.commandQueue.frames.empty(), "empty gameplay frame should return drained queue state");
	Expect(result.state.interaction.targets.targets().empty(), "empty gameplay frame should preserve empty interaction state");
	Expect(result.state.inventory.inventory.stacks.empty(), "empty gameplay frame should preserve empty inventory");
	Expect(HasEvent(result.report.events, iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::CommandFrameQueued), "empty gameplay frame report should include queued frame fact");
}

void TestMoveIntentUpdatesReturnedGameplaySession()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer({ 0.0F, 0.0F }));

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerMoveToPointIntent({ 1.0F, 0.0F }) }));

	Expect(NearVec(result.state.session.player.position, { 1.0F, 0.0F }), "gameplay frame should return moved player in aggregate session");
	Expect(result.report.acceptedCommandCount == 1, "gameplay frame report should count accepted move command");
}

void TestInteractToggleTargetUpdatesReturnedInteractionState()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:source"),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry(targets),
		Catalog({
			Entry("target:source", { iggy::toggleTargetInteractionEffect(Id("target:door"), false) }),
		}),
	};
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction);

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(Id("target:source")) }));

	ExpectTargetEnabled(result.state.interaction.targets, Id("target:door"), false, "gameplay frame should return toggled interaction target");
	Expect(result.report.interactionAppliedCount == 1, "gameplay frame report should count applied interaction effect");
	Expect(result.report.interactionMutated, "gameplay frame report should mark interaction mutation");
}

void TestInteractPickupItemUpdatesReturnedInventoryState()
{
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction, inventory);

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }));

	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:potion", 2) }), "gameplay frame should return picked-up item in inventory");
	Expect(result.state.inventory.drops.drops.size() == 1 && !result.state.inventory.drops.drops[0].enabled, "gameplay frame should return consumed drop state");
	Expect(result.report.pickedUpCount == 1, "gameplay frame report should count pickup");
	Expect(result.report.inventoryChanged, "gameplay frame report should mark inventory changed");
}

void TestCombinedToggleAndPickupUpdatesBothExplicitStatePackets()
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
	const iggy::runtime::RuntimeGameplayState state =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(Id("target:combo")) }));

	ExpectTargetEnabled(result.state.interaction.targets, Id("target:door"), false, "combo gameplay frame should return updated interaction state");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:key", 1) }), "combo gameplay frame should return updated inventory state");
	Expect(result.report.interactionAppliedCount == 1 && result.report.pickedUpCount == 1, "combo gameplay frame report should count both interaction and pickup facts");
}

void TestContextBlockedInteractLeavesExplicitStatesUnchanged()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:blocked", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:blocked", "item:blocked", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:blocked", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction, inventory);

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }, context));

	Expect(result.report.blockedIntentCount == 1, "context-blocked gameplay frame should report blocked intent");
	Expect(result.state.interaction.targets.find(target.id)->enabled == target.enabled, "context-blocked gameplay frame should preserve interaction state");
	Expect(result.state.inventory.inventory.stacks.empty(), "context-blocked gameplay frame should preserve inventory");
	Expect(result.state.inventory.drops.drops.size() == 1 && result.state.inventory.drops.drops[0].enabled, "context-blocked gameplay frame should preserve drop");
}

void TestQueueRejectedInputPreservesDiagnosticsAndDoesNotMutateExplicitStates()
{
	const iggy::InteractionTarget2D target = Target("target:queued", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:queued", "item:queued", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:queued", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeGameplayState state =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }), fullQueue);
	iggy::runtime::RuntimeCommandQueueConfig queueConfig;
	queueConfig.maxFrames = 1;

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }, {}, queueConfig));

	Expect(result.frame.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected gameplay frame should preserve nested rejection");
	Expect(result.frame.interaction.playerInput.command.intake.mapping.frame.commands.size() == 1, "queue-rejected gameplay frame should preserve mapped command diagnostics");
	Expect(result.state.commandQueue.frames.size() == 1, "queue-rejected gameplay frame should preserve original queue");
	Expect(result.state.inventory.inventory.stacks.empty(), "queue-rejected gameplay frame should not consume pickup");
	Expect(result.state.inventory.drops.drops.size() == 1 && result.state.inventory.drops.drops[0].enabled, "queue-rejected gameplay frame should preserve drops");
}

void TestExplicitCollisionWorldOverloadAffectsMovementReachAndPickup()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:explicit", "item:explicit", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:explicit", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState state = GameplayState(session, interaction, InventoryState({}, { drop }));
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(
			Input(state, {
				iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}),
			emptyWorld);

	Expect(NearVec(result.state.session.player.position, { 1.0F, 0.0F }), "explicit-world gameplay frame should return movement override result");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:explicit", 1) }), "explicit-world gameplay frame should pick up after movement override");
}

void TestOriginalGameplayStateInputIsNotMutated()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:immutable", "item:immutable", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		Catalog({ Entry("target:immutable", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction, inventory);
	const iggy::runtime::RuntimeGameplayFrameInput input =
		Input(state, { iggy::playerInteractIntent(target.id) });

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(input);

	Expect(result.report.pickedUpCount == 1, "immutability setup should pick up item");
	ExpectPlayerAgent(input.state.session.player, state.session.player, "gameplay frame should not mutate input session player");
	Expect(input.state.interaction.targets.find(target.id)->enabled == target.enabled, "gameplay frame should not mutate input interaction targets");
	Expect(input.state.inventory.inventory.stacks.empty(), "gameplay frame should not mutate input inventory");
	Expect(input.state.inventory.drops.drops.size() == 1 && input.state.inventory.drops.drops[0].enabled, "gameplay frame should not mutate input drops");
}

} // namespace

int main()
{
	TestEmptyInputReturnsGameplayStateThroughExistingSemanticsAndReport();
	TestMoveIntentUpdatesReturnedGameplaySession();
	TestInteractToggleTargetUpdatesReturnedInteractionState();
	TestInteractPickupItemUpdatesReturnedInventoryState();
	TestCombinedToggleAndPickupUpdatesBothExplicitStatePackets();
	TestContextBlockedInteractLeavesExplicitStatesUnchanged();
	TestQueueRejectedInputPreservesDiagnosticsAndDoesNotMutateExplicitStates();
	TestExplicitCollisionWorldOverloadAffectsMovementReachAndPickup();
	TestOriginalGameplayStateInputIsNotMutated();

	return Failures;
}
