#include <cstdlib>

#include "core/resource/ResourceId.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerCommandPlanner2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId PlayerId { "player:one" };
const iggy::ResourceId OtherActorId { "player:two" };
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

void TestPlanningDoesNotMutatePlayerState()
{
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(PlayerId, { 5.0F, 6.0F });
	iggy::PlayerAgentState player = Player();
	const iggy::PlayerAgentState original = player;

	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(player, command);

	Expect(plan.type == iggy::PlayerCommandPlan2DType::MoveToPoint, "non-mutating setup should produce movement plan");
	Expect(player.id == original.id, "planning should not mutate player id");
	Expect(NearVec(player.position, original.position), "planning should not mutate player position");
	Expect(player.spawnTile == original.spawnTile, "planning should not mutate player spawn tile");
	Expect(player.movementStatus == original.movementStatus, "planning should not mutate player movement status");
	Expect(player.facing == original.facing, "planning should not mutate player facing");
}

void TestInvalidInteractRejects()
{
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, {}));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::Rejected, "invalid interact should reject");
	Expect(plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::InvalidCommand, "invalid interact should reject with InvalidCommand");
	Expect(plan.actorId == PlayerId, "invalid interact rejection should preserve resolved actor id");
	Expect(plan.targetId.empty() && plan.targetTile == iggy::TileCoord { -1, -1 }, "invalid interact rejection should leave unrelated targets defaulted");
}

void TestInvalidCommandWinsOverActorMismatch()
{
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.interact(OtherActorId, {}));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::Rejected, "invalid mismatched command should reject");
	Expect(plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::InvalidCommand, "invalid command should win over actor mismatch");
	Expect(plan.actorId == OtherActorId, "invalid mismatched rejection should preserve command actor id");
}

void TestActorMismatchOnlyWhenBothIdsAreNonEmptyAndDifferent()
{
	const iggy::runtime::GameplayCommand2D command = iggy::runtime::GameplayCommand2DFactory {}.wait(OtherActorId);

	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), command);

	Expect(plan.type == iggy::PlayerCommandPlan2DType::Rejected, "different non-empty actor ids should reject");
	Expect(plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::ActorMismatch, "different non-empty actor ids should reject with ActorMismatch");
	Expect(plan.actorId == OtherActorId, "actor mismatch rejection should preserve command actor id");
}

void TestEmptyCommandActorAppliesToPlayer()
{
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.wait());

	Expect(plan.type == iggy::PlayerCommandPlan2DType::Wait, "empty command actor should be allowed");
	Expect(plan.actorId == PlayerId, "empty command actor should resolve to player id");
	Expect(plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::None, "empty command actor should not reject");
}

void TestEmptyPlayerIdAcceptsCommandActor()
{
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player({}), iggy::runtime::GameplayCommand2DFactory {}.wait(OtherActorId));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::Wait, "empty player id should accept non-empty command actor");
	Expect(plan.actorId == OtherActorId, "empty player id should resolve actor from command");
	Expect(plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::None, "empty player id should not reject matching");
}

void TestNoneCommandYieldsNonePlan()
{
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.none(PlayerId));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::None, "None command should yield None plan");
	Expect(plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::None, "None command should not reject");
	Expect(plan.actorId == PlayerId, "None plan should preserve actor id");
}

void TestWaitCommandYieldsWaitPlan()
{
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::Wait, "Wait command should yield Wait plan");
	Expect(plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::None, "Wait command should not reject");
	Expect(plan.actorId == PlayerId, "Wait plan should preserve actor id");
}

void TestMoveToPointPreservesPointAndComputesTile()
{
	const iggy::Vec2 target { 3.75F, 4.1F };
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(PlayerId, target));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::MoveToPoint, "MoveToPoint command should yield MoveToPoint plan");
	Expect(NearVec(plan.targetPoint, target), "MoveToPoint plan should preserve target point");
	Expect(plan.targetTile == iggy::TileCoord { 3, 4 }, "MoveToPoint plan should compute target tile with floor semantics");
}

void TestMoveToPointNegativeCoordinatesUseFloorSemantics()
{
	const iggy::Vec2 target { -0.25F, -1.1F };
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.moveToPoint(PlayerId, target));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::MoveToPoint, "negative MoveToPoint should yield MoveToPoint plan");
	Expect(NearVec(plan.targetPoint, target), "negative MoveToPoint plan should preserve target point");
	Expect(plan.targetTile == iggy::TileCoord { -1, -2 }, "negative MoveToPoint plan should use tileForPoint floor semantics");
}

void TestMoveToTilePreservesTileAndComputesCenterPoint()
{
	const iggy::TileCoord target { 6, 8 };
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.moveToTile(PlayerId, target));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::MoveToPoint, "MoveToTile command should yield MoveToPoint plan");
	Expect(plan.targetTile == target, "MoveToTile plan should preserve target tile");
	Expect(NearVec(plan.targetPoint, { 6.5F, 8.5F }), "MoveToTile plan should compute target point with tileCenter");
}

void TestMoveToTileNegativeCoordinatesUseCenterSemantics()
{
	const iggy::TileCoord target { -2, -3 };
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.moveToTile(PlayerId, target));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::MoveToPoint, "negative MoveToTile should yield MoveToPoint plan");
	Expect(plan.targetTile == target, "negative MoveToTile plan should preserve target tile");
	Expect(NearVec(plan.targetPoint, { -1.5F, -2.5F }), "negative MoveToTile plan should use tileCenter semantics");
}

void TestInteractPreservesTargetId()
{
	const iggy::PlayerCommandPlan2D plan = iggy::PlayerCommandPlanner2D {}.plan(Player(), iggy::runtime::GameplayCommand2DFactory {}.interact(PlayerId, TargetId));

	Expect(plan.type == iggy::PlayerCommandPlan2DType::Interact, "Interact command should yield Interact plan");
	Expect(plan.rejectReason == iggy::PlayerCommandPlan2DRejectReason::None, "valid Interact command should not reject");
	Expect(plan.actorId == PlayerId, "Interact plan should preserve actor id");
	Expect(plan.targetId == TargetId, "Interact plan should preserve target id");
}

} // namespace

int main()
{
	TestPlanningDoesNotMutatePlayerState();
	TestInvalidInteractRejects();
	TestInvalidCommandWinsOverActorMismatch();
	TestActorMismatchOnlyWhenBothIdsAreNonEmptyAndDifferent();
	TestEmptyCommandActorAppliesToPlayer();
	TestEmptyPlayerIdAcceptsCommandActor();
	TestNoneCommandYieldsNonePlan();
	TestWaitCommandYieldsWaitPlan();
	TestMoveToPointPreservesPointAndComputesTile();
	TestMoveToPointNegativeCoordinatesUseFloorSemantics();
	TestMoveToTilePreservesTileAndComputesCenterPoint();
	TestMoveToTileNegativeCoordinatesUseCenterSemantics();
	TestInteractPreservesTargetId();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
