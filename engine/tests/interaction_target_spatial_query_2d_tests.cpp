#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionTargetSpatialQuery2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::Vec2 position,
	float radius,
	bool enabled = true,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "spatial query test registry setup should build");
	return result.registry;
}

void ExpectDefaultResult(
	const iggy::InteractionTargetSpatialQuery2DResult &result,
	const char *message)
{
	Expect(result.status == iggy::InteractionTargetSpatialQuery2DStatus::NotFound, message);
	Expect(!result.hasTarget(), message);
	Expect(result.targetId.empty(), message);
	Expect(result.target.id.empty(), message);
	Expect(result.target.kind == iggy::InteractionTarget2DKind::Unknown, message);
	Expect(NearVec(result.target.position, { 0.0F, 0.0F }), message);
	Expect(result.target.radius == 0.0F, message);
	Expect(result.target.enabled, message);
	Expect(result.targetIndex == 0, message);
	Expect(result.distance == 0.0F, message);
	Expect(result.allowedDistance == 0.0F, message);
}

void ExpectTarget(
	const iggy::InteractionTarget2D &actual,
	const iggy::InteractionTarget2D &expected,
	const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void TestEmptyRegistryReturnsNotFound()
{
	const iggy::InteractionTarget2DRegistry registry = Registry({});

	const iggy::InteractionTargetSpatialQuery2DResult result =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { 1.0F, 2.0F });

	ExpectDefaultResult(result, "empty registry spatial query should return stable NotFound defaults");
}

void TestPointInsideOneEnabledTargetRadiusReturnsFound()
{
	const iggy::InteractionTarget2D target =
		Target("target:door", { 2.0F, 3.0F }, 2.0F, true, iggy::InteractionTarget2DKind::Door);
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:far", { 10.0F, 10.0F }, 1.0F),
		target,
	});

	const iggy::InteractionTargetSpatialQuery2DResult result =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { 3.0F, 3.0F });

	Expect(result.status == iggy::InteractionTargetSpatialQuery2DStatus::Found, "in-radius query should report Found");
	Expect(result.hasTarget(), "in-radius query should have target");
	Expect(result.targetId == target.id, "in-radius query should preserve target id");
	ExpectTarget(result.target, target, "in-radius query should copy target");
	Expect(result.targetIndex == 1, "in-radius query should preserve target index");
	Expect(Near(result.distance, 1.0F), "in-radius query should compute Euclidean distance");
	Expect(Near(result.allowedDistance, 2.0F), "in-radius query should compute allowed distance");
}

void TestPointAtExactRadiusBoundaryReturnsFound()
{
	const iggy::InteractionTarget2D target = Target("target:boundary", { 0.0F, 0.0F }, 5.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });

	const iggy::InteractionTargetSpatialQuery2DResult result =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { 3.0F, 4.0F });

	Expect(result.status == iggy::InteractionTargetSpatialQuery2DStatus::Found, "exact radius boundary should match");
	Expect(result.targetId == target.id, "exact radius boundary should return target");
	Expect(Near(result.distance, 5.0F), "exact radius boundary should report distance");
	Expect(Near(result.allowedDistance, 5.0F), "exact radius boundary should report allowed distance");
}

void TestPointOutsideAllRadiiReturnsNotFound()
{
	const iggy::InteractionTarget2DRegistry registry = Registry({
		Target("target:a", { 0.0F, 0.0F }, 0.5F),
		Target("target:b", { 4.0F, 0.0F }, 1.0F),
	});

	const iggy::InteractionTargetSpatialQuery2DResult result =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { 2.0F, 0.0F });

	ExpectDefaultResult(result, "outside all target radii should return NotFound defaults");
}

void TestZeroRadiusTargetMatchesOnlySamePosition()
{
	const iggy::InteractionTarget2D target = Target("target:pin", { -2.0F, 7.0F }, 0.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });

	const iggy::InteractionTargetSpatialQuery2DResult samePosition =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { -2.0F, 7.0F });
	const iggy::InteractionTargetSpatialQuery2DResult offset =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { -2.0F, 7.0001F });

	Expect(samePosition.status == iggy::InteractionTargetSpatialQuery2DStatus::Found, "zero-radius target should match same position");
	Expect(samePosition.targetId == target.id, "zero-radius same-position match should return target");
	ExpectDefaultResult(offset, "zero-radius target should not match offset point");
}

void TestOverlappingTargetsChooseNearest()
{
	const iggy::InteractionTarget2D farther = Target("target:farther", { 0.0F, 0.0F }, 10.0F);
	const iggy::InteractionTarget2D nearer = Target("target:nearer", { 2.0F, 0.0F }, 10.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ farther, nearer });

	const iggy::InteractionTargetSpatialQuery2DResult result =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { 3.0F, 0.0F });

	Expect(result.status == iggy::InteractionTargetSpatialQuery2DStatus::Found, "overlapping targets should find a target");
	Expect(result.targetId == nearer.id, "overlapping targets should choose nearest target");
	Expect(result.targetIndex == 1, "nearest target index should be reported");
	Expect(Near(result.distance, 1.0F), "nearest target distance should be reported");
}

void TestExactDistanceTieChoosesFirstRegistryTarget()
{
	const iggy::InteractionTarget2D first = Target("target:first", { -1.0F, 0.0F }, 2.0F);
	const iggy::InteractionTarget2D second = Target("target:second", { 1.0F, 0.0F }, 2.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ first, second });

	const iggy::InteractionTargetSpatialQuery2DResult result =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { 0.0F, 0.0F });

	Expect(result.status == iggy::InteractionTargetSpatialQuery2DStatus::Found, "tie query should find a target");
	Expect(result.targetId == first.id, "exact distance tie should preserve first registry target");
	Expect(result.targetIndex == 0, "exact distance tie should preserve first target index");
}

void TestDisabledTargetsAreIgnored()
{
	const iggy::InteractionTarget2D disabledNear =
		Target("target:disabled", { 0.0F, 0.0F }, 5.0F, false);
	const iggy::InteractionTarget2D enabledFar =
		Target("target:enabled", { 3.0F, 0.0F }, 5.0F, true);
	const iggy::InteractionTarget2DRegistry mixedRegistry = Registry({ disabledNear, enabledFar });
	const iggy::InteractionTarget2DRegistry disabledOnlyRegistry = Registry({ disabledNear });

	const iggy::InteractionTargetSpatialQuery2DResult mixedResult =
		iggy::InteractionTargetSpatialQuery2D {}.find(mixedRegistry, { 0.0F, 0.0F });
	const iggy::InteractionTargetSpatialQuery2DResult disabledOnlyResult =
		iggy::InteractionTargetSpatialQuery2D {}.find(disabledOnlyRegistry, { 0.0F, 0.0F });

	Expect(mixedResult.status == iggy::InteractionTargetSpatialQuery2DStatus::Found, "enabled farther target should be found when disabled nearer target is ignored");
	Expect(mixedResult.targetId == enabledFar.id, "disabled nearer target should not win spatial query");
	Expect(mixedResult.targetIndex == 1, "enabled farther target should report original registry index");
	ExpectDefaultResult(disabledOnlyResult, "only disabled in-radius targets should return NotFound");
}

void TestExtraRadiusExpandsDistanceAndNegativeExtraClampsToZero()
{
	const iggy::InteractionTarget2D target = Target("target:expand", { 0.0F, 0.0F }, 1.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });

	const iggy::InteractionTargetSpatialQuery2DResult expanded =
		iggy::InteractionTargetSpatialQuery2D {}.find(
			registry,
			{ 2.0F, 0.0F },
			{ 1.0F });
	const iggy::InteractionTargetSpatialQuery2DResult negative =
		iggy::InteractionTargetSpatialQuery2D {}.find(
			registry,
			{ 1.5F, 0.0F },
			{ -10.0F });

	Expect(expanded.status == iggy::InteractionTargetSpatialQuery2DStatus::Found, "extra radius should expand matching distance");
	Expect(expanded.targetId == target.id, "expanded query should return target");
	Expect(Near(expanded.distance, 2.0F), "expanded query should report distance");
	Expect(Near(expanded.allowedDistance, 2.0F), "expanded query should include positive extra radius");
	ExpectDefaultResult(negative, "negative extra radius should clamp to zero and not expand match distance");
}

void TestFindTileCenterMatchesPointQueryAtTileCenter()
{
	const iggy::TileCoord tile { 4, -3 };
	const iggy::Vec2 center = iggy::tileCenter(tile);
	const iggy::InteractionTarget2D target = Target("target:tile", center, 0.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ target });

	const iggy::InteractionTargetSpatialQuery2DResult tileResult =
		iggy::InteractionTargetSpatialQuery2D {}.findTileCenter(registry, tile);
	const iggy::InteractionTargetSpatialQuery2DResult pointResult =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, center);

	Expect(tileResult.status == pointResult.status, "tile-center query should match point query status");
	Expect(tileResult.targetId == pointResult.targetId, "tile-center query should match point query target id");
	Expect(tileResult.targetIndex == pointResult.targetIndex, "tile-center query should match point query index");
	Expect(Near(tileResult.distance, pointResult.distance), "tile-center query should match point query distance");
	Expect(Near(tileResult.allowedDistance, pointResult.allowedDistance), "tile-center query should match point query allowed distance");
}

void TestQueryDoesNotMutateRegistry()
{
	const iggy::InteractionTarget2D first = Target("target:first", { 1.0F, 2.0F }, 3.0F);
	const iggy::InteractionTarget2D second = Target("target:second", { 3.0F, 4.0F }, 5.0F);
	const iggy::InteractionTarget2DRegistry registry = Registry({ first, second });
	const std::vector<iggy::InteractionTarget2D> before = registry.targets();

	const iggy::InteractionTargetSpatialQuery2DResult result =
		iggy::InteractionTargetSpatialQuery2D {}.find(registry, { 1.0F, 2.0F });

	Expect(result.status == iggy::InteractionTargetSpatialQuery2DStatus::Found, "immutability setup should find target");
	Expect(registry.targets().size() == before.size(), "spatial query should not mutate registry target count");
	if (registry.targets().size() == before.size()) {
		ExpectTarget(registry.targets()[0], before[0], "spatial query should not mutate first target payload");
		ExpectTarget(registry.targets()[1], before[1], "spatial query should not mutate second target payload");
	}
}

} // namespace

int main()
{
	TestEmptyRegistryReturnsNotFound();
	TestPointInsideOneEnabledTargetRadiusReturnsFound();
	TestPointAtExactRadiusBoundaryReturnsFound();
	TestPointOutsideAllRadiiReturnsNotFound();
	TestZeroRadiusTargetMatchesOnlySamePosition();
	TestOverlappingTargetsChooseNearest();
	TestExactDistanceTieChoosesFirstRegistryTarget();
	TestDisabledTargetsAreIgnored();
	TestExtraRadiusExpandsDistanceAndNegativeExtraClampsToZero();
	TestFindTileCenterMatchesPointQueryAtTileCenter();
	TestQueryDoesNotMutateRegistry();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
