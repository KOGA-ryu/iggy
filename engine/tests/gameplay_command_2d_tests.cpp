#include <cstdlib>

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/level/LevelRuntimeState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId ActorId { "actor:player" };
const iggy::ResourceId TargetId { "target:lever" };

void TestDefaultCommandIsValidNone()
{
	const iggy::runtime::GameplayCommand2D command;

	Expect(command.type == iggy::runtime::GameplayCommand2DType::None, "default gameplay command should be None");
	Expect(command.actorId.empty(), "default gameplay command should have empty actor id");
	Expect(NearVec(command.targetPoint, { 0.0F, 0.0F }), "default gameplay command should have default target point");
	Expect(command.targetTile == iggy::TileCoord { -1, -1 }, "default gameplay command should have default target tile");
	Expect(command.targetId.empty(), "default gameplay command should have empty target id");
	Expect(iggy::runtime::validate(command) == iggy::runtime::GameplayCommand2DStatus::Valid, "default gameplay command should validate");
	Expect(iggy::runtime::valid(command), "default gameplay command should be valid");
}

void TestNonePreservesActorId()
{
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.none(ActorId);

	Expect(command.type == iggy::runtime::GameplayCommand2DType::None, "none command should have None type");
	Expect(command.actorId == ActorId, "none command should preserve actor id");
	Expect(command.targetId.empty() && command.targetTile == iggy::TileCoord { -1, -1 }, "none command should leave target data defaulted");
	Expect(iggy::runtime::valid(command), "none command should validate");
}

void TestMoveToPointPreservesActorAndTargetPoint()
{
	const iggy::Vec2 target { -3.5F, 9.25F };
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(ActorId, target);

	Expect(command.type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "move-to-point command should have MoveToPoint type");
	Expect(command.actorId == ActorId, "move-to-point command should preserve actor id");
	Expect(NearVec(command.targetPoint, target), "move-to-point command should preserve target point");
	Expect(command.targetId.empty() && command.targetTile == iggy::TileCoord { -1, -1 }, "move-to-point command should leave irrelevant target fields defaulted");
	Expect(iggy::runtime::valid(command), "move-to-point command should validate");
}

void TestMoveToTilePreservesActorAndTargetTile()
{
	const iggy::TileCoord target { -4, 7 };
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.moveToTile(ActorId, target);

	Expect(command.type == iggy::runtime::GameplayCommand2DType::MoveToTile, "move-to-tile command should have MoveToTile type");
	Expect(command.actorId == ActorId, "move-to-tile command should preserve actor id");
	Expect(command.targetTile == target, "move-to-tile command should preserve target tile including negative coordinates");
	Expect(NearVec(command.targetPoint, { 0.0F, 0.0F }) && command.targetId.empty(), "move-to-tile command should leave irrelevant target fields defaulted");
	Expect(iggy::runtime::valid(command), "move-to-tile command should validate");
}

void TestInteractRequiresTargetId()
{
	const iggy::runtime::GameplayCommand2D validCommand = iggy::runtime::GameplayCommand2DFactory {}.interact(ActorId, TargetId);
	const iggy::runtime::GameplayCommand2D invalidCommand = iggy::runtime::GameplayCommand2DFactory {}.interact(ActorId, {});

	Expect(validCommand.type == iggy::runtime::GameplayCommand2DType::Interact, "interact command should have Interact type");
	Expect(validCommand.actorId == ActorId && validCommand.targetId == TargetId, "interact command should preserve actor and target ids");
	Expect(iggy::runtime::validate(validCommand) == iggy::runtime::GameplayCommand2DStatus::Valid, "interact command with target should validate");
	Expect(iggy::runtime::valid(validCommand), "interact command with target should be valid");
	Expect(iggy::runtime::validate(invalidCommand) == iggy::runtime::GameplayCommand2DStatus::MissingTarget, "interact command without target should report missing target");
	Expect(!iggy::runtime::valid(invalidCommand), "interact command without target should not be valid");
}

void TestWaitPreservesActorId()
{
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.wait(ActorId);

	Expect(command.type == iggy::runtime::GameplayCommand2DType::Wait, "wait command should have Wait type");
	Expect(command.actorId == ActorId, "wait command should preserve actor id");
	Expect(command.targetId.empty() && command.targetTile == iggy::TileCoord { -1, -1 }, "wait command should leave target data defaulted");
	Expect(iggy::runtime::valid(command), "wait command should validate");
}

void TestEmptyActorIdAllowedForEveryCommandType()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommand2D none = factory.none();
	const iggy::runtime::GameplayCommand2D movePoint = factory.moveToPoint({}, { 1.0F, 2.0F });
	const iggy::runtime::GameplayCommand2D moveTile = factory.moveToTile({}, { -2, -3 });
	const iggy::runtime::GameplayCommand2D interact = factory.interact({}, TargetId);
	const iggy::runtime::GameplayCommand2D wait = factory.wait();

	Expect(none.actorId.empty() && iggy::runtime::valid(none), "none command should allow empty actor id");
	Expect(movePoint.actorId.empty() && iggy::runtime::valid(movePoint), "move-to-point command should allow empty actor id");
	Expect(moveTile.actorId.empty() && iggy::runtime::valid(moveTile), "move-to-tile command should allow empty actor id");
	Expect(interact.actorId.empty() && iggy::runtime::valid(interact), "interact command should allow empty actor id when target exists");
	Expect(wait.actorId.empty() && iggy::runtime::valid(wait), "wait command should allow empty actor id");
}

void TestCommandFramePreservesOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::GameplayCommandFrame2D frame;
	frame.commands.push_back(factory.moveToPoint(ActorId, { 1.0F, 1.0F }));
	frame.commands.push_back(factory.wait(ActorId));
	frame.commands.push_back(factory.interact(ActorId, TargetId));

	Expect(frame.commands.size() == 3, "gameplay command frame should preserve command count");
	Expect(frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "gameplay command frame should preserve first command");
	Expect(frame.commands[1].type == iggy::runtime::GameplayCommand2DType::Wait, "gameplay command frame should preserve second command");
	Expect(frame.commands[2].type == iggy::runtime::GameplayCommand2DType::Interact, "gameplay command frame should preserve third command");
}

void TestEmptyCommandFrameIsValidData()
{
	const iggy::runtime::GameplayCommandFrame2D frame;

	Expect(frame.commands.empty(), "empty gameplay command frame should be valid data");
}

void TestCommandsDoNotMutateRuntimeLevelState()
{
	iggy::LevelRuntimeState state;
	state.map.id = iggy::ResourceId { "level:test" };
	state.map.width = 2;
	state.map.height = 1;

	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommand2D command = factory.moveToPoint(ActorId, { 5.0F, 6.0F });

	Expect(iggy::runtime::valid(command), "state non-mutation setup command should validate");
	Expect(state.map.id == iggy::ResourceId { "level:test" }, "constructing commands should not mutate level map id");
	Expect(state.map.width == 2 && state.map.height == 1, "constructing commands should not mutate level map dimensions");
	Expect(state.npcAgents.empty(), "constructing commands should not mutate level NPC list");
}

} // namespace

int main()
{
	TestDefaultCommandIsValidNone();
	TestNonePreservesActorId();
	TestMoveToPointPreservesActorAndTargetPoint();
	TestMoveToTilePreservesActorAndTargetTile();
	TestInteractRequiresTargetId();
	TestWaitPreservesActorId();
	TestEmptyActorIdAllowedForEveryCommandType();
	TestCommandFramePreservesOrder();
	TestEmptyCommandFrameIsValidData();
	TestCommandsDoNotMutateRuntimeLevelState();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
