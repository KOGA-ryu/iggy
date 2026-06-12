#include <cstdlib>
#include <initializer_list>

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerCommandFramePlanner2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId PlayerId { "player:one" };
const iggy::ResourceId OtherActorId { "player:two" };
const iggy::ResourceId ThirdActorId { "player:three" };
const iggy::ResourceId TargetId { "target:lever" };

iggy::PlayerAgentState Player(iggy::ResourceId id = PlayerId)
{
	iggy::PlayerAgentState player;
	player.id = id;
	player.position = { 1.25F, 2.75F };
	player.spawnTile = { 1, 2 };
	player.movementStatus = iggy::PlayerMovementStatus::Idle;
	player.facing = iggy::PlayerFacing2D::South;
	return player;
}

iggy::runtime::GameplayCommandFrame2D Frame(std::initializer_list<iggy::runtime::GameplayCommand2D> commands)
{
	iggy::runtime::GameplayCommandFrame2D frame;
	frame.commands.insert(frame.commands.end(), commands.begin(), commands.end());
	return frame;
}

void TestEmptyFrameProducesEmptyResult()
{
	const iggy::runtime::GameplayCommandFrame2D frame;
	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(!result.validation.hasInvalidCommands, "empty frame should have no invalid commands");
	Expect(result.validation.acceptedFrame.commands.empty(), "empty frame should have empty accepted frame");
	Expect(result.validation.invalidCommands.empty(), "empty frame should have no invalid diagnostics");
	Expect(result.plans.empty(), "empty frame should produce no plans");
	Expect(result.rejectedPlans.empty(), "empty frame should produce no rejected plans");
}

void TestAllValidFrameProducesPlansInOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(PlayerId, { 1.5F, 2.5F }),
		factory.wait(PlayerId),
		factory.interact(PlayerId, TargetId),
	});

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(!result.validation.hasInvalidCommands, "all-valid frame should have no invalid commands");
	Expect(result.plans.size() == 3, "all-valid frame should produce one plan per command");
	if (result.plans.size() == 3) {
		Expect(result.plans[0].type == iggy::PlayerCommandPlan2DType::MoveToPoint, "first valid command should produce first plan");
		Expect(result.plans[1].type == iggy::PlayerCommandPlan2DType::Wait, "second valid command should produce second plan");
		Expect(result.plans[2].type == iggy::PlayerCommandPlan2DType::Interact, "third valid command should produce third plan");
	}
	Expect(result.rejectedPlans.empty(), "all-valid matching frame should have no rejected plans");
}

void TestFramePlannerPreservesSingleCommandPlannerBehavior()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(PlayerId, { 3.75F, 4.1F }),
		factory.moveToTile(PlayerId, { -2, -3 }),
		factory.wait(PlayerId),
		factory.interact(PlayerId, TargetId),
	});

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(result.plans.size() == 4, "frame planner should produce four plans for four valid commands");
	if (result.plans.size() == 4) {
		Expect(result.plans[0].type == iggy::PlayerCommandPlan2DType::MoveToPoint && NearVec(result.plans[0].targetPoint, { 3.75F, 4.1F }) && result.plans[0].targetTile == iggy::TileCoord { 3, 4 }, "frame planner should preserve MoveToPoint planning");
		Expect(result.plans[1].type == iggy::PlayerCommandPlan2DType::MoveToPoint && result.plans[1].targetTile == iggy::TileCoord { -2, -3 } && NearVec(result.plans[1].targetPoint, { -1.5F, -2.5F }), "frame planner should preserve MoveToTile planning");
		Expect(result.plans[2].type == iggy::PlayerCommandPlan2DType::Wait, "frame planner should preserve Wait planning");
		Expect(result.plans[3].type == iggy::PlayerCommandPlan2DType::Interact && result.plans[3].targetId == TargetId, "frame planner should preserve Interact planning");
	}
}

void TestInvalidInteractReportedByValidationAndNotPlanned()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.wait(PlayerId),
		factory.interact(PlayerId, {}),
		factory.moveToTile(PlayerId, { 2, 3 }),
	});

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(result.validation.hasInvalidCommands, "invalid command should appear in validation diagnostics");
	Expect(result.validation.invalidCommands.size() == 1, "one invalid interact should produce one validation diagnostic");
	if (result.validation.invalidCommands.size() == 1) {
		Expect(result.validation.invalidCommands[0].index == 1, "validation diagnostic should preserve invalid command original index");
		Expect(result.validation.invalidCommands[0].status == iggy::runtime::GameplayCommand2DStatus::MissingTarget, "validation diagnostic should preserve MissingTarget status");
	}
	Expect(result.plans.size() == 2, "invalid command should not be planned");
	if (result.plans.size() == 2) {
		Expect(result.plans[0].type == iggy::PlayerCommandPlan2DType::Wait, "valid command before invalid command should be planned");
		Expect(result.plans[1].type == iggy::PlayerCommandPlan2DType::MoveToPoint, "valid command after invalid command should be planned");
	}
	Expect(result.rejectedPlans.empty(), "invalid command should not appear in player-level rejected plans");
}

void TestMixedValidInvalidCommandsPlanAcceptedRelativeOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.interact(PlayerId, {}),
		factory.moveToPoint(PlayerId, { 5.0F, 6.0F }),
		factory.interact(PlayerId, TargetId),
		factory.interact(PlayerId, {}),
		factory.wait(PlayerId),
	});

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(result.validation.invalidCommands.size() == 2, "mixed frame should report invalid commands");
	Expect(result.plans.size() == 3, "mixed frame should plan only accepted commands");
	if (result.plans.size() == 3) {
		Expect(result.plans[0].type == iggy::PlayerCommandPlan2DType::MoveToPoint, "mixed frame should preserve first accepted plan");
		Expect(result.plans[1].type == iggy::PlayerCommandPlan2DType::Interact, "mixed frame should preserve second accepted plan");
		Expect(result.plans[2].type == iggy::PlayerCommandPlan2DType::Wait, "mixed frame should preserve third accepted plan");
	}
}

void TestActorMismatchedAcceptedCommandProducesRejectedPlan()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.wait(PlayerId),
		factory.moveToPoint(OtherActorId, { 7.0F, 8.0F }),
	});

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(!result.validation.hasInvalidCommands, "actor mismatch is a player-level rejection, not validation failure");
	Expect(result.plans.size() == 2, "actor-mismatched accepted command should still produce a plan");
	Expect(result.rejectedPlans.size() == 1, "actor-mismatched accepted command should be reported as rejected plan");
	if (result.rejectedPlans.size() == 1) {
		Expect(result.rejectedPlans[0].originalCommandIndex == 1, "rejected plan should preserve original command index");
		Expect(result.rejectedPlans[0].planIndex == 1, "rejected plan should preserve plan index");
		Expect(result.rejectedPlans[0].plan.type == iggy::PlayerCommandPlan2DType::Rejected && result.rejectedPlans[0].plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::ActorMismatch, "rejected plan should preserve actor mismatch plan");
	}
}

void TestInvalidActorMismatchedCommandOnlyAppearsInValidation()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.interact(OtherActorId, {}),
	});

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(result.validation.invalidCommands.size() == 1, "invalid actor-mismatched command should appear in validation diagnostics");
	Expect(result.validation.invalidCommands[0].index == 0, "invalid actor-mismatched validation diagnostic should preserve index");
	Expect(result.plans.empty(), "invalid actor-mismatched command should not be planned");
	Expect(result.rejectedPlans.empty(), "invalid actor-mismatched command should not appear in player-level rejections");
}

void TestMultipleRejectedAcceptedCommandsReportedInOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(OtherActorId, { 1.0F, 1.0F }),
		factory.wait(PlayerId),
		factory.interact(ThirdActorId, TargetId),
	});

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(result.plans.size() == 3, "multiple rejection setup should plan all accepted commands");
	Expect(result.rejectedPlans.size() == 2, "multiple actor mismatches should produce multiple rejected plan diagnostics");
	if (result.rejectedPlans.size() == 2) {
		Expect(result.rejectedPlans[0].originalCommandIndex == 0 && result.rejectedPlans[0].planIndex == 0, "first rejected plan should preserve input and plan order");
		Expect(result.rejectedPlans[1].originalCommandIndex == 2 && result.rejectedPlans[1].planIndex == 2, "second rejected plan should preserve input and plan order");
	}
}

void TestEmptyActorIdsRemainAcceptedThroughFramePlanner()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.wait(),
		factory.moveToPoint({}, { 2.0F, 3.0F }),
		factory.interact({}, TargetId),
	});

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(Player(), frame);

	Expect(!result.validation.hasInvalidCommands, "empty actor ids should not fail validation");
	Expect(result.plans.size() == 3, "empty actor id commands should be planned");
	Expect(result.rejectedPlans.empty(), "empty actor id commands should not be player-level rejected");
	if (result.plans.size() == 3)
		Expect(result.plans[0].actorId == PlayerId && result.plans[1].actorId == PlayerId && result.plans[2].actorId == PlayerId, "empty actor id plans should resolve to player id");
}

void TestInputsAreNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::PlayerAgentState player = Player();
	const iggy::PlayerAgentState originalPlayer = player;
	iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(PlayerId, { 9.0F, 10.0F }),
		factory.interact(PlayerId, {}),
	});
	const iggy::runtime::GameplayCommandFrame2D originalFrame = frame;

	const iggy::PlayerCommandFramePlan2DResult result = iggy::PlayerCommandFramePlanner2D {}.plan(player, frame);

	Expect(result.validation.hasInvalidCommands, "input immutability setup should include invalid command");
	Expect(player.id == originalPlayer.id && NearVec(player.position, originalPlayer.position), "frame planner should not mutate player state");
	Expect(player.spawnTile == originalPlayer.spawnTile && player.movementStatus == originalPlayer.movementStatus && player.facing == originalPlayer.facing, "frame planner should not mutate player status fields");
	Expect(frame.commands.size() == originalFrame.commands.size(), "frame planner should not mutate command frame count");
	Expect(frame.commands[0].type == originalFrame.commands[0].type && NearVec(frame.commands[0].targetPoint, originalFrame.commands[0].targetPoint), "frame planner should not mutate first command");
	Expect(frame.commands[1].type == originalFrame.commands[1].type && frame.commands[1].targetId == originalFrame.commands[1].targetId, "frame planner should not mutate second command");
}

} // namespace

int main()
{
	TestEmptyFrameProducesEmptyResult();
	TestAllValidFrameProducesPlansInOrder();
	TestFramePlannerPreservesSingleCommandPlannerBehavior();
	TestInvalidInteractReportedByValidationAndNotPlanned();
	TestMixedValidInvalidCommandsPlanAcceptedRelativeOrder();
	TestActorMismatchedAcceptedCommandProducesRejectedPlan();
	TestInvalidActorMismatchedCommandOnlyAppearsInValidation();
	TestMultipleRejectedAcceptedCommandsReportedInOrder();
	TestEmptyActorIdsRemainAcceptedThroughFramePlanner();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
