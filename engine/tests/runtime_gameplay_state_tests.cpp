#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

template <typename T, typename = void>
struct HasNpcActorsField : std::false_type {
};

template <typename T>
struct HasNpcActorsField<T, std::void_t<decltype(&T::npcActors)>> : std::true_type {
};

template <typename T, typename = void>
struct HasNpcControlsField : std::false_type {
};

template <typename T>
struct HasNpcControlsField<T, std::void_t<decltype(&T::npcControls)>> : std::true_type {
};

static_assert(!HasNpcActorsField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasNpcControlsField<iggy::runtime::RuntimeSessionState>::value);

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::InteractionTarget2D Target(const char *id)
{
	return {
		Id(id),
		iggy::InteractionTarget2DKind::Usable,
		{ 1.0F, 2.0F },
		0.5F,
		true,
	};
}

iggy::InteractionTarget2DRegistry Targets(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "runtime gameplay state target fixture should build");
	return result.registry;
}

iggy::InteractionEffectCatalog2D Effects(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result =
		iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime gameplay state effect fixture should build");
	return result.catalog;
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime gameplay state inventory fixture should build");
	return result.inventory;
}

iggy::LevelItemDrop2DRegistry Drops(std::vector<iggy::LevelItemDrop2D> drops)
{
	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);
	Expect(result.built, "runtime gameplay state drop fixture should build");
	return result.registry;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame(iggy::ResourceId actorId)
{
	return {
		{ iggy::runtime::GameplayCommand2DFactory {}.wait(actorId) },
	};
}

iggy::NpcActorState2D NpcActor(const char *npcId, iggy::Vec2 position = { 0.0F, 0.0F })
{
	return {
		Id(npcId),
		Id("profile:runtime-gameplay-state"),
		Id("faction:runtime-gameplay-state"),
		position,
		{},
		true,
	};
}

iggy::NpcActorControlState2D NpcControl(
	const char *npcId,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still)
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		iggy::idleNpcBehaviorState(),
		moveMode,
	};
}

bool SameCommand(
	const iggy::runtime::GameplayCommand2D &actual,
	const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameFrame(
	const iggy::runtime::GameplayCommandFrame2D &actual,
	const iggy::runtime::GameplayCommandFrame2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

bool SameNpcActor(const iggy::NpcActorState2D &actual, const iggy::NpcActorState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.aiProfileId == expected.aiProfileId
		&& actual.factionId == expected.factionId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.present == expected.present;
}

bool SameNpcActors(
	const iggy::NpcActorState2DRegistry &actual,
	const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size())
		return false;
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		if (!SameNpcActor(actual.actors[index], expected.actors[index]))
			return false;
	}
	return true;
}

bool SameObjective(const iggy::NpcObjective &actual, const iggy::NpcObjective &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
}

bool SameBehavior(const iggy::NpcBehaviorState &actual, const iggy::NpcBehaviorState &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
}

bool SameNpcControl(
	const iggy::NpcActorControlState2D &actual,
	const iggy::NpcActorControlState2D &expected)
{
	return actual.npcId == expected.npcId
		&& SameObjective(actual.objective, expected.objective)
		&& SameBehavior(actual.behavior, expected.behavior)
		&& actual.moveMode == expected.moveMode;
}

bool SameNpcControls(
	const iggy::NpcActorControlState2DRegistry &actual,
	const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size())
		return false;
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (!SameNpcControl(actual.entries[index], expected.entries[index]))
			return false;
	}
	return true;
}

void TestDefaultStateHasDefaultChildren()
{
	const iggy::runtime::RuntimeGameplayState state;

	Expect(state.session.tickIndex == 0, "default runtime gameplay state should default session tick");
	Expect(!state.session.hasPlayer, "default runtime gameplay state should default missing player");
	Expect(state.commandQueue.frames.empty(), "default runtime gameplay state should default empty command queue");
	Expect(state.interaction.targets.targets().empty(), "default runtime gameplay state should default empty interaction targets");
	Expect(state.interaction.effects.entries().empty(), "default runtime gameplay state should default empty interaction effects");
	Expect(state.inventory.inventory.stacks.empty(), "default runtime gameplay state should default empty inventory");
	Expect(state.inventory.drops.drops.empty(), "default runtime gameplay state should default empty drops");
	Expect(state.npcActors.actors.empty(), "default runtime gameplay state should default empty NPC actors");
	Expect(state.npcControls.entries.empty(), "default runtime gameplay state should default empty NPC controls");
}

void TestExplicitStatePreservesChildStateByValue()
{
	iggy::runtime::RuntimeSessionState session;
	session.tickIndex = 12;
	session.hasPlayer = true;
	session.player.id = Id("actor:player");
	session.player.position = { 3.0F, 4.0F };

	iggy::runtime::RuntimeCommandQueueState queue;
	const iggy::runtime::GameplayCommandFrame2D frame = WaitFrame(Id("actor:player"));
	queue.frames.push_back(frame);

	const iggy::runtime::RuntimeInteractionState interaction {
		Targets({ Target("target:lever") }),
		Effects({
			{ Id("target:lever"), { iggy::inspectTextInteractionEffect(Id("target:lever"), "Lever") } },
		}),
	};
	const iggy::runtime::RuntimeInventoryState inventory {
		Inventory({ { Id("item:potion"), 3 } }),
		Drops({ { Id("drop:key"), Id("item:key"), 1, { 5.0F, 6.0F }, 0.25F, true } }),
	};
	const iggy::NpcActorState2DRegistry npcActors { { NpcActor("npc:actor", { 7.0F, 8.0F }) } };
	const iggy::NpcActorControlState2DRegistry npcControls { { NpcControl("npc:actor", iggy::NpcMoveMode::Run) } };

	const iggy::runtime::RuntimeGameplayState state {
		session,
		queue,
		interaction,
		inventory,
		npcActors,
		npcControls,
	};

	Expect(state.session.tickIndex == 12, "runtime gameplay state should preserve session tick");
	Expect(state.session.hasPlayer && state.session.player.id == Id("actor:player"), "runtime gameplay state should preserve player");
	Expect(NearVec(state.session.player.position, { 3.0F, 4.0F }), "runtime gameplay state should preserve player position");
	Expect(state.commandQueue.frames.size() == 1 && SameFrame(state.commandQueue.frames[0], frame), "runtime gameplay state should preserve command queue frames");
	Expect(state.interaction.targets.targets().size() == 1 && state.interaction.targets.targets()[0].id == Id("target:lever"), "runtime gameplay state should preserve interaction targets");
	Expect(state.interaction.effects.entries().size() == 1 && state.interaction.effects.entries()[0].targetId == Id("target:lever"), "runtime gameplay state should preserve interaction effects");
	Expect(state.inventory.inventory.stacks.size() == 1 && state.inventory.inventory.stacks[0].itemId == Id("item:potion") && state.inventory.inventory.stacks[0].count == 3, "runtime gameplay state should preserve inventory stacks");
	Expect(state.inventory.drops.drops.size() == 1 && state.inventory.drops.drops[0].id == Id("drop:key"), "runtime gameplay state should preserve drops");
	Expect(SameNpcActors(state.npcActors, npcActors), "runtime gameplay state should preserve NPC actor registry");
	Expect(SameNpcControls(state.npcControls, npcControls), "runtime gameplay state should preserve NPC control registry");
}

void TestCopiedStatePreservesChildren()
{
	iggy::runtime::RuntimeGameplayState original;
	original.session.tickIndex = 7;
	original.commandQueue.frames.push_back(WaitFrame(Id("actor:a")));
	original.interaction.targets = Targets({ Target("target:a") });
	original.inventory.inventory = Inventory({ { Id("item:a"), 2 } });
	original.npcActors = { { NpcActor("npc:a", { 2.0F, 3.0F }) } };
	original.npcControls = { { NpcControl("npc:a", iggy::NpcMoveMode::Jog) } };

	const iggy::runtime::RuntimeGameplayState copy = original;

	Expect(copy.session.tickIndex == 7, "copied runtime gameplay state should preserve session");
	Expect(copy.commandQueue.frames.size() == 1 && SameFrame(copy.commandQueue.frames[0], original.commandQueue.frames[0]), "copied runtime gameplay state should preserve queue");
	Expect(copy.interaction.targets.targets().size() == 1 && copy.interaction.targets.targets()[0].id == Id("target:a"), "copied runtime gameplay state should preserve interaction state");
	Expect(copy.inventory.inventory.stacks.size() == 1 && copy.inventory.inventory.stacks[0].itemId == Id("item:a"), "copied runtime gameplay state should preserve inventory state");
	Expect(SameNpcActors(copy.npcActors, original.npcActors), "copied runtime gameplay state should preserve NPC actors");
	Expect(SameNpcControls(copy.npcControls, original.npcControls), "copied runtime gameplay state should preserve NPC controls");
}

void TestReassigningCopiedChildrenDoesNotMutateOriginal()
{
	iggy::runtime::RuntimeGameplayState original;
	original.session.tickIndex = 1;
	original.commandQueue.frames.push_back(WaitFrame(Id("actor:original")));
	original.interaction.targets = Targets({ Target("target:original") });
	original.inventory.inventory = Inventory({ { Id("item:original"), 1 } });
	original.npcActors = { { NpcActor("npc:original", { 1.0F, 1.0F }) } };
	original.npcControls = { { NpcControl("npc:original", iggy::NpcMoveMode::Walk) } };

	iggy::runtime::RuntimeGameplayState copy = original;
	copy.session.tickIndex = 99;
	copy.commandQueue.frames = { WaitFrame(Id("actor:copy")) };
	copy.interaction.targets = Targets({ Target("target:copy") });
	copy.inventory.inventory = Inventory({ { Id("item:copy"), 5 } });
	copy.npcActors = { { NpcActor("npc:copy", { 5.0F, 5.0F }) } };
	copy.npcControls = { { NpcControl("npc:copy", iggy::NpcMoveMode::Sprint) } };

	Expect(copy.session.tickIndex == 99, "runtime gameplay state copy should accept reassigned session");
	Expect(copy.commandQueue.frames.size() == 1 && copy.commandQueue.frames[0].commands[0].actorId == Id("actor:copy"), "runtime gameplay state copy should accept reassigned queue");
	Expect(copy.interaction.targets.targets()[0].id == Id("target:copy"), "runtime gameplay state copy should accept reassigned interaction targets");
	Expect(copy.inventory.inventory.stacks[0].itemId == Id("item:copy"), "runtime gameplay state copy should accept reassigned inventory");
	Expect(copy.npcActors.actors[0].npcId == Id("npc:copy"), "runtime gameplay state copy should accept reassigned NPC actors");
	Expect(copy.npcControls.entries[0].npcId == Id("npc:copy"), "runtime gameplay state copy should accept reassigned NPC controls");
	Expect(original.session.tickIndex == 1, "reassigning copy should not mutate original session");
	Expect(original.commandQueue.frames[0].commands[0].actorId == Id("actor:original"), "reassigning copy should not mutate original queue");
	Expect(original.interaction.targets.targets()[0].id == Id("target:original"), "reassigning copy should not mutate original interaction state");
	Expect(original.inventory.inventory.stacks[0].itemId == Id("item:original"), "reassigning copy should not mutate original inventory state");
	Expect(original.npcActors.actors[0].npcId == Id("npc:original"), "reassigning copy should not mutate original NPC actors");
	Expect(original.npcControls.entries[0].npcId == Id("npc:original"), "reassigning copy should not mutate original NPC controls");
}

void TestConstructionDoesNotValidateOrRejectChildren()
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	iggy::runtime::RuntimeCommandQueueState queue;
	queue.frames.push_back({ { iggy::runtime::GameplayCommand2D {} } });
	iggy::runtime::RuntimeInteractionState interaction;
	interaction.targets = iggy::InteractionTarget2DRegistry { { {} } };
	iggy::runtime::RuntimeInventoryState inventory;
	inventory.inventory.stacks.push_back({ {}, 0 });
	inventory.drops.drops.push_back({});
	iggy::NpcActorState2DRegistry npcActors { { {} } };
	iggy::NpcActorControlState2DRegistry npcControls { { {} } };

	const iggy::runtime::RuntimeGameplayState state {
		session,
		queue,
		interaction,
		inventory,
		npcActors,
		npcControls,
	};

	Expect(state.session.hasPlayer, "runtime gameplay state should preserve session values without validation");
	Expect(state.commandQueue.frames.size() == 1, "runtime gameplay state should preserve queue values without validation");
	Expect(state.interaction.targets.targets().size() == 1, "runtime gameplay state should preserve interaction target values without validation");
	Expect(state.inventory.inventory.stacks.size() == 1, "runtime gameplay state should preserve inventory values without validation");
	Expect(state.inventory.inventory.stacks[0].itemId.empty() && state.inventory.inventory.stacks[0].count == 0, "runtime gameplay state should preserve invalid-looking inventory stack");
	Expect(state.inventory.drops.drops.size() == 1 && state.inventory.drops.drops[0].id.empty(), "runtime gameplay state should preserve invalid-looking drop");
	Expect(state.npcActors.actors.size() == 1 && state.npcActors.actors[0].npcId.empty(), "runtime gameplay state should preserve invalid-looking NPC actor");
	Expect(state.npcControls.entries.size() == 1 && state.npcControls.entries[0].npcId.empty(), "runtime gameplay state should preserve invalid-looking NPC control");
}

} // namespace

int main()
{
	TestDefaultStateHasDefaultChildren();
	TestExplicitStatePreservesChildStateByValue();
	TestCopiedStatePreservesChildren();
	TestReassigningCopiedChildrenDoesNotMutateOriginal();
	TestConstructionDoesNotValidateOrRejectChildren();

	return Failures;
}
