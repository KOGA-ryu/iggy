#include <cstdlib>
#include <vector>

#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerInputCommandFrameMapper2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId ActorId { "player:frame-mapper" };
const iggy::ResourceId TargetId { "target:lever" };

void ExpectIntentEquals(const iggy::PlayerInputIntent2D &actual, const iggy::PlayerInputIntent2D &expected, const char *message)
{
	Expect(actual.type == expected.type, message);
	Expect(NearVec(actual.worldPoint, expected.worldPoint), message);
	Expect(actual.tile == expected.tile, message);
	Expect(actual.targetId == expected.targetId, message);
}

void TestEmptyIntentsProduceEmptyFrameWithNoIssues()
{
	const iggy::PlayerInputCommandFrameMapper2DResult result = iggy::PlayerInputCommandFrameMapper2D {}.map(ActorId, {});

	Expect(result.frame.commands.empty(), "empty intents should produce empty frame");
	Expect(result.issues.empty(), "empty intents should produce no issues");
	Expect(!result.hasIssues(), "empty intents should report no issues");
}

void TestSupportedIntentsMapInOrderWithExactPayloads()
{
	const iggy::Vec2 point { -2.5F, 8.75F };
	const iggy::TileCoord tile { -3, 11 };
	const std::vector<iggy::PlayerInputIntent2D> intents {
		{},
		iggy::playerMoveToPointIntent(point),
		iggy::playerMoveToTileIntent(tile),
		iggy::playerInteractIntent(TargetId),
		iggy::playerWaitIntent(),
	};

	const iggy::PlayerInputCommandFrameMapper2DResult result = iggy::PlayerInputCommandFrameMapper2D {}.map(ActorId, intents);

	Expect(result.frame.commands.size() == 5, "supported intents should all be appended");
	Expect(result.issues.empty(), "supported intents should produce no issues");
	Expect(!result.hasIssues(), "supported intents should report no issues");

	Expect(result.frame.commands[0].type == iggy::runtime::GameplayCommand2DType::None, "first command should preserve None order");
	Expect(result.frame.commands[0].actorId == ActorId, "none command should preserve actor id");
	Expect(iggy::runtime::valid(result.frame.commands[0]), "none command should validate");

	Expect(result.frame.commands[1].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "second command should be MoveToPoint");
	Expect(result.frame.commands[1].actorId == ActorId, "move-to-point command should preserve actor id");
	Expect(NearVec(result.frame.commands[1].targetPoint, point), "move-to-point command should preserve target point");
	Expect(iggy::runtime::valid(result.frame.commands[1]), "move-to-point command should validate");

	Expect(result.frame.commands[2].type == iggy::runtime::GameplayCommand2DType::MoveToTile, "third command should be MoveToTile");
	Expect(result.frame.commands[2].actorId == ActorId, "move-to-tile command should preserve actor id");
	Expect(result.frame.commands[2].targetTile == tile, "move-to-tile command should preserve negative tile coordinates");
	Expect(iggy::runtime::valid(result.frame.commands[2]), "move-to-tile command should validate");

	Expect(result.frame.commands[3].type == iggy::runtime::GameplayCommand2DType::Interact, "fourth command should be Interact");
	Expect(result.frame.commands[3].actorId == ActorId, "interact command should preserve actor id");
	Expect(result.frame.commands[3].targetId == TargetId, "interact command should preserve target id");
	Expect(iggy::runtime::valid(result.frame.commands[3]), "interact command should validate");

	Expect(result.frame.commands[4].type == iggy::runtime::GameplayCommand2DType::Wait, "fifth command should be Wait");
	Expect(result.frame.commands[4].actorId == ActorId, "wait command should preserve actor id");
	Expect(iggy::runtime::valid(result.frame.commands[4]), "wait command should validate");
}

void TestInvalidAndUnsupportedIntentsAreExcludedAndReported()
{
	const iggy::PlayerInputIntent2D validMove = iggy::playerMoveToPointIntent({ 1.0F, 2.0F });
	const iggy::PlayerInputIntent2D invalidInteract = iggy::playerInteractIntent({});
	const iggy::PlayerInputIntent2D unsupportedInspect = iggy::playerInspectIntent(TargetId);
	const iggy::PlayerInputIntent2D unsupportedCancel = iggy::playerCancelIntent();
	const iggy::PlayerInputIntent2D validWait = iggy::playerWaitIntent();
	const std::vector<iggy::PlayerInputIntent2D> intents {
		validMove,
		invalidInteract,
		unsupportedInspect,
		unsupportedCancel,
		validWait,
	};

	const iggy::PlayerInputCommandFrameMapper2DResult result = iggy::PlayerInputCommandFrameMapper2D {}.map(ActorId, intents);

	Expect(result.frame.commands.size() == 2, "only mapped intents should be appended to frame");
	Expect(result.frame.commands[0].type == iggy::runtime::GameplayCommand2DType::MoveToPoint, "first accepted command should stay first");
	Expect(NearVec(result.frame.commands[0].targetPoint, validMove.worldPoint), "accepted move payload should be preserved");
	Expect(result.frame.commands[1].type == iggy::runtime::GameplayCommand2DType::Wait, "later accepted command should preserve relative order");
	Expect(result.issues.size() == 3, "invalid and unsupported intents should be reported as issues");
	Expect(result.hasIssues(), "mixed batch should report issues");

	Expect(result.issues[0].intentIndex == 1, "invalid interact should report original index");
	Expect(result.issues[0].map.status == iggy::PlayerInputCommandMapper2DStatus::InvalidIntent, "invalid interact should preserve mapper status");
	Expect(result.issues[0].map.intentStatus == iggy::PlayerInputIntent2DStatus::MissingTarget, "invalid interact should preserve intent status");
	ExpectIntentEquals(result.issues[0].intent, invalidInteract, "invalid interact issue should preserve original intent");

	Expect(result.issues[1].intentIndex == 2, "unsupported inspect should report original index");
	Expect(result.issues[1].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "unsupported inspect should preserve mapper status");
	Expect(result.issues[1].map.intentStatus == iggy::PlayerInputIntent2DStatus::Valid, "unsupported inspect should preserve valid intent status");
	ExpectIntentEquals(result.issues[1].intent, unsupportedInspect, "unsupported inspect issue should preserve original intent");

	Expect(result.issues[2].intentIndex == 3, "unsupported cancel should report original index");
	Expect(result.issues[2].map.status == iggy::PlayerInputCommandMapper2DStatus::UnsupportedIntent, "unsupported cancel should preserve mapper status");
	Expect(result.issues[2].map.intentStatus == iggy::PlayerInputIntent2DStatus::Valid, "unsupported cancel should preserve valid intent status");
	ExpectIntentEquals(result.issues[2].intent, unsupportedCancel, "unsupported cancel issue should preserve original intent");
}

void TestEmptyActorIdIsPreservedAcrossMappedCommands()
{
	const std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent({ 3.0F, 4.0F }),
		iggy::playerWaitIntent(),
	};

	const iggy::PlayerInputCommandFrameMapper2DResult result = iggy::PlayerInputCommandFrameMapper2D {}.map({}, intents);

	Expect(result.frame.commands.size() == 2, "empty actor id batch should map supported intents");
	Expect(result.frame.commands[0].actorId.empty(), "first mapped command should preserve empty actor id");
	Expect(result.frame.commands[1].actorId.empty(), "second mapped command should preserve empty actor id");
	Expect(iggy::runtime::valid(result.frame.commands[0]), "mapped command with empty actor id should validate");
	Expect(iggy::runtime::valid(result.frame.commands[1]), "mapped command with empty actor id should validate");
}

void TestInputsAreNotMutated()
{
	iggy::ResourceId actorId = ActorId;
	const iggy::ResourceId actorIdBefore = actorId;
	std::vector<iggy::PlayerInputIntent2D> intents {
		iggy::playerMoveToPointIntent({ 5.0F, 6.0F }),
		iggy::playerInspectIntent(TargetId),
		iggy::playerWaitIntent(),
	};
	const std::vector<iggy::PlayerInputIntent2D> intentsBefore = intents;

	const iggy::PlayerInputCommandFrameMapper2DResult result = iggy::PlayerInputCommandFrameMapper2D {}.map(actorId, intents);

	Expect(result.frame.commands.size() == 2, "immutability setup should map supported intents");
	Expect(result.issues.size() == 1, "immutability setup should report unsupported intent");
	Expect(actorId == actorIdBefore, "frame mapper should not mutate actor id");
	Expect(intents.size() == intentsBefore.size(), "frame mapper should not mutate input vector size");
	for (std::size_t index = 0; index < intents.size(); ++index) {
		ExpectIntentEquals(intents[index], intentsBefore[index], "frame mapper should not mutate input intents");
	}
}

} // namespace

int main()
{
	TestEmptyIntentsProduceEmptyFrameWithNoIssues();
	TestSupportedIntentsMapInOrderWithExactPayloads();
	TestInvalidAndUnsupportedIntentsAreExcludedAndReported();
	TestEmptyActorIdIsPreservedAcrossMappedCommands();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
