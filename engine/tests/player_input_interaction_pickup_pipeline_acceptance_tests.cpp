#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionPickupFrameReporter.hpp"
#include "runtime/RuntimePlayerInputInteractionPickupFrameStep.hpp"
#include "support/CommandFrameFixtures.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::CommandFrame;
using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:input-interaction-pickup-acceptance" };

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
	session.level.map.id = iggy::ResourceId { "level:input-interaction-pickup-acceptance" };
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 101;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId) });
}

iggy::runtime::RuntimePlayerInputGatedFrameStepInput PlayerInput(
	iggy::runtime::RuntimeSessionState session,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::PlayerInputContext2D context = {},
	iggy::runtime::RuntimeCommandQueueState queue = {},
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	return {
		{
			session,
			queue,
			queueConfig,
			PlayerId,
			context,
			intents,
			{ 1.5F, 1.5F },
			PlayerConfig(),
			NpcConfig(),
		},
	};
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "interaction pickup acceptance target registry should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "interaction pickup acceptance effect catalog should build");
	return result.catalog;
}

iggy::runtime::RuntimeInteractionState InteractionState(
	iggy::InteractionTarget2DRegistry targets,
	iggy::InteractionEffectCatalog2D effects)
{
	return { targets, effects };
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { iggy::ResourceId { itemId }, count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "interaction pickup acceptance inventory should build");
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

iggy::runtime::RuntimePlayerInputInteractionStateApplyFrameInput InteractionInput(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::runtime::RuntimeInteractionState interaction,
	iggy::InteractionReach2DConfig reach = {})
{
	return { playerInput, interaction, reach, {}, {} };
}

iggy::runtime::RuntimePlayerInputInteractionPickupFrameInput Input(
	iggy::runtime::RuntimePlayerInputInteractionStateApplyFrameInput interactionInput,
	iggy::runtime::RuntimeInventoryState inventory,
	iggy::runtime::RuntimePickupConfig pickup = {})
{
	return { interactionInput, inventory, pickup };
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "interaction pickup acceptance collision world should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({
		Object(iggy::ResourceId { "wall:east" }, { { 0.5F, -0.5F }, { 1.5F, 0.5F } }),
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

void ExpectTarget(const iggy::InteractionTarget2D &actual, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void ExpectRegistryTargets(
	const iggy::InteractionTarget2DRegistry &registry,
	const std::vector<iggy::InteractionTarget2D> &expected,
	const char *message)
{
	Expect(registry.targets().size() == expected.size(), message);
	if (registry.targets().size() != expected.size())
		return;
	for (std::size_t index = 0; index < expected.size(); ++index)
		ExpectTarget(registry.targets()[index], expected[index], message);
}

bool HasReportEvent(
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport &report,
	iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent event)
{
	for (const iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent actual : report.events) {
		if (actual == event)
			return true;
	}
	return false;
}

iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport Report(
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult &result)
{
	return iggy::runtime::RuntimePlayerInputInteractionPickupFrameReporter {}.report(result);
}

void TestInteractPickupItemEffectPicksUpItem()
{
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry({ target }),
		Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }), interaction),
				inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report = Report(result);

	Expect(result.interaction.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "pickup-only acceptance should not mutate interaction targets");
	ExpectRegistryTargets(result.interactionState.targets, { target }, "pickup-only acceptance should preserve interaction state");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "pickup-only acceptance should add item to inventory");
	Expect(result.inventory.drops.drops.size() == 1, "pickup-only acceptance should preserve consumed drop in disable mode");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "pickup-only acceptance should disable consumed drop");
	}
	Expect(report.acceptedCommandCount == 1, "pickup-only report should count accepted command");
	Expect(report.interactionDeferredCount == 1, "pickup-only report should count deferred pickup effect");
	Expect(report.pickedUpCount == 1, "pickup-only report should count item pickup");
	Expect(report.inventoryChanged, "pickup-only report should mark inventory changed");
	Expect(HasReportEvent(report, iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::ItemPickedUp), "pickup-only report should include ItemPickedUp");
}

void TestToggleTargetPlusPickupItemUpdatesBothExplicitStates()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:combo", iggy::InteractionTarget2DKind::Usable),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expectedTargets = targets;
	expectedTargets[1].enabled = false;
	const iggy::LevelItemDrop2D drop = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry(targets),
		Catalog({
			Entry("target:combo", {
				iggy::toggleTargetInteractionEffect(targets[1].id, false),
				iggy::pickupItemInteractionEffect(targets[0].id, drop.id),
			}),
		}));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(targets[0].id) }), interaction),
				inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report = Report(result);

	ExpectRegistryTargets(result.interactionState.targets, expectedTargets, "combo acceptance should toggle interaction target");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:key", 1) }), "combo acceptance should add picked-up item");
	Expect(result.interaction.events.events.size() == 1, "combo acceptance should surface local target-toggled event");
	Expect(report.interactionAppliedCount == 1, "combo report should count interaction application");
	Expect(report.interactionEventCount == 1, "combo report should count local interaction event");
	Expect(report.pickedUpCount == 1, "combo report should count item pickup");
	Expect(report.interactionMutated, "combo report should mark interaction mutation");
	Expect(report.inventoryChanged, "combo report should mark inventory change");
	Expect(HasReportEvent(report, iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::InteractionEffectApplied), "combo report should include interaction application event");
	Expect(HasReportEvent(report, iggy::runtime::RuntimePlayerInputInteractionPickupFrameEvent::ItemPickedUp), "combo report should include item pickup event");
}

void TestExistingInventoryStackIncrements()
{
	const iggy::InteractionTarget2D target = Target("target:stack", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 3);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 4) }, { drop });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry({ target }),
		Catalog({ Entry("target:stack", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }), interaction),
				inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report = Report(result);

	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 7) }), "existing-stack acceptance should increment inventory count");
	Expect(report.pickedUpCount == 1, "existing-stack acceptance should report item pickup");
	Expect(report.inventoryChanged, "existing-stack acceptance should mark inventory changed");
}

void TestContextBlockedInteractDoesNotMutateExplicitStates()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:blocked", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry({ target }),
		Catalog({ Entry("target:blocked", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, context), interaction),
				inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report = Report(result);

	Expect(result.interaction.playerInput.command.intake.mapping.gateIssues.size() == 1, "context-blocked acceptance should preserve gate diagnostics");
	ExpectRegistryTargets(result.interactionState.targets, { target }, "context-blocked acceptance should preserve interaction targets");
	ExpectInventoryState(result.inventory, inventory, "context-blocked acceptance should preserve inventory");
	Expect(report.blockedIntentCount == 1, "context-blocked report should count blocked intent");
	Expect(report.pickedUpCount == 0 && report.pickupNotReadyCount == 0 && report.pickupFailedCount == 0, "context-blocked report should count no pickup facts");
	Expect(!report.inventoryChanged, "context-blocked report should not mark inventory changed");
}

void TestQueueRejectedMappedInteractDoesNotApplyOrPickup()
{
	const iggy::InteractionTarget2D target = Target("target:queued", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry({ target }),
		Catalog({ Entry("target:queued", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, {}, fullQueue, { 1 }),
					interaction),
				inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report = Report(result);

	Expect(result.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected acceptance should preserve queue rejection");
	Expect(result.interaction.playerInput.command.intake.mapping.frame.commands.size() == 1, "queue-rejected acceptance should preserve mapped interact diagnostics");
	ExpectRegistryTargets(result.interactionState.targets, { target }, "queue-rejected acceptance should preserve interaction targets");
	ExpectInventoryState(result.inventory, inventory, "queue-rejected acceptance should preserve inventory");
	Expect(report.acceptedCommandCount == 1, "queue-rejected report should preserve accepted mapped command count");
	Expect(report.pickedUpCount == 0 && report.pickupNotReadyCount == 0 && report.pickupFailedCount == 0, "queue-rejected report should count no pickup facts");
	Expect(!report.inventoryChanged, "queue-rejected report should not mark inventory changed");
}

void TestMoveThenInteractPickupUsesPostMovePosition()
{
	const iggy::InteractionTarget2D target = Target("target:after_move", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:after_move", "item:after_move", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry({ target }),
		Catalog({ Entry("target:after_move", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), {
						iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
						iggy::playerInteractIntent(target.id),
					}),
					interaction),
				inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report = Report(result);

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-pickup acceptance should preserve post-move position");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "move-then-pickup acceptance should pick up after movement");
	if (!result.pickup.entries.empty())
		Expect(NearVec(result.pickup.entries[0].result.pickup.plan.actorPosition, { 1.0F, 0.0F }), "move-then-pickup acceptance should plan pickup from post-move position");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:after_move", 1) }), "move-then-pickup acceptance should add item");
	Expect(report.pickedUpCount == 1, "move-then-pickup report should count pickup");
}

void TestExplicitCollisionOverrideAffectsReachAndPickup()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:explicit", "item:explicit", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry({ target }),
		Catalog({ Entry("target:explicit", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(session, {
						iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
						iggy::playerInteractIntent(target.id),
					}),
					interaction),
				inventory),
			emptyWorld);
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport report = Report(result);

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit-world pickup acceptance should use explicit movement world");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "explicit-world pickup acceptance should pick up after movement override");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:explicit", 1) }), "explicit-world pickup acceptance should add item");
	Expect(report.pickedUpCount == 1, "explicit-world pickup report should count pickup");
}

void TestPickupNotReadyKeepsInventoryUnchanged()
{
	const iggy::InteractionTarget2D missingTarget = Target("target:missing_drop", iggy::InteractionTarget2DKind::Pickup);
	const iggy::InteractionTarget2D disabledTarget = Target("target:disabled_drop", iggy::InteractionTarget2DKind::Pickup);
	const iggy::InteractionTarget2D farTarget = Target("target:far_drop", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D disabled = Drop("drop:disabled", "item:disabled", 1, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::LevelItemDrop2D far = Drop("drop:far", "item:far", 1, { 5.0F, 0.0F }, 1.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { disabled, far });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry({ missingTarget, disabledTarget, farTarget }),
		Catalog({
			Entry("target:missing_drop", { iggy::pickupItemInteractionEffect(missingTarget.id, iggy::ResourceId { "drop:missing" }) }),
			Entry("target:disabled_drop", { iggy::pickupItemInteractionEffect(disabledTarget.id, disabled.id) }),
			Entry("target:far_drop", { iggy::pickupItemInteractionEffect(farTarget.id, far.id) }),
		}));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult missingResult =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(missingTarget.id) }), interaction), inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult disabledResult =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(disabledTarget.id) }), interaction), inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult farResult =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(farTarget.id) }), interaction), inventory));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport missingReport = Report(missingResult);
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport disabledReport = Report(disabledResult);
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameReport farReport = Report(farResult);

	Expect(missingResult.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady, "missing-drop acceptance should report pickup not ready");
	Expect(disabledResult.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady, "disabled-drop acceptance should report pickup not ready");
	Expect(farResult.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady, "far-drop acceptance should report pickup not ready");
	ExpectInventoryState(missingResult.inventory, inventory, "missing-drop acceptance should preserve inventory");
	ExpectInventoryState(disabledResult.inventory, inventory, "disabled-drop acceptance should preserve inventory");
	ExpectInventoryState(farResult.inventory, inventory, "far-drop acceptance should preserve inventory");
	Expect(missingReport.interactionDeferredCount == 1 && disabledReport.interactionDeferredCount == 1 && farReport.interactionDeferredCount == 1, "pickup-not-ready reports should preserve deferred pickup requests");
	Expect(missingReport.pickupNotReadyCount == 1 && disabledReport.pickupNotReadyCount == 1 && farReport.pickupNotReadyCount == 1, "pickup-not-ready reports should count not-ready pickup");
	Expect(!missingReport.inventoryChanged && !disabledReport.inventoryChanged && !farReport.inventoryChanged, "pickup-not-ready reports should not mark inventory changed");
}

} // namespace

int main()
{
	TestInteractPickupItemEffectPicksUpItem();
	TestToggleTargetPlusPickupItemUpdatesBothExplicitStates();
	TestExistingInventoryStackIncrements();
	TestContextBlockedInteractDoesNotMutateExplicitStates();
	TestQueueRejectedMappedInteractDoesNotApplyOrPickup();
	TestMoveThenInteractPickupUsesPostMovePosition();
	TestExplicitCollisionOverrideAffectsReachAndPickup();
	TestPickupNotReadyKeepsInventoryUnchanged();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
