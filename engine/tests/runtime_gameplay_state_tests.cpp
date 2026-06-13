#include <cstdlib>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

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

	const iggy::runtime::RuntimeGameplayState state {
		session,
		queue,
		interaction,
		inventory,
	};

	Expect(state.session.tickIndex == 12, "runtime gameplay state should preserve session tick");
	Expect(state.session.hasPlayer && state.session.player.id == Id("actor:player"), "runtime gameplay state should preserve player");
	Expect(NearVec(state.session.player.position, { 3.0F, 4.0F }), "runtime gameplay state should preserve player position");
	Expect(state.commandQueue.frames.size() == 1 && SameFrame(state.commandQueue.frames[0], frame), "runtime gameplay state should preserve command queue frames");
	Expect(state.interaction.targets.targets().size() == 1 && state.interaction.targets.targets()[0].id == Id("target:lever"), "runtime gameplay state should preserve interaction targets");
	Expect(state.interaction.effects.entries().size() == 1 && state.interaction.effects.entries()[0].targetId == Id("target:lever"), "runtime gameplay state should preserve interaction effects");
	Expect(state.inventory.inventory.stacks.size() == 1 && state.inventory.inventory.stacks[0].itemId == Id("item:potion") && state.inventory.inventory.stacks[0].count == 3, "runtime gameplay state should preserve inventory stacks");
	Expect(state.inventory.drops.drops.size() == 1 && state.inventory.drops.drops[0].id == Id("drop:key"), "runtime gameplay state should preserve drops");
}

void TestCopiedStatePreservesChildren()
{
	iggy::runtime::RuntimeGameplayState original;
	original.session.tickIndex = 7;
	original.commandQueue.frames.push_back(WaitFrame(Id("actor:a")));
	original.interaction.targets = Targets({ Target("target:a") });
	original.inventory.inventory = Inventory({ { Id("item:a"), 2 } });

	const iggy::runtime::RuntimeGameplayState copy = original;

	Expect(copy.session.tickIndex == 7, "copied runtime gameplay state should preserve session");
	Expect(copy.commandQueue.frames.size() == 1 && SameFrame(copy.commandQueue.frames[0], original.commandQueue.frames[0]), "copied runtime gameplay state should preserve queue");
	Expect(copy.interaction.targets.targets().size() == 1 && copy.interaction.targets.targets()[0].id == Id("target:a"), "copied runtime gameplay state should preserve interaction state");
	Expect(copy.inventory.inventory.stacks.size() == 1 && copy.inventory.inventory.stacks[0].itemId == Id("item:a"), "copied runtime gameplay state should preserve inventory state");
}

void TestReassigningCopiedChildrenDoesNotMutateOriginal()
{
	iggy::runtime::RuntimeGameplayState original;
	original.session.tickIndex = 1;
	original.commandQueue.frames.push_back(WaitFrame(Id("actor:original")));
	original.interaction.targets = Targets({ Target("target:original") });
	original.inventory.inventory = Inventory({ { Id("item:original"), 1 } });

	iggy::runtime::RuntimeGameplayState copy = original;
	copy.session.tickIndex = 99;
	copy.commandQueue.frames = { WaitFrame(Id("actor:copy")) };
	copy.interaction.targets = Targets({ Target("target:copy") });
	copy.inventory.inventory = Inventory({ { Id("item:copy"), 5 } });

	Expect(copy.session.tickIndex == 99, "runtime gameplay state copy should accept reassigned session");
	Expect(copy.commandQueue.frames.size() == 1 && copy.commandQueue.frames[0].commands[0].actorId == Id("actor:copy"), "runtime gameplay state copy should accept reassigned queue");
	Expect(copy.interaction.targets.targets()[0].id == Id("target:copy"), "runtime gameplay state copy should accept reassigned interaction targets");
	Expect(copy.inventory.inventory.stacks[0].itemId == Id("item:copy"), "runtime gameplay state copy should accept reassigned inventory");
	Expect(original.session.tickIndex == 1, "reassigning copy should not mutate original session");
	Expect(original.commandQueue.frames[0].commands[0].actorId == Id("actor:original"), "reassigning copy should not mutate original queue");
	Expect(original.interaction.targets.targets()[0].id == Id("target:original"), "reassigning copy should not mutate original interaction state");
	Expect(original.inventory.inventory.stacks[0].itemId == Id("item:original"), "reassigning copy should not mutate original inventory state");
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

	const iggy::runtime::RuntimeGameplayState state {
		session,
		queue,
		interaction,
		inventory,
	};

	Expect(state.session.hasPlayer, "runtime gameplay state should preserve session values without validation");
	Expect(state.commandQueue.frames.size() == 1, "runtime gameplay state should preserve queue values without validation");
	Expect(state.interaction.targets.targets().size() == 1, "runtime gameplay state should preserve interaction target values without validation");
	Expect(state.inventory.inventory.stacks.size() == 1, "runtime gameplay state should preserve inventory values without validation");
	Expect(state.inventory.inventory.stacks[0].itemId.empty() && state.inventory.inventory.stacks[0].count == 0, "runtime gameplay state should preserve invalid-looking inventory stack");
	Expect(state.inventory.drops.drops.size() == 1 && state.inventory.drops.drops[0].id.empty(), "runtime gameplay state should preserve invalid-looking drop");
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
