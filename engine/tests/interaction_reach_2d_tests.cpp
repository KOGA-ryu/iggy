#include <cstdlib>

#include "scene/interaction/InteractionReach2D.hpp"
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

iggy::InteractionTargetQuery2DResult Query(
	iggy::InteractionTargetQuery2DStatus status,
	iggy::ResourceId targetId = {},
	iggy::InteractionTarget2D target = {})
{
	return { status, targetId, target };
}

void ExpectTarget(const iggy::InteractionTarget2D &target, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(target.id == expected.id, message);
	Expect(target.kind == expected.kind, message);
	Expect(NearVec(target.position, expected.position), message);
	Expect(target.radius == expected.radius, message);
	Expect(target.enabled == expected.enabled, message);
}

void ExpectQuery(const iggy::InteractionTargetQuery2DResult &query, const iggy::InteractionTargetQuery2DResult &expected, const char *message)
{
	Expect(query.status == expected.status, message);
	Expect(query.targetId == expected.targetId, message);
	ExpectTarget(query.target, expected.target, message);
}

void TestMissingTargetIdMapsToMissingTargetId()
{
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::MissingTargetId);
	const iggy::Vec2 actorPosition { 1.0F, 2.0F };

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate(actorPosition, query);

	Expect(result.status == iggy::InteractionReach2DStatus::MissingTargetId, "missing target id query should map to reach MissingTargetId");
	Expect(!result.reachable(), "missing target id query should not be reachable");
	ExpectQuery(result.query, query, "missing target id reach result should preserve query");
	Expect(NearVec(result.actorPosition, actorPosition), "missing target id reach result should preserve actor position");
	Expect(Near(result.distance, 0.0F), "missing target id reach result should not compute distance");
	Expect(Near(result.allowedDistance, 0.0F), "missing target id reach result should not compute allowed distance");
}

void TestNotFoundMapsToTargetNotFound()
{
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::NotFound, iggy::ResourceId { "target:missing" });

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ -1.0F, 3.0F }, query);

	Expect(result.status == iggy::InteractionReach2DStatus::TargetNotFound, "not-found query should map to reach TargetNotFound");
	Expect(!result.reachable(), "not-found query should not be reachable");
	ExpectQuery(result.query, query, "not-found reach result should preserve query");
	Expect(Near(result.distance, 0.0F), "not-found reach result should not compute distance");
	Expect(Near(result.allowedDistance, 0.0F), "not-found reach result should not compute allowed distance");
}

void TestDisabledMapsToTargetDisabledAndPreservesTarget()
{
	const iggy::InteractionTarget2D target = Target("target:disabled", iggy::InteractionTarget2DKind::Talk, { 4.0F, 5.0F }, 2.0F, false);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Disabled, target.id, target);

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ 4.0F, 5.0F }, query, { 99.0F });

	Expect(result.status == iggy::InteractionReach2DStatus::TargetDisabled, "disabled query should map to reach TargetDisabled");
	Expect(!result.reachable(), "disabled query should not be reachable");
	ExpectQuery(result.query, query, "disabled reach result should preserve query and copied target");
	Expect(Near(result.distance, 0.0F), "disabled reach result should not compute distance");
	Expect(Near(result.allowedDistance, 0.0F), "disabled reach result should not compute allowed distance");
}

void TestFoundSamePositionWithZeroRadiusIsReachable()
{
	const iggy::InteractionTarget2D target = Target("target:same", iggy::InteractionTarget2DKind::Usable, { 2.0F, 3.0F }, 0.0F);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Found, target.id, target);

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ 2.0F, 3.0F }, query);

	Expect(result.status == iggy::InteractionReach2DStatus::Reachable, "same-position zero-radius target should be reachable");
	Expect(result.reachable(), "same-position zero-radius result should report reachable");
	Expect(Near(result.distance, 0.0F), "same-position zero-radius result should compute zero distance");
	Expect(Near(result.allowedDistance, 0.0F), "same-position zero-radius result should allow zero distance");
}

void TestDistanceInsideRadiusIsReachable()
{
	const iggy::InteractionTarget2D target = Target("target:inside", iggy::InteractionTarget2DKind::Door, { 3.0F, 4.0F }, 5.0F);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Found, target.id, target);

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ 0.0F, 0.0F }, query);

	Expect(result.status == iggy::InteractionReach2DStatus::Reachable, "actor inside target radius should be reachable");
	Expect(result.reachable(), "inside radius result should report reachable");
	Expect(Near(result.distance, 5.0F), "inside radius result should compute Euclidean distance");
	Expect(Near(result.allowedDistance, 5.0F), "inside radius result should use target radius as allowed distance");
}

void TestDistanceAtRadiusBoundaryIsReachable()
{
	const iggy::InteractionTarget2D target = Target("target:boundary", iggy::InteractionTarget2DKind::Inspectable, { 6.0F, 8.0F }, 10.0F);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Found, target.id, target);

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ 0.0F, 0.0F }, query);

	Expect(result.status == iggy::InteractionReach2DStatus::Reachable, "actor exactly at radius boundary should be reachable");
	Expect(result.reachable(), "radius boundary result should report reachable");
	Expect(Near(result.distance, 10.0F), "radius boundary result should compute boundary distance");
	Expect(Near(result.allowedDistance, 10.0F), "radius boundary result should compute allowed distance");
}

void TestDistanceOutsideRadiusIsOutOfRange()
{
	const iggy::InteractionTarget2D target = Target("target:far", iggy::InteractionTarget2DKind::Pickup, { 4.0F, 0.0F }, 3.0F);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Found, target.id, target);

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ 0.0F, 0.0F }, query);

	Expect(result.status == iggy::InteractionReach2DStatus::OutOfRange, "actor outside radius should be out of range");
	Expect(!result.reachable(), "out-of-range result should not report reachable");
	Expect(Near(result.distance, 4.0F), "out-of-range result should compute Euclidean distance");
	Expect(Near(result.allowedDistance, 3.0F), "out-of-range result should compute allowed distance");
}

void TestExtraReachExpandsAllowedDistance()
{
	const iggy::InteractionTarget2D target = Target("target:extra", iggy::InteractionTarget2DKind::Usable, { 4.0F, 0.0F }, 2.5F);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Found, target.id, target);

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ 0.0F, 0.0F }, query, { 1.5F });

	Expect(result.status == iggy::InteractionReach2DStatus::Reachable, "extra reach should expand allowed interaction distance");
	Expect(result.reachable(), "extra reach result should report reachable");
	Expect(Near(result.distance, 4.0F), "extra reach result should compute distance");
	Expect(Near(result.allowedDistance, 4.0F), "extra reach result should add extra reach to radius");
}

void TestNegativeExtraReachClampsToZero()
{
	const iggy::InteractionTarget2D target = Target("target:negative_extra", iggy::InteractionTarget2DKind::Usable, { 4.0F, 0.0F }, 3.0F);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Found, target.id, target);

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ 0.0F, 0.0F }, query, { -10.0F });

	Expect(result.status == iggy::InteractionReach2DStatus::OutOfRange, "negative extra reach should clamp to zero");
	Expect(!result.reachable(), "negative extra reach should not expand range");
	Expect(Near(result.distance, 4.0F), "negative extra reach result should compute distance");
	Expect(Near(result.allowedDistance, 3.0F), "negative extra reach result should use radius only");
}

void TestNegativeTargetRadiusClampsToZeroDefensively()
{
	iggy::InteractionTarget2D target = Target("target:negative_radius", iggy::InteractionTarget2DKind::Unknown, { 1.0F, 0.0F }, -5.0F);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Found, target.id, target);

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate({ 1.0F, 0.0F }, query);

	Expect(result.status == iggy::InteractionReach2DStatus::Reachable, "negative target radius should clamp to zero defensively");
	Expect(result.reachable(), "negative target radius at same position should be reachable after clamp");
	Expect(Near(result.distance, 0.0F), "negative target radius result should compute distance");
	Expect(Near(result.allowedDistance, 0.0F), "negative target radius result should clamp allowed distance to zero");
	ExpectQuery(result.query, query, "negative target radius result should preserve original query payload");
}

void TestPreservesQueryAndActorDataExactly()
{
	const iggy::InteractionTarget2D target = Target("target:preserve", iggy::InteractionTarget2DKind::Door, { -3.0F, -4.0F }, 5.0F, true);
	const iggy::InteractionTargetQuery2DResult query = Query(iggy::InteractionTargetQuery2DStatus::Found, target.id, target);
	const iggy::Vec2 actorPosition { 1.25F, -2.5F };

	const iggy::InteractionReach2DResult result = iggy::InteractionReach2D {}.evaluate(actorPosition, query, { 2.0F });

	ExpectQuery(result.query, query, "reach result should preserve query payload exactly");
	Expect(NearVec(result.actorPosition, actorPosition), "reach result should preserve actor position exactly");
	Expect(Near(result.allowedDistance, 7.0F), "reach result should preserve target radius plus extra reach math");
}

} // namespace

int main()
{
	TestMissingTargetIdMapsToMissingTargetId();
	TestNotFoundMapsToTargetNotFound();
	TestDisabledMapsToTargetDisabledAndPreservesTarget();
	TestFoundSamePositionWithZeroRadiusIsReachable();
	TestDistanceInsideRadiusIsReachable();
	TestDistanceAtRadiusBoundaryIsReachable();
	TestDistanceOutsideRadiusIsOutOfRange();
	TestExtraReachExpandsAllowedDistance();
	TestNegativeExtraReachClampsToZero();
	TestNegativeTargetRadiusClampsToZeroDefensively();
	TestPreservesQueryAndActorDataExactly();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
