#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiRouteRequest2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::AiMapNode2D Node(
	const char *id,
	iggy::Vec2 position,
	float radius,
	float patrolWeight,
	float coverWeight,
	float dangerWeight,
	float interestWeight)
{
	return {
		Id(id),
		position,
		radius,
		patrolWeight,
		coverWeight,
		dangerWeight,
		interestWeight,
		{},
		{},
		true,
	};
}

iggy::AiMap2D Map(std::vector<iggy::AiMapNode2D> nodes)
{
	return { nodes };
}

iggy::NpcAiProfile2D Profile(
	const char *profileId = "ai-profile:route",
	float aggression = 0.0F,
	float bravery = 0.0F,
	float alertness = 1.0F)
{
	return {
		Id(profileId),
		Id("faction:town"),
		aggression,
		bravery,
		alertness,
		4.0F,
		{},
	};
}

iggy::NpcAiCurrentState2D State(iggy::Vec2 position = { 0.0F, 0.0F })
{
	return {
		Id("npc:route"),
		position,
		Id("goal:route"),
		true,
	};
}

iggy::NpcAiDecision2DResult Decide(
	const iggy::AiMap2D &map,
	const iggy::NpcAiProfile2D &profile = Profile(),
	const iggy::NpcAiCurrentState2D &state = State(),
	const iggy::NpcAiDecision2DConfig &config = {})
{
	return iggy::NpcAiDecisionMaker2D {}.decide(map, profile, state, config);
}

void ExpectRoute(
	const iggy::NpcAiRouteRequest2DResult &result,
	iggy::NpcAiBehaviorIntent2DType intent,
	iggy::Vec2 start,
	iggy::Vec2 target,
	const char *message)
{
	Expect(result.status == iggy::NpcAiRouteRequest2DStatus::Requested, message);
	Expect(result.requestsRoute, message);
	Expect(result.hasRouteRequest(), message);
	Expect(result.intentType == intent, message);
	Expect(NearVec(result.startPosition, start), message);
	Expect(NearVec(result.targetPosition, target), message);
}

void TestNonDecidedStatusesProduceNoDecision()
{
	iggy::NpcAiDecision2DResult decision;
	decision.status = iggy::NpcAiDecision2DStatus::NoMapContext;
	decision.intent.type = iggy::NpcAiBehaviorIntent2DType::Patrol;
	decision.target.selectedPosition = { 10.0F, 0.0F };

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	Expect(result.status == iggy::NpcAiRouteRequest2DStatus::NoDecision, "non-decided decision should not request route");
	Expect(!result.requestsRoute, "non-decided decision should keep route request false");
	Expect(!result.hasRouteRequest(), "non-decided decision should not have route request");
	Expect(result.decision.status == decision.status, "non-decided route result should preserve copied decision");
}

void TestHoldPositionProducesNoRoute()
{
	const iggy::AiMap2D map = Map({
		Node("ai:quiet", { 2.0F, 3.0F }, 2.0F, 1.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::NpcAiCurrentState2D state = State({ 2.0F, 3.0F });
	const iggy::NpcAiDecision2DResult decision =
		Decide(map, Profile("ai-profile:hold"), state, { { 10.0F } });

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	Expect(result.status == iggy::NpcAiRouteRequest2DStatus::HoldPosition, "HoldPosition decision should not request route");
	Expect(!result.requestsRoute, "HoldPosition decision should keep route request false");
	Expect(result.intentType == iggy::NpcAiBehaviorIntent2DType::HoldPosition, "HoldPosition result should preserve intent type");
	Expect(NearVec(result.startPosition, state.position), "HoldPosition result should preserve start position");
	Expect(NearVec(result.targetPosition, state.position), "HoldPosition result should preserve target position");
}

void TestPatrolProducesRouteRequest()
{
	const iggy::AiMap2D map = Map({
		Node("ai:patrol", { 3.0F, 0.0F }, 4.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::NpcAiDecision2DResult decision = Decide(map);

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	ExpectRoute(result, iggy::NpcAiBehaviorIntent2DType::Patrol, { 0.0F, 0.0F }, { 3.0F, 0.0F }, "Patrol decision should request route to selected patrol target");
}

void TestCoverProducesRouteRequest()
{
	const iggy::AiMap2D map = Map({
		Node("ai:cover", { 0.0F, 3.0F }, 4.0F, 0.0F, 5.0F, 0.0F, 0.0F),
	});
	const iggy::NpcAiDecision2DResult decision = Decide(map, Profile("ai-profile:cover"));

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	ExpectRoute(result, iggy::NpcAiBehaviorIntent2DType::TakeCover, { 0.0F, 0.0F }, { 0.0F, 3.0F }, "TakeCover decision should request route to selected cover target");
}

void TestAvoidDangerProducesRouteRequest()
{
	const iggy::AiMap2D map = Map({
		Node("ai:danger-high", { 0.0F, 0.0F }, 4.0F, 0.0F, 0.0F, 8.0F, 0.0F),
		Node("ai:danger-low", { 2.0F, 0.0F }, 4.0F, 0.0F, 0.0F, 1.0F, 0.0F),
	});
	const iggy::NpcAiDecision2DResult decision =
		Decide(map, Profile("ai-profile:danger", 0.0F, 0.0F, 1.0F));

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	ExpectRoute(result, iggy::NpcAiBehaviorIntent2DType::AvoidDanger, { 0.0F, 0.0F }, { 2.0F, 0.0F }, "AvoidDanger decision should request route to safest selected target");
}

void TestInvestigateProducesRouteRequest()
{
	const iggy::AiMap2D map = Map({
		Node("ai:interest", { 0.0F, 2.0F }, 4.0F, 0.0F, 0.0F, 0.0F, 5.0F),
	});
	const iggy::NpcAiDecision2DResult decision =
		Decide(map, Profile("ai-profile:interest", 0.0F, 0.0F, 1.0F));

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	ExpectRoute(result, iggy::NpcAiBehaviorIntent2DType::Investigate, { 0.0F, 0.0F }, { 0.0F, 2.0F }, "Investigate decision should request route to selected interest target");
}

void TestArrivalToleranceSuppressesRoute()
{
	const iggy::AiMap2D map = Map({
		Node("ai:near", { 0.05F, 0.0F }, 1.0F, 2.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::NpcAiDecision2DResult decision = Decide(map);

	const iggy::NpcAiRouteRequest2DResult suppressed =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision, { 0.1F });
	const iggy::NpcAiRouteRequest2DResult requested =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision, { 0.001F });

	Expect(suppressed.status == iggy::NpcAiRouteRequest2DStatus::NoMovementNeeded, "arrival tolerance should suppress route when already near target");
	Expect(!suppressed.requestsRoute, "suppressed route should keep request false");
	Expect(requested.status == iggy::NpcAiRouteRequest2DStatus::Requested, "smaller arrival tolerance should request route");
	Expect(requested.requestsRoute, "smaller arrival tolerance should set route request true");
}

void TestStartAndTargetPositionsComeFromDecisionFacts()
{
	const iggy::AiMap2D map = Map({
		Node("ai:target", { 5.0F, 6.0F }, 10.0F, 4.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::NpcAiCurrentState2D state = State({ 1.0F, 2.0F });
	const iggy::NpcAiDecision2DResult decision = Decide(map, Profile("ai-profile:positions"), state);

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	Expect(result.status == iggy::NpcAiRouteRequest2DStatus::Requested, "position setup should request route");
	Expect(NearVec(result.startPosition, state.position), "route start should come from NPC current state");
	Expect(NearVec(result.targetPosition, decision.target.selectedPosition), "route target should come from selected decision target");
}

void TestCopiedDecisionIsPreserved()
{
	const iggy::AiMap2D map = Map({
		Node("ai:copy", { 3.0F, 4.0F }, 5.0F, 4.0F, 0.0F, 0.0F, 0.0F),
	});
	const iggy::NpcAiDecision2DResult decision = Decide(map, Profile("ai-profile:copy"));

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	Expect(result.status == iggy::NpcAiRouteRequest2DStatus::Requested, "copied decision setup should request route");
	Expect(result.decision.status == decision.status, "route result should preserve copied decision status");
	Expect(result.decision.intent.type == decision.intent.type, "route result should preserve copied intent type");
	Expect(result.decision.target.selectedNodeId == decision.target.selectedNodeId, "route result should preserve copied target node id");
	Expect(NearVec(result.decision.target.selectedPosition, decision.target.selectedPosition), "route result should preserve copied target position");
}

void TestInputDecisionIsNotMutated()
{
	const iggy::AiMap2D map = Map({
		Node("ai:immutable", { 3.0F, 0.0F }, 4.0F, 5.0F, 0.0F, 0.0F, 0.0F),
	});
	iggy::NpcAiDecision2DResult decision = Decide(map, Profile("ai-profile:immutable"));
	const iggy::NpcAiDecision2DStatus statusBefore = decision.status;
	const iggy::NpcAiBehaviorIntent2DType intentBefore = decision.intent.type;
	const iggy::ResourceId selectedNodeBefore = decision.target.selectedNodeId;
	const iggy::Vec2 selectedPositionBefore = decision.target.selectedPosition;

	const iggy::NpcAiRouteRequest2DResult result =
		iggy::NpcAiRouteRequestBuilder2D {}.build(decision);

	Expect(result.status == iggy::NpcAiRouteRequest2DStatus::Requested, "immutability setup should request route");
	Expect(decision.status == statusBefore, "route request builder should not mutate decision status");
	Expect(decision.intent.type == intentBefore, "route request builder should not mutate decision intent");
	Expect(decision.target.selectedNodeId == selectedNodeBefore, "route request builder should not mutate target node id");
	Expect(NearVec(decision.target.selectedPosition, selectedPositionBefore), "route request builder should not mutate target position");
}

} // namespace

int main()
{
	TestNonDecidedStatusesProduceNoDecision();
	TestHoldPositionProducesNoRoute();
	TestPatrolProducesRouteRequest();
	TestCoverProducesRouteRequest();
	TestAvoidDangerProducesRouteRequest();
	TestInvestigateProducesRouteRequest();
	TestArrivalToleranceSuppressesRoute();
	TestStartAndTargetPositionsComeFromDecisionFacts();
	TestCopiedDecisionIsPreserved();
	TestInputDecisionIsNotMutated();

	return Failures;
}
