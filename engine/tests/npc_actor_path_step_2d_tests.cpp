#include <cstdlib>
#include <string_view>
#include <vector>

#include "scene/npc/NpcActorPathStep2D.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;
using iggy::test::SameTile;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap MapFromRows(std::vector<std::string_view> rows)
{
	return iggy::test::MapFromRows(rows);
}

iggy::NpcActorRouteTarget2D ReadyRoute(
	const char *npcId,
	iggy::Vec2 start,
	iggy::Vec2 target,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk)
{
	iggy::NpcActorMovementIntent2D intent;
	intent.status = iggy::NpcActorMovementIntent2DStatus::Ready;
	intent.type = iggy::NpcActorMovementIntent2DType::MoveTo;
	intent.npcId = Id(npcId);
	intent.startPosition = start;
	intent.targetPosition = target;
	intent.moveMode = moveMode;
	intent.speedMultiplier = iggy::npcMoveModeSpeedMultiplier(moveMode);
	intent.requestsMovement = true;

	iggy::NpcActorRouteTarget2D route;
	route.intent = intent;
	route.status = iggy::NpcActorRouteTarget2DStatus::Ready;
	route.type = iggy::NpcActorRouteTarget2DType::MoveTo;
	route.npcId = intent.npcId;
	route.startPosition = start;
	route.targetPosition = target;
	route.moveMode = moveMode;
	route.speedMultiplier = intent.speedMultiplier;
	route.requestsRoute = true;
	return route;
}

iggy::NpcActorPathReport2D PathReportFor(
	const iggy::LevelTileMap &map,
	iggy::Vec2 start,
	iggy::Vec2 target,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk,
	const char *npcId = "npc:runner")
{
	const iggy::NpcActorNavigationRequest2D navigation =
		iggy::NpcActorNavigationRequestBuilder2D {}.build(
			ReadyRoute(npcId, start, target, moveMode),
			map);
	return iggy::NpcActorPathReporter2D {}.findPath(navigation, map);
}

iggy::NpcActorPathReport2D ManualFoundPath(
	iggy::Vec2 start,
	iggy::Vec2 target,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk,
	const char *npcId = "npc:manual")
{
	iggy::NpcActorPathReport2D report;
	report.navigation.route = ReadyRoute(npcId, start, target, moveMode);
	report.navigation.status = iggy::NpcActorNavigationRequest2DStatus::Ready;
	report.navigation.requestsPath = true;
	report.status = iggy::NpcActorPathReport2DStatus::PathFound;
	report.requestsStep = true;
	report.path.status = iggy::navigation::NavigationPathStatus::Found;
	report.path.tiles.push_back(iggy::tileForPoint(start));
	report.path.waypoints.push_back(start);
	if (!NearVec(start, target)) {
		report.path.tiles.push_back(iggy::tileForPoint(target));
		report.path.waypoints.push_back(target);
	}
	return report;
}

void TestNoPathReportProducesNoMovement()
{
	iggy::NpcActorPathReport2D report;
	report.status = iggy::NpcActorPathReport2DStatus::NoNavigationRequest;
	report.requestsStep = false;

	const iggy::NpcActorPathStep2D step = iggy::NpcActorPathStepper2D {}.step(report);

	Expect(step.status == iggy::NpcActorPathStep2DStatus::NoPath, "missing path should map to NoPath");
	Expect(!step.requestsMovement, "missing path should not request movement");
	Expect(!step.proposed(), "missing path should not be proposed");
	Expect(step.pathReport.status == report.status, "path step should preserve copied no-path report");
}

void TestWalkProposesFirstBoundedStep()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorPathReport2D report =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::Walk, "npc:walker");

	const iggy::NpcActorPathStep2D step = iggy::NpcActorPathStepper2D {}.step(report);

	Expect(step.status == iggy::NpcActorPathStep2DStatus::Proposed, "walk path should propose a step");
	Expect(step.ready(), "proposed walk step should be ready");
	Expect(step.requestsMovement, "proposed walk step should request movement");
	Expect(step.npcId == Id("npc:walker"), "path step should preserve npc id");
	Expect(NearVec(step.oldPosition, { 0.5F, 0.5F }), "path step should preserve old position");
	Expect(NearVec(step.proposedPosition, { 1.5F, 0.5F }), "walk step should advance one base distance to next waypoint");
	Expect(step.moveMode == iggy::NpcMoveMode::Walk, "path step should preserve walk mode");
	Expect(Near(step.requestedDistance, 1.0F), "path step should preserve requested base distance");
	Expect(Near(step.maxDistance, 1.0F), "walk max distance should equal base distance");
	Expect(!step.completedPath, "one walk step should not complete longer path");
	Expect(SameTile(step.oldTile, 0, 0), "old tile should come from old position");
	Expect(SameTile(step.proposedTile, 1, 0), "proposed tile should come from proposed position");
}

void TestMoveModeMultiplierCanAdvanceFartherThanWalk()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorPathReport2D walkReport =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::Walk);
	const iggy::NpcActorPathReport2D runReport =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::Run);
	const iggy::NpcActorPathReport2D sprintReport =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::Sprint);

	const iggy::NpcActorPathStep2D walk = iggy::NpcActorPathStepper2D {}.step(walkReport);
	const iggy::NpcActorPathStep2D run = iggy::NpcActorPathStepper2D {}.step(runReport);
	const iggy::NpcActorPathStep2D sprint = iggy::NpcActorPathStepper2D {}.step(sprintReport);

	Expect(NearVec(walk.proposedPosition, { 1.5F, 0.5F }), "walk should advance one tile center");
	Expect(NearVec(run.proposedPosition, { 2.5F, 0.5F }), "run should advance two base distances");
	Expect(Near(run.maxDistance, 2.0F), "run max distance should use move mode multiplier");
	Expect(NearVec(sprint.proposedPosition, { 3.5F, 0.5F }), "sprint should reach final target on this path");
	Expect(sprint.completedPath, "sprint should mark completed path when it reaches target");
	Expect(sprint.requestsMovement, "sprint completion from a different start still requests movement");
}

void TestLargeStepCanCompletePath()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorPathReport2D report =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::Walk);

	iggy::NpcActorPathStep2DConfig config;
	config.baseStepDistance = 4.0F;
	const iggy::NpcActorPathStep2D step = iggy::NpcActorPathStepper2D {}.step(report, config);

	Expect(step.status == iggy::NpcActorPathStep2DStatus::Proposed, "large path step should still be proposed");
	Expect(step.completedPath, "large path step should complete path");
	Expect(step.requestsMovement, "large path step should request movement if start differs from target");
	Expect(NearVec(step.proposedPosition, { 3.5F, 0.5F }), "large path step should land on final target");
	Expect(SameTile(step.proposedTile, 3, 0), "large path step should preserve target tile");
}

void TestAlreadyAtTargetDoesNotRequestMovement()
{
	const iggy::NpcActorPathReport2D report =
		ManualFoundPath({ 2.5F, 1.5F }, { 2.5F, 1.5F }, iggy::NpcMoveMode::Walk, "npc:done");

	const iggy::NpcActorPathStep2D step = iggy::NpcActorPathStepper2D {}.step(report);

	Expect(step.status == iggy::NpcActorPathStep2DStatus::AlreadyAtTarget, "already-at-target path should not propose movement");
	Expect(!step.requestsMovement, "already-at-target path should not request movement");
	Expect(step.completedPath, "already-at-target path should mark completed path");
	Expect(NearVec(step.oldPosition, { 2.5F, 1.5F }), "already-at-target should preserve current position");
	Expect(NearVec(step.proposedPosition, { 2.5F, 1.5F }), "already-at-target should preserve proposed position as current");
}

void TestNonMovingModesProduceZeroStepOnValidPath()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorPathReport2D stillReport =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::Still);
	const iggy::NpcActorPathReport2D noneReport =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::None);

	const iggy::NpcActorPathStep2D still = iggy::NpcActorPathStepper2D {}.step(stillReport);
	const iggy::NpcActorPathStep2D none = iggy::NpcActorPathStepper2D {}.step(noneReport);

	Expect(still.status == iggy::NpcActorPathStep2DStatus::ZeroStep, "Still mode should produce ZeroStep");
	Expect(!still.requestsMovement, "Still mode should not request movement");
	Expect(none.status == iggy::NpcActorPathStep2DStatus::ZeroStep, "None mode should produce ZeroStep");
	Expect(!none.requestsMovement, "None mode should not request movement");
}

void TestInvalidMoveModeIsRejected()
{
	iggy::NpcActorPathReport2D report =
		ManualFoundPath({ 0.5F, 0.5F }, { 1.5F, 0.5F }, iggy::NpcMoveMode::Walk);
	report.navigation.route.moveMode = static_cast<iggy::NpcMoveMode>(999);

	const iggy::NpcActorPathStep2D step = iggy::NpcActorPathStepper2D {}.step(report);

	Expect(step.status == iggy::NpcActorPathStep2DStatus::InvalidMoveMode, "unknown move mode should be rejected");
	Expect(!step.requestsMovement, "unknown move mode should not request movement");
	Expect(step.moveMode == static_cast<iggy::NpcMoveMode>(999), "path step should preserve invalid mode for diagnostics");
}

void TestCustomBaseDistanceControlsBoundedStep()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	const iggy::NpcActorPathReport2D report =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::Walk);
	iggy::NpcActorPathStep2DConfig config;
	config.baseStepDistance = 0.25F;

	const iggy::NpcActorPathStep2D step = iggy::NpcActorPathStepper2D {}.step(report, config);

	Expect(step.status == iggy::NpcActorPathStep2DStatus::Proposed, "partial custom step should be proposed");
	Expect(Near(step.maxDistance, 0.25F), "custom base distance should drive max distance");
	Expect(NearVec(step.proposedPosition, { 0.75F, 0.5F }), "custom base distance should produce partial follower step");
	Expect(step.follower.waypointIndex == 1, "partial follower step should preserve target waypoint index");
}

void TestCopiedPathReportAndNestedFactsArePreserved()
{
	const iggy::LevelTileMap map = MapFromRows({ "...." });
	iggy::NpcActorPathReport2D report =
		PathReportFor(map, { 0.5F, 0.5F }, { 3.5F, 0.5F }, iggy::NpcMoveMode::Jog, "runner");
	const iggy::NpcActorPathReport2DStatus statusBefore = report.status;
	const bool requestsStepBefore = report.requestsStep;
	const iggy::Vec2 startBefore = report.navigation.route.startPosition;
	const iggy::Vec2 targetBefore = report.navigation.route.targetPosition;
	const iggy::NpcMoveMode moveModeBefore = report.navigation.route.moveMode;
	const std::size_t waypointCountBefore = report.path.waypoints.size();

	const iggy::NpcActorPathStep2D step = iggy::NpcActorPathStepper2D {}.step(report);

	Expect(step.status == iggy::NpcActorPathStep2DStatus::Proposed, "immutability setup should propose a step");
	Expect(step.pathReport.status == statusBefore, "path step should preserve copied report status");
	Expect(step.pathReport.requestsStep == requestsStepBefore, "path step should preserve copied requestsStep");
	Expect(step.pathReport.navigation.route.npcId == Id("runner"), "path step should preserve unqualified npc id");
	Expect(step.pathReport.navigation.route.moveMode == moveModeBefore, "path step should preserve nested move mode");
	Expect(NearVec(step.pathReport.navigation.route.startPosition, startBefore), "path step should preserve nested start position");
	Expect(NearVec(step.pathReport.navigation.route.targetPosition, targetBefore), "path step should preserve nested target position");
	Expect(step.pathReport.path.waypoints.size() == waypointCountBefore, "path step should preserve copied waypoints");
	Expect(report.status == statusBefore, "path stepper should not mutate input status");
	Expect(report.requestsStep == requestsStepBefore, "path stepper should not mutate input requestsStep");
	Expect(NearVec(report.navigation.route.startPosition, startBefore), "path stepper should not mutate input route start");
	Expect(NearVec(report.navigation.route.targetPosition, targetBefore), "path stepper should not mutate input route target");
	Expect(report.navigation.route.moveMode == moveModeBefore, "path stepper should not mutate input move mode");
	Expect(report.path.waypoints.size() == waypointCountBefore, "path stepper should not mutate input path");
}

} // namespace

int main()
{
	TestNoPathReportProducesNoMovement();
	TestWalkProposesFirstBoundedStep();
	TestMoveModeMultiplierCanAdvanceFartherThanWalk();
	TestLargeStepCanCompletePath();
	TestAlreadyAtTargetDoesNotRequestMovement();
	TestNonMovingModesProduceZeroStepOnValidPath();
	TestInvalidMoveModeIsRejected();
	TestCustomBaseDistanceControlsBoundedStep();
	TestCopiedPathReportAndNestedFactsArePreserved();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
