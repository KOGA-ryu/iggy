#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiMovementProposal2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcAiPathReport2DResult PathReport(
	iggy::NpcAiPathReport2DStatus status,
	iggy::navigation::NavigationPathStatus pathStatus,
	std::vector<iggy::Vec2> waypoints,
	iggy::Vec2 start = { 0.5F, 0.5F },
	iggy::Vec2 target = { 3.5F, 0.5F },
	iggy::NpcAiBehaviorIntent2DType intent = iggy::NpcAiBehaviorIntent2DType::Patrol,
	const char *npcId = "npc:proposal")
{
	iggy::NpcAiPathReport2DResult report;
	report.status = status;
	report.navigation.status = status == iggy::NpcAiPathReport2DStatus::PathFound
		? iggy::NpcAiNavigationRequest2DStatus::Built
		: iggy::NpcAiNavigationRequest2DStatus::NoRouteRequest;
	report.navigation.route.status = iggy::NpcAiRouteRequest2DStatus::Requested;
	report.navigation.route.startPosition = start;
	report.navigation.route.targetPosition = target;
	report.navigation.route.intentType = intent;
	report.navigation.route.requestsRoute = true;
	report.navigation.route.decision.status = iggy::NpcAiDecision2DStatus::Decided;
	report.navigation.route.decision.intent.type = intent;
	report.navigation.route.decision.score.currentStateValidation.state.npcId = Id(npcId);
	report.navigation.route.decision.score.currentStateValidation.state.position = start;
	report.path.status = pathStatus;
	report.path.waypoints = waypoints;
	for (std::size_t index = 0; index < waypoints.size(); ++index)
		report.path.tiles.push_back({ static_cast<int>(index), 0 });
	return report;
}

void ExpectNoProposal(
	const iggy::NpcAiMovementProposal2DResult &result,
	iggy::NpcAiMovementProposal2DStatus status,
	const char *message)
{
	Expect(result.status == status, message);
	Expect(!result.requestsMovement, message);
	Expect(!result.hasMovementProposal(), message);
}

void TestNoPathReturnsNoPath()
{
	const iggy::NpcAiPathReport2DResult path =
		PathReport(iggy::NpcAiPathReport2DStatus::PathNotFound, iggy::navigation::NavigationPathStatus::NoPath, {});

	const iggy::NpcAiMovementProposal2DResult result =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path);

	ExpectNoProposal(result, iggy::NpcAiMovementProposal2DStatus::NoPath, "path failure should not produce movement proposal");
	Expect(result.path.status == path.status, "NoPath result should preserve copied path report");
	Expect(result.intentType == iggy::NpcAiBehaviorIntent2DType::Patrol, "NoPath result should preserve behavior intent type");
}

void TestInvalidNavigationPathReturnsNoPath()
{
	const iggy::NpcAiPathReport2DResult path =
		PathReport(iggy::NpcAiPathReport2DStatus::NoNavigationRequest, iggy::navigation::NavigationPathStatus::NoPath, {});

	const iggy::NpcAiMovementProposal2DResult result =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path);

	ExpectNoProposal(result, iggy::NpcAiMovementProposal2DStatus::NoPath, "invalid/no navigation path should not produce movement proposal");
}

void TestHoldPositionReturnsHoldPositionEvenWithPathData()
{
	const iggy::NpcAiPathReport2DResult path =
		PathReport(
			iggy::NpcAiPathReport2DStatus::PathFound,
			iggy::navigation::NavigationPathStatus::Found,
			{ { 0.5F, 0.5F }, { 1.5F, 0.5F } },
			{ 0.5F, 0.5F },
			{ 1.5F, 0.5F },
			iggy::NpcAiBehaviorIntent2DType::HoldPosition);

	const iggy::NpcAiMovementProposal2DResult result =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path);

	ExpectNoProposal(result, iggy::NpcAiMovementProposal2DStatus::HoldPosition, "HoldPosition intent should not produce movement proposal even if path data exists");
	Expect(result.intentType == iggy::NpcAiBehaviorIntent2DType::HoldPosition, "HoldPosition result should preserve intent type");
}

void TestAlreadyAtTargetReturnsAlreadyAtTarget()
{
	const iggy::NpcAiPathReport2DResult path =
		PathReport(
			iggy::NpcAiPathReport2DStatus::PathFound,
			iggy::navigation::NavigationPathStatus::Found,
			{ { 0.5F, 0.5F } },
			{ 0.5F, 0.5F },
			{ 0.5005F, 0.5F });

	const iggy::NpcAiMovementProposal2DResult result =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path, { 1, 0.001F });

	ExpectNoProposal(result, iggy::NpcAiMovementProposal2DStatus::AlreadyAtTarget, "arrival tolerance should suppress movement when already at target");
	Expect(NearVec(result.startPosition, { 0.5F, 0.5F }), "AlreadyAtTarget should preserve start position");
	Expect(NearVec(result.finalTargetPosition, { 0.5005F, 0.5F }), "AlreadyAtTarget should preserve final target position");
}

void TestFoundPathProposesFirstNextWaypoint()
{
	const iggy::NpcAiPathReport2DResult path =
		PathReport(
			iggy::NpcAiPathReport2DStatus::PathFound,
			iggy::navigation::NavigationPathStatus::Found,
			{ { 0.5F, 0.5F }, { 1.5F, 0.5F }, { 2.5F, 0.5F }, { 3.5F, 0.5F } },
			{ 0.5F, 0.5F },
			{ 3.5F, 0.5F },
			iggy::NpcAiBehaviorIntent2DType::Investigate,
			"npc:scout");

	const iggy::NpcAiMovementProposal2DResult result =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path);

	Expect(result.status == iggy::NpcAiMovementProposal2DStatus::Proposed, "found path should produce movement proposal");
	Expect(result.requestsMovement, "found path should request movement");
	Expect(result.hasMovementProposal(), "found path should report movement proposal");
	Expect(result.npcId == Id("npc:scout"), "movement proposal should preserve NPC id");
	Expect(result.intentType == iggy::NpcAiBehaviorIntent2DType::Investigate, "movement proposal should preserve behavior intent");
	Expect(NearVec(result.startPosition, { 0.5F, 0.5F }), "movement proposal should preserve start position");
	Expect(NearVec(result.finalTargetPosition, { 3.5F, 0.5F }), "movement proposal should preserve final target position");
	Expect(result.selectedWaypointIndex == 1, "movement proposal should skip start waypoint when present");
	Expect(NearVec(result.proposedPosition, { 1.5F, 0.5F }), "movement proposal should choose first next waypoint");
}

void TestPathWithOnlyTargetWaypointUsesTarget()
{
	const iggy::NpcAiPathReport2DResult path =
		PathReport(
			iggy::NpcAiPathReport2DStatus::PathFound,
			iggy::navigation::NavigationPathStatus::Found,
			{ { 3.5F, 0.5F } },
			{ 0.5F, 0.5F },
			{ 3.5F, 0.5F });

	const iggy::NpcAiMovementProposal2DResult result =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path);

	Expect(result.status == iggy::NpcAiMovementProposal2DStatus::Proposed, "target-only path should produce movement proposal");
	Expect(result.selectedWaypointIndex == 0, "target-only path should select only waypoint");
	Expect(NearVec(result.proposedPosition, { 3.5F, 0.5F }), "target-only path should propose final target waypoint");
}

void TestLookaheadChoosesLaterWaypointAndClamps()
{
	const iggy::NpcAiPathReport2DResult path =
		PathReport(
			iggy::NpcAiPathReport2DStatus::PathFound,
			iggy::navigation::NavigationPathStatus::Found,
			{ { 0.5F, 0.5F }, { 1.5F, 0.5F }, { 2.5F, 0.5F }, { 3.5F, 0.5F } },
			{ 0.5F, 0.5F },
			{ 3.5F, 0.5F });

	const iggy::NpcAiMovementProposal2DResult lookahead =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path, { 2, 0.001F });
	const iggy::NpcAiMovementProposal2DResult clamped =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path, { 99, 0.001F });

	Expect(lookahead.status == iggy::NpcAiMovementProposal2DStatus::Proposed, "lookahead setup should propose movement");
	Expect(lookahead.selectedWaypointIndex == 2, "lookahead should choose later waypoint");
	Expect(NearVec(lookahead.proposedPosition, { 2.5F, 0.5F }), "lookahead should preserve selected waypoint position");
	Expect(clamped.selectedWaypointIndex == 3, "large lookahead should clamp to final waypoint");
	Expect(NearVec(clamped.proposedPosition, { 3.5F, 0.5F }), "clamped lookahead should propose final waypoint");
}

void TestInputPathReportIsNotMutated()
{
	iggy::NpcAiPathReport2DResult path =
		PathReport(
			iggy::NpcAiPathReport2DStatus::PathFound,
			iggy::navigation::NavigationPathStatus::Found,
			{ { 0.5F, 0.5F }, { 1.5F, 0.5F }, { 2.5F, 0.5F } },
			{ 0.5F, 0.5F },
			{ 2.5F, 0.5F },
			iggy::NpcAiBehaviorIntent2DType::Patrol,
			"npc:immutable");
	const iggy::NpcAiPathReport2DStatus statusBefore = path.status;
	const iggy::navigation::NavigationPathStatus pathStatusBefore = path.path.status;
	const std::vector<iggy::Vec2> waypointsBefore = path.path.waypoints;
	const iggy::Vec2 startBefore = path.navigation.route.startPosition;
	const iggy::Vec2 targetBefore = path.navigation.route.targetPosition;
	const iggy::ResourceId npcIdBefore = path.navigation.route.decision.score.currentStateValidation.state.npcId;

	const iggy::NpcAiMovementProposal2DResult result =
		iggy::NpcAiMovementProposalBuilder2D {}.build(path);

	Expect(result.status == iggy::NpcAiMovementProposal2DStatus::Proposed, "immutability setup should propose movement");
	Expect(path.status == statusBefore, "movement proposal should not mutate path report status");
	Expect(path.path.status == pathStatusBefore, "movement proposal should not mutate path status");
	Expect(path.path.waypoints == waypointsBefore, "movement proposal should not mutate path waypoints");
	Expect(NearVec(path.navigation.route.startPosition, startBefore), "movement proposal should not mutate route start");
	Expect(NearVec(path.navigation.route.targetPosition, targetBefore), "movement proposal should not mutate route target");
	Expect(path.navigation.route.decision.score.currentStateValidation.state.npcId == npcIdBefore, "movement proposal should not mutate NPC id");
}

} // namespace

int main()
{
	TestNoPathReturnsNoPath();
	TestInvalidNavigationPathReturnsNoPath();
	TestHoldPositionReturnsHoldPositionEvenWithPathData();
	TestAlreadyAtTargetReturnsAlreadyAtTarget();
	TestFoundPathProposesFirstNextWaypoint();
	TestPathWithOnlyTargetWaypointUsesTarget();
	TestLookaheadChoosesLaterWaypointAndClamps();
	TestInputPathReportIsNotMutated();

	return Failures;
}
