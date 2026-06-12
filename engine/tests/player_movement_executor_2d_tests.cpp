#include <cstdlib>
#include <vector>

#include "scene/player/PlayerMovementExecutor2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:one" };

iggy::PlayerCommandPlan2D MovePlan(iggy::Vec2 target)
{
	iggy::PlayerCommandPlan2D plan;
	plan.type = iggy::PlayerCommandPlan2DType::MoveToPoint;
	plan.actorId = PlayerId;
	plan.targetPoint = target;
	return plan;
}

iggy::physics2d::CollisionObject2D Object(
	iggy::ResourceId id,
	iggy::Aabb2 bounds,
	bool solid = true)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), solid };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(result.built, "test fixture world should build");
	return result.world;
}

void ExpectObject(const iggy::physics2d::CollisionObject2D &actual, const iggy::physics2d::CollisionObject2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.shape.type == expected.shape.type, message);
	ExpectBounds(actual.shape.bounds, expected.shape.bounds, message);
	Expect(actual.solid == expected.solid, message);
}

void ExpectWorldUnchanged(
	const iggy::physics2d::CollisionWorld2D &world,
	const std::vector<iggy::physics2d::CollisionObject2D> &before,
	const char *message)
{
	Expect(world.objects().size() == before.size(), message);
	for (std::size_t index = 0; index < before.size(); ++index)
		ExpectObject(world.objects()[index], before[index], message);
}

void TestEmptyWorldMoveTowardFarTargetUsesMaxStep()
{
	const iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::North);
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 2.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		player,
		MovePlan({ 6.0F, 0.0F }),
		{},
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Moved, "far target in empty world should move");
	Expect(NearVec(result.requestedDelta, { 2.0F, 0.0F }), "far target should request maxStep delta");
	Expect(NearVec(result.movement.allowedDelta, { 2.0F, 0.0F }), "far target should allow maxStep delta");
	Expect(NearVec(result.state.position, { 2.0F, 0.0F }), "far target should update returned player position");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Moving, "far target should leave player moving");
	Expect(result.state.facing == iggy::PlayerFacing2D::East, "far target should update facing from requested delta");
	Expect(!result.reachedTarget, "far target should not be reached after capped movement");
	Expect(!result.movement.motion.blocked, "far target in empty world should embed unblocked motion");
}

void TestMoveReachesTargetWithinMaxStep()
{
	const iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 1.0F, 1.0F }, { 1, 1 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West);
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 3.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		player,
		MovePlan({ 3.0F, 1.0F }),
		{},
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Moved, "target inside maxStep should still report moved");
	Expect(result.reachedTarget, "target inside maxStep should be reached");
	Expect(NearVec(result.state.position, { 3.0F, 1.0F }), "target inside maxStep should place player at target");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Idle, "reached target should set movement status idle");
	Expect(result.state.facing == iggy::PlayerFacing2D::East, "reached target should update facing from movement");
}

void TestBlockedMovementPreservesHitMetadata()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 2.5F, -0.5F }, { 3.5F, 0.5F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 4.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::North),
		MovePlan({ 8.0F, 0.0F }),
		world,
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Blocked, "blocked movement should report Blocked");
	Expect(NearVec(result.movement.allowedDelta, { 2.0F, 0.0F }), "blocked movement should use CharacterMove2D allowed delta");
	Expect(NearVec(result.state.position, { 2.0F, 0.0F }), "blocked movement should not pass through obstacle");
	Expect(result.movement.motion.blocked, "blocked movement should preserve blocked motion result");
	Expect(result.movement.motion.hit.objectIndex == 0, "blocked movement should preserve hit object index");
	ExpectObject(result.movement.motion.hit.object, wall, "blocked movement should preserve hit object metadata");
	ExpectBounds(result.movement.motion.hit.objectBounds, wall.shape.bounds, "blocked movement should preserve hit bounds");
}

void TestPartialBlockedMovementKeepsMovingStatus()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 4.5F, -0.5F }, { 5.5F, 0.5F } }),
	});
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 8.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::North),
		MovePlan({ 8.0F, 0.0F }),
		world,
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Blocked, "partial blocked movement should report Blocked");
	Expect(NearVec(result.movement.allowedDelta, { 4.0F, 0.0F }), "partial blocked movement should preserve nonzero allowed delta");
	Expect(NearVec(result.state.position, { 4.0F, 0.0F }), "partial blocked movement should update by allowed delta");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Moving, "partial blocked movement should keep moving status");
	Expect(result.state.facing == iggy::PlayerFacing2D::East, "partial blocked movement should face requested direction");
}

void TestStartingOverlapBlocksWithoutMoving()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { -0.25F, -0.25F }, { 0.25F, 0.25F } }),
	});
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 2.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West),
		MovePlan({ 2.0F, 0.0F }),
		world,
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Blocked, "starting overlap should block");
	Expect(NearVec(result.movement.allowedDelta, { 0.0F, 0.0F }), "starting overlap should allow zero movement");
	Expect(NearVec(result.state.position, { 0.0F, 0.0F }), "starting overlap should leave position unchanged");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Idle, "starting overlap with zero allowed delta should set idle");
	Expect(result.state.facing == iggy::PlayerFacing2D::East, "starting overlap should still face requested direction");
}

void TestStartingEdgeTouchBlocksWithoutMoving()
{
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 0.5F, -0.5F }, { 1.5F, 0.5F } }),
	});
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 2.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West),
		MovePlan({ 2.0F, 0.0F }),
		world,
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Blocked, "starting edge-touch should block");
	Expect(NearVec(result.movement.allowedDelta, { 0.0F, 0.0F }), "starting edge-touch should allow zero movement");
	Expect(NearVec(result.state.position, { 0.0F, 0.0F }), "starting edge-touch should leave position unchanged");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Idle, "starting edge-touch with zero allowed delta should set idle");
}

void TestAlreadyAtTargetDoesNotMove()
{
	const iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 4.0F, 5.0F }, { 4, 5 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::South);

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		player,
		MovePlan(player.position),
		{},
		{});

	Expect(result.status == iggy::PlayerMovementExecutionStatus::NoMovement, "already at target should not move");
	Expect(result.reachedTarget, "already at target should report reached target");
	Expect(NearVec(result.state.position, player.position), "already at target should preserve position");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Idle, "already at target should set idle");
	Expect(result.state.facing == player.facing, "already at target should preserve facing");
}

void TestNonPositiveMaxStepDoesNotMove()
{
	const iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::North);
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 0.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		player,
		MovePlan({ 2.0F, 0.0F }),
		{},
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::NoMovement, "non-positive maxStep should not move");
	Expect(!result.reachedTarget, "non-positive maxStep should not report reached target when target differs");
	Expect(NearVec(result.state.position, player.position), "non-positive maxStep should preserve position");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Idle, "non-positive maxStep should set idle");
	Expect(result.state.facing == player.facing, "non-positive maxStep should preserve facing");
}

void TestNoneAndWaitPlansDoNotMove()
{
	iggy::PlayerCommandPlan2D none;
	none.type = iggy::PlayerCommandPlan2DType::None;
	iggy::PlayerCommandPlan2D wait;
	wait.type = iggy::PlayerCommandPlan2DType::Wait;
	const iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 1.0F, 2.0F }, { 1, 2 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West);

	const iggy::PlayerMovementExecutionResult noneResult = iggy::PlayerMovementExecutor2D {}.execute(player, none, {}, {});
	const iggy::PlayerMovementExecutionResult waitResult = iggy::PlayerMovementExecutor2D {}.execute(player, wait, {}, {});

	Expect(noneResult.status == iggy::PlayerMovementExecutionStatus::NoMovement, "None plan should not move");
	Expect(waitResult.status == iggy::PlayerMovementExecutionStatus::NoMovement, "Wait plan should not move");
	Expect(NearVec(noneResult.state.position, player.position), "None plan should preserve position");
	Expect(NearVec(waitResult.state.position, player.position), "Wait plan should preserve position");
	Expect(noneResult.state.movementStatus == iggy::PlayerMovementStatus::Idle, "None plan should set idle");
	Expect(waitResult.state.movementStatus == iggy::PlayerMovementStatus::Idle, "Wait plan should set idle");
	Expect(noneResult.state.facing == player.facing && waitResult.state.facing == player.facing, "None and Wait should preserve facing");
}

void TestRejectedPlanDoesNotMove()
{
	iggy::PlayerCommandPlan2D plan;
	plan.type = iggy::PlayerCommandPlan2DType::Rejected;
	plan.rejectReason = iggy::PlayerCommandPlan2DRejectReason::InvalidCommand;
	const iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 1.0F, 2.0F }, { 1, 2 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::North);

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(player, plan, {}, {});

	Expect(result.status == iggy::PlayerMovementExecutionStatus::RejectedPlan, "Rejected plan should report RejectedPlan");
	Expect(NearVec(result.state.position, player.position), "Rejected plan should preserve position");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Idle, "Rejected plan should set idle");
	Expect(result.state.facing == player.facing, "Rejected plan should preserve facing");
}

void TestInteractPlanIsUnsupportedAndDoesNotMove()
{
	iggy::PlayerCommandPlan2D plan;
	plan.type = iggy::PlayerCommandPlan2DType::Interact;
	plan.targetId = iggy::ResourceId("target:lever");
	const iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 1.0F, 2.0F }, { 1, 2 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::North);

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(player, plan, {}, {});

	Expect(result.status == iggy::PlayerMovementExecutionStatus::UnsupportedPlan, "Interact plan should be unsupported by movement executor");
	Expect(NearVec(result.state.position, player.position), "Interact plan should preserve position");
	Expect(result.state.movementStatus == iggy::PlayerMovementStatus::Idle, "Interact plan should set idle");
	Expect(result.state.facing == player.facing, "Interact plan should preserve facing");
}

void TestBodyBoundsHonorConfiguredSizeAndAnchor()
{
	const iggy::physics2d::CollisionObject2D wall = Object(
		iggy::ResourceId("wall"),
		{ { 2.0F, -0.5F }, { 3.0F, 0.5F } });
	const iggy::physics2d::CollisionWorld2D world = World({ wall });
	iggy::PlayerMovementExecutor2DConfig config;
	config.bodySize = { 2.0F, 1.0F };
	config.bodyAnchor = { 0.0F, 0.5F };
	config.maxStep = 2.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::South),
		MovePlan({ 4.0F, 0.0F }),
		world,
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Blocked, "configured body bounds should affect collision");
	Expect(NearVec(result.movement.allowedDelta, { 0.0F, 0.0F }), "configured size/anchor should start edge-touching the wall");
	ExpectBounds(result.movement.startBounds, { { 0.0F, -0.5F }, { 2.0F, 0.5F } }, "configured size/anchor should build expected body bounds");
}

void TestSolidFalseObjectCanBlockAndMetadataIsPreserved()
{
	const iggy::physics2d::CollisionObject2D trigger = Object(
		iggy::ResourceId("trigger"),
		{ { 2.5F, -0.5F }, { 3.5F, 0.5F } },
		false);
	const iggy::physics2d::CollisionWorld2D world = World({ trigger });
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 4.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::North),
		MovePlan({ 8.0F, 0.0F }),
		world,
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Blocked, "solid=false object should block through CharacterMove2D");
	Expect(result.movement.motion.hit.object.solid == false, "solid=false movement hit should preserve solid flag");
	ExpectObject(result.movement.motion.hit.object, trigger, "solid=false movement hit should preserve object metadata");
}

void TestNegativeBodySizeIsNormalized()
{
	iggy::PlayerMovementExecutor2DConfig config;
	config.bodySize = { -2.0F, -1.0F };
	config.bodyAnchor = { 0.5F, 1.0F };
	config.maxStep = 1.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		PlayerAgent(PlayerId, { 2.0F, 3.0F }, { 2, 3 }),
		MovePlan({ 3.0F, 3.0F }),
		{},
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Moved, "negative body size should still move in empty world");
	ExpectBounds(result.movement.startBounds, { { 1.0F, 2.0F }, { 3.0F, 3.0F } }, "negative body size should be normalized by absolute value");
}

void TestZeroBodySizeCanMoveInEmptyWorld()
{
	iggy::PlayerMovementExecutor2DConfig config;
	config.bodySize = { 0.0F, 0.0F };
	config.maxStep = 1.0F;

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(
		PlayerAgent(PlayerId, { 2.0F, 3.0F }, { 2, 3 }),
		MovePlan({ 3.0F, 3.0F }),
		{},
		config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Moved, "zero body size should move in empty world");
	ExpectBounds(result.movement.startBounds, { { 2.0F, 3.0F }, { 2.0F, 3.0F } }, "zero body size should produce degenerate bounds");
}

void TestFacingUpdatesByDominantAxis()
{
	iggy::PlayerMovementExecutor2DConfig config;
	config.maxStep = 1.0F;
	const iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::None);
	const iggy::physics2d::CollisionWorld2D world;

	const iggy::PlayerMovementExecutionResult east = iggy::PlayerMovementExecutor2D {}.execute(player, MovePlan({ 2.0F, 1.0F }), world, config);
	const iggy::PlayerMovementExecutionResult west = iggy::PlayerMovementExecutor2D {}.execute(player, MovePlan({ -2.0F, 1.0F }), world, config);
	const iggy::PlayerMovementExecutionResult south = iggy::PlayerMovementExecutor2D {}.execute(player, MovePlan({ 1.0F, 2.0F }), world, config);
	const iggy::PlayerMovementExecutionResult north = iggy::PlayerMovementExecutor2D {}.execute(player, MovePlan({ 1.0F, -2.0F }), world, config);
	const iggy::PlayerMovementExecutionResult tiedEast = iggy::PlayerMovementExecutor2D {}.execute(player, MovePlan({ 1.0F, 1.0F }), world, config);

	Expect(east.state.facing == iggy::PlayerFacing2D::East, "dominant positive x should face east");
	Expect(west.state.facing == iggy::PlayerFacing2D::West, "dominant negative x should face west");
	Expect(south.state.facing == iggy::PlayerFacing2D::South, "dominant positive y should face south");
	Expect(north.state.facing == iggy::PlayerFacing2D::North, "dominant negative y should face north");
	Expect(tiedEast.state.facing == iggy::PlayerFacing2D::East, "equal axis movement should prefer x facing");
}

void TestInputsAreNotMutated()
{
	iggy::PlayerAgentState player = PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West);
	const iggy::PlayerAgentState originalPlayer = player;
	iggy::PlayerCommandPlan2D plan = MovePlan({ 4.0F, 0.0F });
	const iggy::PlayerCommandPlan2D originalPlan = plan;
	iggy::PlayerMovementExecutor2DConfig config;
	config.bodySize = { 2.0F, 1.0F };
	config.bodyAnchor = { 0.0F, 0.5F };
	config.maxStep = 2.0F;
	const iggy::PlayerMovementExecutor2DConfig originalConfig = config;
	const iggy::physics2d::CollisionWorld2D world = World({
		Object(iggy::ResourceId("wall"), { { 5.0F, 0.0F }, { 6.0F, 1.0F } }),
	});
	const std::vector<iggy::physics2d::CollisionObject2D> beforeObjects = world.objects();

	const iggy::PlayerMovementExecutionResult result = iggy::PlayerMovementExecutor2D {}.execute(player, plan, world, config);

	Expect(result.status == iggy::PlayerMovementExecutionStatus::Moved, "mutation check should execute a movement");
	ExpectPlayerAgent(player, originalPlayer, "movement executor input player");
	Expect(plan.type == originalPlan.type && NearVec(plan.targetPoint, originalPlan.targetPoint), "movement executor should not mutate plan");
	Expect(NearVec(config.bodySize, originalConfig.bodySize) && NearVec(config.bodyAnchor, originalConfig.bodyAnchor) && Near(config.maxStep, originalConfig.maxStep), "movement executor should not mutate config");
	ExpectWorldUnchanged(world, beforeObjects, "movement executor should not mutate world");
}

} // namespace

int main()
{
	TestEmptyWorldMoveTowardFarTargetUsesMaxStep();
	TestMoveReachesTargetWithinMaxStep();
	TestBlockedMovementPreservesHitMetadata();
	TestPartialBlockedMovementKeepsMovingStatus();
	TestStartingOverlapBlocksWithoutMoving();
	TestStartingEdgeTouchBlocksWithoutMoving();
	TestAlreadyAtTargetDoesNotMove();
	TestNonPositiveMaxStepDoesNotMove();
	TestNoneAndWaitPlansDoNotMove();
	TestRejectedPlanDoesNotMove();
	TestInteractPlanIsUnsupportedAndDoesNotMove();
	TestBodyBoundsHonorConfiguredSizeAndAnchor();
	TestSolidFalseObjectCanBlockAndMetadataIsPreserved();
	TestNegativeBodySizeIsNormalized();
	TestZeroBodySizeCanMoveInEmptyWorld();
	TestFacingUpdatesByDominantAxis();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
