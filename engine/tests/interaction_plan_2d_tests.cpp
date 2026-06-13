#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionPlan2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind,
	iggy::Vec2 position,
	float radius,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "plan test registry setup should build");
	return result.registry;
}

void ExpectTarget(const iggy::InteractionTarget2D &target, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(target.id == expected.id, message);
	Expect(target.kind == expected.kind, message);
	Expect(NearVec(target.position, expected.position), message);
	Expect(target.radius == expected.radius, message);
	Expect(target.enabled == expected.enabled, message);
}

void ExpectQueryMatchesReach(const iggy::InteractionPlan2DResult &plan, const char *message)
{
	Expect(plan.reach.query.status == plan.query.status, message);
	Expect(plan.reach.query.targetId == plan.query.targetId, message);
	ExpectTarget(plan.reach.query.target, plan.query.target, message);
}

void TestEmptyTargetIdPlansMissingTargetId()
{
	const iggy::InteractionTarget2DRegistry registry = Registry({});
	const iggy::Vec2 actorPosition { 1.0F, 2.0F };

	const iggy::InteractionPlan2DResult result = iggy::InteractionPlan2D {}.plan(registry, {}, actorPosition);

	Expect(result.status == iggy::InteractionPlan2DStatus::MissingTargetId, "empty target id should plan MissingTargetId");
	Expect(!result.ready(), "empty target id plan should not be ready");
	Expect(result.targetId.empty(), "empty target id plan should preserve requested id");
	Expect(NearVec(result.actorPosition, actorPosition), "empty target id plan should preserve actor position");
	Expect(result.query.status == iggy::InteractionTargetQuery2DStatus::MissingTargetId, "empty target id plan should preserve query status");
	Expect(result.reach.status == iggy::InteractionReach2DStatus::MissingTargetId, "empty target id plan should preserve reach status");
	ExpectQueryMatchesReach(result, "empty target id plan reach should preserve query");
}

void TestMissingTargetPlansTargetNotFound()
{
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:known", iggy::InteractionTarget2DKind::Inspectable, { 0.0F, 0.0F }, 1.0F),
	});
	const iggy::ResourceId targetId { "target:missing" };

	const iggy::InteractionPlan2DResult result = iggy::InteractionPlan2D {}.plan(registry, targetId, { 0.0F, 0.0F });

	Expect(result.status == iggy::InteractionPlan2DStatus::TargetNotFound, "missing target should plan TargetNotFound");
	Expect(!result.ready(), "missing target plan should not be ready");
	Expect(result.targetId == targetId, "missing target plan should preserve requested id");
	Expect(result.query.status == iggy::InteractionTargetQuery2DStatus::NotFound, "missing target plan should preserve query status");
	Expect(result.reach.status == iggy::InteractionReach2DStatus::TargetNotFound, "missing target plan should preserve reach status");
	ExpectQueryMatchesReach(result, "missing target plan reach should preserve query");
}

void TestDisabledTargetPlansTargetDisabled()
{
	const iggy::InteractionTarget2D target = Target("target:disabled", iggy::InteractionTarget2DKind::Talk, { 2.0F, 0.0F }, 10.0F, false);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});

	const iggy::InteractionPlan2DResult result = iggy::InteractionPlan2D {}.plan(registry, target.id, { 2.0F, 0.0F });

	Expect(result.status == iggy::InteractionPlan2DStatus::TargetDisabled, "disabled target should plan TargetDisabled");
	Expect(!result.ready(), "disabled target plan should not be ready");
	Expect(result.query.status == iggy::InteractionTargetQuery2DStatus::Disabled, "disabled target plan should preserve query status");
	Expect(result.reach.status == iggy::InteractionReach2DStatus::TargetDisabled, "disabled target plan should preserve reach status");
	ExpectTarget(result.query.target, target, "disabled target plan should copy query target");
	ExpectTarget(result.reach.query.target, target, "disabled target plan should copy reach query target");
}

void TestReachableTargetPlansReady()
{
	const iggy::InteractionTarget2D target = Target("target:ready", iggy::InteractionTarget2DKind::Usable, { 3.0F, 4.0F }, 5.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});

	const iggy::InteractionPlan2DResult result = iggy::InteractionPlan2D {}.plan(registry, target.id, { 0.0F, 0.0F });

	Expect(result.status == iggy::InteractionPlan2DStatus::Ready, "reachable target should plan Ready");
	Expect(result.ready(), "reachable target plan should be ready");
	Expect(result.targetId == target.id, "reachable target plan should preserve requested id");
	Expect(result.query.status == iggy::InteractionTargetQuery2DStatus::Found, "reachable target plan should preserve query status");
	Expect(result.reach.status == iggy::InteractionReach2DStatus::Reachable, "reachable target plan should preserve reach status");
	Expect(Near(result.reach.distance, 5.0F), "reachable target plan should preserve reach distance");
	Expect(Near(result.reach.allowedDistance, 5.0F), "reachable target plan should preserve allowed distance");
	ExpectTarget(result.query.target, target, "reachable target plan should copy target fields");
}

void TestOutOfRangeTargetPlansOutOfRange()
{
	const iggy::InteractionTarget2D target = Target("target:far", iggy::InteractionTarget2DKind::Door, { 4.0F, 0.0F }, 3.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});

	const iggy::InteractionPlan2DResult result = iggy::InteractionPlan2D {}.plan(registry, target.id, { 0.0F, 0.0F });

	Expect(result.status == iggy::InteractionPlan2DStatus::OutOfRange, "out-of-range target should plan OutOfRange");
	Expect(!result.ready(), "out-of-range target plan should not be ready");
	Expect(result.query.status == iggy::InteractionTargetQuery2DStatus::Found, "out-of-range target plan should preserve found query status");
	Expect(result.reach.status == iggy::InteractionReach2DStatus::OutOfRange, "out-of-range target plan should preserve reach status");
	Expect(Near(result.reach.distance, 4.0F), "out-of-range target plan should preserve reach distance");
	Expect(Near(result.reach.allowedDistance, 3.0F), "out-of-range target plan should preserve allowed distance");
}

void TestExtraReachCanMakeTargetReady()
{
	const iggy::InteractionTarget2D target = Target("target:extra", iggy::InteractionTarget2DKind::Pickup, { 4.0F, 0.0F }, 3.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});

	const iggy::InteractionPlan2DResult result = iggy::InteractionPlan2D {}.plan(registry, target.id, { 0.0F, 0.0F }, { 1.0F });

	Expect(result.status == iggy::InteractionPlan2DStatus::Ready, "extra reach should make target ready");
	Expect(result.ready(), "extra reach ready plan should report ready");
	Expect(result.reach.status == iggy::InteractionReach2DStatus::Reachable, "extra reach plan should preserve reachable status");
	Expect(Near(result.reach.distance, 4.0F), "extra reach plan should preserve distance");
	Expect(Near(result.reach.allowedDistance, 4.0F), "extra reach plan should preserve expanded allowed distance");
}

void TestPreservesTargetIdActorPositionAndNestedData()
{
	const iggy::InteractionTarget2D target = Target("target:preserve", iggy::InteractionTarget2DKind::Door, { -3.0F, -4.0F }, 5.5F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		target,
	});
	const iggy::ResourceId targetId { "target:preserve" };
	const iggy::ResourceId targetIdBefore = targetId;
	const iggy::Vec2 actorPosition { -1.0F, -1.0F };

	const iggy::InteractionPlan2DResult result = iggy::InteractionPlan2D {}.plan(registry, targetId, actorPosition, { 0.5F });

	Expect(targetId == targetIdBefore, "interaction plan should not mutate caller-held target id");
	Expect(result.targetId == targetId, "interaction plan should preserve requested target id");
	Expect(NearVec(result.actorPosition, actorPosition), "interaction plan should preserve actor position");
	Expect(NearVec(result.reach.actorPosition, actorPosition), "interaction plan should preserve reach actor position");
	ExpectTarget(result.query.target, target, "interaction plan should preserve query target fields");
	ExpectTarget(result.reach.query.target, target, "interaction plan should preserve reach query target fields");
	ExpectQueryMatchesReach(result, "interaction plan should preserve nested query data in reach result");
	Expect(Near(result.reach.allowedDistance, 6.0F), "interaction plan should preserve reach config math");
}

void TestNamespacedAndUnqualifiedIdsRemainDistinct()
{
	const iggy::InteractionTarget2D unqualified = Target("door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F);
	const iggy::InteractionTarget2D namespaced = Target("target:door", iggy::InteractionTarget2DKind::Inspectable, { 2.0F, 0.0F }, 2.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		unqualified,
		namespaced,
	});

	const iggy::InteractionPlan2DResult unqualifiedResult = iggy::InteractionPlan2D {}.plan(registry, iggy::ResourceId { "door" }, { 0.0F, 0.0F });
	const iggy::InteractionPlan2DResult namespacedResult = iggy::InteractionPlan2D {}.plan(registry, iggy::ResourceId { "target:door" }, { 0.0F, 0.0F });

	Expect(unqualifiedResult.status == iggy::InteractionPlan2DStatus::Ready, "unqualified id should resolve to unqualified ready target");
	Expect(namespacedResult.status == iggy::InteractionPlan2DStatus::Ready, "namespaced id should resolve to namespaced ready target");
	ExpectTarget(unqualifiedResult.query.target, unqualified, "unqualified plan should preserve unqualified target");
	ExpectTarget(namespacedResult.query.target, namespaced, "namespaced plan should preserve namespaced target");
}

} // namespace

int main()
{
	TestEmptyTargetIdPlansMissingTargetId();
	TestMissingTargetPlansTargetNotFound();
	TestDisabledTargetPlansTargetDisabled();
	TestReachableTargetPlansReady();
	TestOutOfRangeTargetPlansOutOfRange();
	TestExtraReachCanMakeTargetReady();
	TestPreservesTargetIdActorPositionAndNestedData();
	TestNamespacedAndUnqualifiedIdsRemainDistinct();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
