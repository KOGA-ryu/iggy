#include <cstdlib>
#include <initializer_list>
#include <string>
#include <string_view>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimePlayerCommandPlanningStep.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId PlayerId { "player:one" };
const iggy::ResourceId OtherActorId { "player:two" };
const iggy::ResourceId TargetId { "target:lever" };
const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::PlayerAgentState Player(const char *id = "player:one")
{
	iggy::PlayerAgentState player;
	player.id = iggy::ResourceId { id };
	player.position = { 2.25F, 3.75F };
	player.spawnTile = { 2, 3 };
	player.movementStatus = iggy::PlayerMovementStatus::Moving;
	player.facing = iggy::PlayerFacing2D::East;
	return player;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = { iggy::test::MapFromRows({ "..", ".." }), {} };
	session.player = Player();
	session.hasPlayer = true;
	session.tickIndex = 5;
	session.renderCache.tileChunkConfig = { 2, 2, { { WalkableMaterial, BlockedMaterial }, 4 } };
	session.hasRenderCache = true;
	return session;
}

iggy::runtime::GameplayCommandFrame2D Frame(std::initializer_list<iggy::runtime::GameplayCommand2D> commands)
{
	iggy::runtime::GameplayCommandFrame2D frame;
	frame.commands.insert(frame.commands.end(), commands.begin(), commands.end());
	return frame;
}

void ExpectPlayer(const iggy::PlayerAgentState &actual, const iggy::PlayerAgentState &expected, std::string_view context)
{
	Expect(actual.id == expected.id, std::string(context) + " should preserve player id");
	Expect(NearVec(actual.position, expected.position), std::string(context) + " should preserve player position");
	Expect(actual.spawnTile == expected.spawnTile, std::string(context) + " should preserve player spawn tile");
	Expect(actual.movementStatus == expected.movementStatus, std::string(context) + " should preserve player movement status");
	Expect(actual.facing == expected.facing, std::string(context) + " should preserve player facing");
}

void TestMissingPlayerReturnsEmptyMissingPlayerResult()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.interact(PlayerId, {}),
	});

	const iggy::runtime::RuntimePlayerCommandPlanningResult result = iggy::runtime::RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::MissingPlayer, "missing player should return MissingPlayer status");
	Expect(!result.playerPlan.validation.hasInvalidCommands, "missing player should not validate command frame");
	Expect(result.playerPlan.validation.invalidCommands.empty(), "missing player should have no invalid diagnostics");
	Expect(result.playerPlan.plans.empty(), "missing player should have no plans");
	Expect(result.playerPlan.rejectedPlans.empty(), "missing player should have no rejected plans");
}

void TestPresentPlayerEmptyFramePlansEmptyResult()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame;

	const iggy::runtime::RuntimePlayerCommandPlanningResult result = iggy::runtime::RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "present player empty frame should return Planned status");
	Expect(!result.playerPlan.validation.hasInvalidCommands, "present player empty frame should have no invalid commands");
	Expect(result.playerPlan.validation.acceptedFrame.commands.empty(), "present player empty frame should have empty accepted frame");
	Expect(result.playerPlan.plans.empty(), "present player empty frame should have no plans");
	Expect(result.playerPlan.rejectedPlans.empty(), "present player empty frame should have no rejected plans");
}

void TestPresentPlayerValidCommandsPreservePlanOrder()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(PlayerId, { 1.5F, 2.5F }),
		factory.wait(PlayerId),
		factory.interact(PlayerId, TargetId),
	});

	const iggy::runtime::RuntimePlayerCommandPlanningResult result = iggy::runtime::RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "present player valid commands should return Planned status");
	Expect(result.playerPlan.plans.size() == 3, "present player valid commands should produce ordered plans");
	if (result.playerPlan.plans.size() == 3) {
		Expect(result.playerPlan.plans[0].type == iggy::PlayerCommandPlan2DType::MoveToPoint, "first valid command should produce first plan");
		Expect(result.playerPlan.plans[1].type == iggy::PlayerCommandPlan2DType::Wait, "second valid command should produce second plan");
		Expect(result.playerPlan.plans[2].type == iggy::PlayerCommandPlan2DType::Interact, "third valid command should produce third plan");
	}
	Expect(result.playerPlan.rejectedPlans.empty(), "matching valid commands should have no rejected plans");
}

void TestPresentPlayerInvalidCommandSurfacesValidationDiagnostics()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.wait(PlayerId),
		factory.interact(PlayerId, {}),
	});

	const iggy::runtime::RuntimePlayerCommandPlanningResult result = iggy::runtime::RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "present player invalid command should still return Planned status");
	Expect(result.playerPlan.validation.hasInvalidCommands, "present player invalid command should surface validation invalid flag");
	Expect(result.playerPlan.validation.invalidCommands.size() == 1, "present player invalid command should surface one invalid diagnostic");
	if (result.playerPlan.validation.invalidCommands.size() == 1) {
		Expect(result.playerPlan.validation.invalidCommands[0].index == 1, "invalid command diagnostic should preserve original index");
		Expect(result.playerPlan.validation.invalidCommands[0].status == iggy::runtime::GameplayCommand2DStatus::MissingTarget, "invalid command diagnostic should preserve MissingTarget status");
	}
	Expect(result.playerPlan.plans.size() == 1, "invalid command should not be planned");
	Expect(result.playerPlan.rejectedPlans.empty(), "invalid command should not become player-level rejected plan");
}

void TestPresentPlayerActorMismatchSurfacesRejectedPlan()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.wait(PlayerId),
		factory.moveToPoint(OtherActorId, { 8.0F, 9.0F }),
	});

	const iggy::runtime::RuntimePlayerCommandPlanningResult result = iggy::runtime::RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "present player actor mismatch should return Planned status");
	Expect(!result.playerPlan.validation.hasInvalidCommands, "actor mismatch should not be runtime validation failure");
	Expect(result.playerPlan.plans.size() == 2, "actor mismatch should still produce a rejected plan entry");
	Expect(result.playerPlan.rejectedPlans.size() == 1, "actor mismatch should surface one rejected plan diagnostic");
	if (result.playerPlan.rejectedPlans.size() == 1) {
		Expect(result.playerPlan.rejectedPlans[0].originalCommandIndex == 1, "rejected plan should preserve original command index");
		Expect(result.playerPlan.rejectedPlans[0].planIndex == 1, "rejected plan should preserve plan index");
		Expect(result.playerPlan.rejectedPlans[0].plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::ActorMismatch, "rejected plan should preserve actor mismatch reason");
	}
}

void TestInputSessionIsNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeSessionState original = session;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(PlayerId, { 1.0F, 1.0F }),
	});

	const iggy::runtime::RuntimePlayerCommandPlanningResult result = iggy::runtime::RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "session immutability setup should plan");
	Expect(session.hasPlayer == original.hasPlayer, "planning step should not mutate hasPlayer");
	ExpectPlayer(session.player, original.player, "planning step input session");
	Expect(session.tickIndex == original.tickIndex, "planning step should not mutate tickIndex");
	Expect(session.level.map.width == original.level.map.width && session.level.map.height == original.level.map.height, "planning step should not mutate level map shape");
	Expect(session.hasRenderCache == original.hasRenderCache, "planning step should not mutate hasRenderCache");
	Expect(session.renderCache.tileChunkConfig.chunkWidth == original.renderCache.tileChunkConfig.chunkWidth && session.renderCache.tileChunkConfig.tileCommands.layer == original.renderCache.tileChunkConfig.tileCommands.layer, "planning step should not mutate render cache config");
}

void TestInputFrameIsNotMutated()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.moveToPoint(PlayerId, { 3.0F, 4.0F }),
		factory.interact(PlayerId, {}),
	});
	const iggy::runtime::GameplayCommandFrame2D original = frame;

	const iggy::runtime::RuntimePlayerCommandPlanningResult result = iggy::runtime::RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "frame immutability setup should plan");
	Expect(frame.commands.size() == original.commands.size(), "planning step should not mutate frame command count");
	Expect(frame.commands[0].type == original.commands[0].type && NearVec(frame.commands[0].targetPoint, original.commands[0].targetPoint), "planning step should not mutate first command");
	Expect(frame.commands[1].type == original.commands[1].type && frame.commands[1].targetId == original.commands[1].targetId, "planning step should not mutate second command");
}

void TestPlanningDoesNotAdvanceTickOrUpdateLevel()
{
	const iggy::runtime::GameplayCommand2DFactory factory;
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.tickIndex = 42;
	const iggy::runtime::GameplayCommandFrame2D frame = Frame({
		factory.wait(PlayerId),
	});

	const iggy::runtime::RuntimePlayerCommandPlanningResult result = iggy::runtime::RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	Expect(result.status == iggy::runtime::RuntimePlayerCommandPlanningStatus::Planned, "non-tick planning setup should plan");
	Expect(session.tickIndex == 42, "planning step should not advance tickIndex");
	Expect(session.level.npcAgents.empty(), "planning step should not update NPC state");
	Expect(session.level.map.width == 2 && session.level.map.height == 2, "planning step should not update level map");
}

} // namespace

int main()
{
	TestMissingPlayerReturnsEmptyMissingPlayerResult();
	TestPresentPlayerEmptyFramePlansEmptyResult();
	TestPresentPlayerValidCommandsPreservePlanOrder();
	TestPresentPlayerInvalidCommandSurfacesValidationDiagnostics();
	TestPresentPlayerActorMismatchSurfacesRejectedPlan();
	TestInputSessionIsNotMutated();
	TestInputFrameIsNotMutated();
	TestPlanningDoesNotAdvanceTickOrUpdateLevel();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
