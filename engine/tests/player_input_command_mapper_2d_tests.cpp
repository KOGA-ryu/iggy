#include <cstdlib>

#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerInputCommandMapper2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId ActorId { "player:mapper" };
const iggy::ResourceId TargetId { "target:door" };

void ExpectDefaultCommand(const iggy::runtime::GameplayCommand2D &command, const char *message)
{
	Expect(command.type == iggy::runtime::GameplayCommand2DType::None, message);
	Expect(command.actorId.empty(), message);
	Expect(NearVec(command.targetPoint, { 0.0F, 0.0F }), message);
	Expect(command.targetTile == iggy::TileCoord { -1, -1 }, message);
	Expect(command.targetId.empty(), message);
}

void TestNoneIntentMapsToNoneCommandWithActorId()
{
	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map(ActorId, {});

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::Mapped, "none intent should map");
	Expect(result.intentStatus == iggy::PlayerInputIntent2DStatus::Valid, "none intent should preserve valid intent status");
	Expect(result.command.type == iggy::runtime::GameplayCommand2DType::None, "none intent should map to None command");
	Expect(result.command.actorId == ActorId, "none intent should preserve actor id");
	Expect(iggy::runtime::valid(result.command), "mapped none command should validate");
}

void TestMoveToPointMapsTargetPointExactly()
{
	const iggy::Vec2 target { -3.5F, 9.25F };
	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map(ActorId, iggy::playerMoveToPointIntent(target));

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::Mapped, "move-to-point intent should map");
	Expect(result.command.type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "move-to-point intent should map to MoveToPoint command");
	Expect(result.command.actorId == ActorId, "move-to-point intent should preserve actor id");
	Expect(NearVec(result.command.targetPoint, target), "move-to-point intent should preserve target point");
	Expect(iggy::runtime::valid(result.command), "mapped move-to-point command should validate");
}

void TestMoveToTileMapsTileExactlyIncludingNegativeCoords()
{
	const iggy::TileCoord tile { -4, 7 };
	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map(ActorId, iggy::playerMoveToTileIntent(tile));

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::Mapped, "move-to-tile intent should map");
	Expect(result.command.type == iggy::runtime::GameplayCommand2DType::MoveToTile, "move-to-tile intent should map to MoveToTile command");
	Expect(result.command.actorId == ActorId, "move-to-tile intent should preserve actor id");
	Expect(result.command.targetTile == tile, "move-to-tile intent should preserve tile including negative coordinates");
	Expect(iggy::runtime::valid(result.command), "mapped move-to-tile command should validate");
}

void TestInteractMapsTargetIdExactly()
{
	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map(ActorId, iggy::playerInteractIntent(TargetId));

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::Mapped, "interact intent should map");
	Expect(result.command.type == iggy::runtime::GameplayCommand2DType::Interact, "interact intent should map to Interact command");
	Expect(result.command.actorId == ActorId, "interact intent should preserve actor id");
	Expect(result.command.targetId == TargetId, "interact intent should preserve target id");
	Expect(iggy::runtime::valid(result.command), "mapped interact command should validate");
}

void TestWaitMapsToWaitCommand()
{
	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map(ActorId, iggy::playerWaitIntent());

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::Mapped, "wait intent should map");
	Expect(result.command.type == iggy::runtime::GameplayCommand2DType::Wait, "wait intent should map to Wait command");
	Expect(result.command.actorId == ActorId, "wait intent should preserve actor id");
	Expect(iggy::runtime::valid(result.command), "mapped wait command should validate");
}

void TestInvalidTargetIntentsReturnInvalidIntentAndDefaultCommand()
{
	const iggy::PlayerInputCommandMapper2DResult invalidInteract = iggy::PlayerInputCommandMapper2D {}.map(ActorId, iggy::playerInteractIntent({}));
	const iggy::PlayerInputCommandMapper2DResult invalidInspect = iggy::PlayerInputCommandMapper2D {}.map(ActorId, iggy::playerInspectIntent({}));

	Expect(invalidInteract.status == iggy::PlayerInputCommandMapper2DStatus::InvalidIntent, "invalid interact intent should return InvalidIntent");
	Expect(invalidInteract.intentStatus == iggy::PlayerInputIntent2DStatus::MissingTarget, "invalid interact intent should preserve MissingTarget");
	ExpectDefaultCommand(invalidInteract.command, "invalid interact intent should return default command");
	Expect(invalidInspect.status == iggy::PlayerInputCommandMapper2DStatus::InvalidIntent, "invalid inspect intent should return InvalidIntent");
	Expect(invalidInspect.intentStatus == iggy::PlayerInputIntent2DStatus::MissingTarget, "invalid inspect intent should preserve MissingTarget");
	ExpectDefaultCommand(invalidInspect.command, "invalid inspect intent should return default command");
}

void TestInspectWithTargetReturnsUnsupportedIntent()
{
	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map(ActorId, iggy::playerInspectIntent(TargetId));

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "inspect intent with target should be unsupported without command type");
	Expect(result.intentStatus == iggy::PlayerInputIntent2DStatus::Valid, "unsupported inspect should preserve valid intent status");
	ExpectDefaultCommand(result.command, "unsupported inspect should return default command");
}

void TestCancelReturnsUnsupportedIntent()
{
	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map(ActorId, iggy::playerCancelIntent());

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "cancel intent should be unsupported without command type");
	Expect(result.intentStatus == iggy::PlayerInputIntent2DStatus::Valid, "unsupported cancel should preserve valid intent status");
	ExpectDefaultCommand(result.command, "unsupported cancel should return default command");
}

void TestEmptyActorIdIsPreserved()
{
	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map({}, iggy::playerMoveToPointIntent({ 1.0F, 2.0F }));

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::Mapped, "empty actor id intent should still map");
	Expect(result.command.actorId.empty(), "empty actor id should be preserved in mapped command");
	Expect(iggy::runtime::valid(result.command), "mapped command with empty actor id should validate");
}

void TestInputsAreNotMutated()
{
	iggy::ResourceId actorId = ActorId;
	const iggy::ResourceId actorIdBefore = actorId;
	iggy::PlayerInputIntent2D intent = iggy::playerMoveToPointIntent({ 5.0F, 6.0F });
	const iggy::PlayerInputIntent2D intentBefore = intent;

	const iggy::PlayerInputCommandMapper2DResult result = iggy::PlayerInputCommandMapper2D {}.map(actorId, intent);

	Expect(result.status == iggy::PlayerInputCommandMapper2DStatus::Mapped, "mapper immutability setup should map");
	Expect(actorId == actorIdBefore, "mapper should not mutate actor id");
	Expect(intent.type == intentBefore.type, "mapper should not mutate intent type");
	Expect(NearVec(intent.worldPoint, intentBefore.worldPoint), "mapper should not mutate intent world point");
	Expect(intent.tile == intentBefore.tile, "mapper should not mutate intent tile");
	Expect(intent.targetId == intentBefore.targetId, "mapper should not mutate intent target id");
}

} // namespace

int main()
{
	TestNoneIntentMapsToNoneCommandWithActorId();
	TestMoveToPointMapsTargetPointExactly();
	TestMoveToTileMapsTileExactlyIncludingNegativeCoords();
	TestInteractMapsTargetIdExactly();
	TestWaitMapsToWaitCommand();
	TestInvalidTargetIntentsReturnInvalidIntentAndDefaultCommand();
	TestInspectWithTargetReturnsUnsupportedIntent();
	TestCancelReturnsUnsupportedIntent();
	TestEmptyActorIdIsPreserved();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
