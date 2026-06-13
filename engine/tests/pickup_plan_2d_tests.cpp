#include <cstdlib>
#include <vector>

#include "scene/inventory/PickupPlan2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

iggy::LevelItemDrop2D Drop(
	const char *id,
	const char *itemId = "item:potion",
	std::uint32_t count = 1,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float pickupRadius = 0.0F,
	bool enabled = true)
{
	return {
		iggy::ResourceId { id },
		iggy::ResourceId { itemId },
		count,
		position,
		pickupRadius,
		enabled,
	};
}

iggy::LevelItemDrop2DRegistry Registry(std::vector<iggy::LevelItemDrop2D> drops)
{
	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);
	Expect(result.built, "pickup plan drop registry fixture should build");
	return result.registry;
}

void ExpectDrop(const iggy::LevelItemDrop2D &actual, const iggy::LevelItemDrop2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.itemId == expected.itemId, message);
	Expect(actual.count == expected.count, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.pickupRadius == expected.pickupRadius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void TestMissingEmptyDropId()
{
	const iggy::Vec2 actorPosition { 1.0F, 2.0F };

	const iggy::PickupPlan2DResult result = iggy::PickupPlan2D {}.plan(Registry({}), {}, actorPosition);

	Expect(result.status == iggy::PickupPlan2DStatus::MissingDropId, "empty pickup drop id should return MissingDropId");
	Expect(!result.ready(), "empty pickup drop id should not be ready");
	Expect(result.dropId.empty(), "empty pickup drop id result should preserve requested empty id");
	Expect(NearVec(result.actorPosition, actorPosition), "empty pickup drop id result should preserve actor position");
	Expect(result.drop.id.empty() && result.drop.itemId.empty(), "empty pickup drop id result should have no drop payload");
	Expect(Near(result.distance, 0.0F), "empty pickup drop id should not compute distance");
	Expect(Near(result.allowedDistance, 0.0F), "empty pickup drop id should not compute allowed distance");
}

void TestMissingNonEmptyDropId()
{
	const iggy::Vec2 actorPosition { -1.0F, 3.0F };

	const iggy::PickupPlan2DResult result =
		iggy::PickupPlan2D {}.plan(Registry({}), iggy::ResourceId { "drop:missing" }, actorPosition);

	Expect(result.status == iggy::PickupPlan2DStatus::DropNotFound, "missing pickup drop id should return DropNotFound");
	Expect(!result.ready(), "missing pickup drop id should not be ready");
	Expect(result.dropId == iggy::ResourceId { "drop:missing" }, "missing pickup drop id result should preserve requested id");
	Expect(NearVec(result.actorPosition, actorPosition), "missing pickup drop id result should preserve actor position");
	Expect(result.drop.id.empty() && result.drop.itemId.empty(), "missing pickup drop id result should have no drop payload");
	Expect(Near(result.distance, 0.0F), "missing pickup drop id should not compute distance");
	Expect(Near(result.allowedDistance, 0.0F), "missing pickup drop id should not compute allowed distance");
}

void TestDisabledDrop()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:disabled", "item:key", 1, { 4.0F, 5.0F }, 2.0F, false);

	const iggy::PickupPlan2DResult result =
		iggy::PickupPlan2D {}.plan(Registry({ drop }), drop.id, { 4.0F, 5.0F }, { 99.0F });

	Expect(result.status == iggy::PickupPlan2DStatus::DropDisabled, "disabled pickup drop should return DropDisabled");
	Expect(!result.ready(), "disabled pickup drop should not be ready");
	ExpectDrop(result.drop, drop, "disabled pickup drop result should preserve copied drop");
	Expect(Near(result.distance, 0.0F), "disabled pickup drop should not compute distance");
	Expect(Near(result.allowedDistance, 0.0F), "disabled pickup drop should not compute allowed distance");
}

void TestReadyAtSamePositionWithRadiusZero()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:same", "item:potion", 1, { 2.0F, 3.0F }, 0.0F);

	const iggy::PickupPlan2DResult result = iggy::PickupPlan2D {}.plan(Registry({ drop }), drop.id, { 2.0F, 3.0F });

	Expect(result.status == iggy::PickupPlan2DStatus::Ready, "same-position zero-radius pickup should be ready");
	Expect(result.ready(), "same-position zero-radius pickup should report ready");
	Expect(Near(result.distance, 0.0F), "same-position zero-radius pickup should compute zero distance");
	Expect(Near(result.allowedDistance, 0.0F), "same-position zero-radius pickup should allow zero distance");
	ExpectDrop(result.drop, drop, "same-position pickup should preserve drop payload");
}

void TestReadyInsideRadius()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:inside", "item:potion", 1, { 3.0F, 4.0F }, 5.0F);

	const iggy::PickupPlan2DResult result = iggy::PickupPlan2D {}.plan(Registry({ drop }), drop.id, { 0.0F, 0.0F });

	Expect(result.status == iggy::PickupPlan2DStatus::Ready, "pickup inside radius should be ready");
	Expect(result.ready(), "inside radius pickup should report ready");
	Expect(Near(result.distance, 5.0F), "inside radius pickup should compute Euclidean distance");
	Expect(Near(result.allowedDistance, 5.0F), "inside radius pickup should use pickup radius as allowed distance");
}

void TestReadyExactlyAtBoundary()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:boundary", "item:potion", 1, { 6.0F, 8.0F }, 10.0F);

	const iggy::PickupPlan2DResult result = iggy::PickupPlan2D {}.plan(Registry({ drop }), drop.id, { 0.0F, 0.0F });

	Expect(result.status == iggy::PickupPlan2DStatus::Ready, "pickup exactly at radius boundary should be ready");
	Expect(result.ready(), "boundary pickup should report ready");
	Expect(Near(result.distance, 10.0F), "boundary pickup should compute boundary distance");
	Expect(Near(result.allowedDistance, 10.0F), "boundary pickup should compute allowed distance");
}

void TestOutOfRangeOutsideRadius()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:far", "item:potion", 1, { 4.0F, 0.0F }, 3.0F);

	const iggy::PickupPlan2DResult result = iggy::PickupPlan2D {}.plan(Registry({ drop }), drop.id, { 0.0F, 0.0F });

	Expect(result.status == iggy::PickupPlan2DStatus::OutOfRange, "pickup outside radius should be out of range");
	Expect(!result.ready(), "out-of-range pickup should not report ready");
	Expect(Near(result.distance, 4.0F), "out-of-range pickup should compute Euclidean distance");
	Expect(Near(result.allowedDistance, 3.0F), "out-of-range pickup should compute allowed distance");
}

void TestExtraReachMakesOutOfRangeDropReady()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:extra", "item:potion", 1, { 4.0F, 0.0F }, 2.5F);

	const iggy::PickupPlan2DResult result =
		iggy::PickupPlan2D {}.plan(Registry({ drop }), drop.id, { 0.0F, 0.0F }, { 1.5F });

	Expect(result.status == iggy::PickupPlan2DStatus::Ready, "extra reach should make pickup ready");
	Expect(result.ready(), "extra reach pickup should report ready");
	Expect(Near(result.distance, 4.0F), "extra reach pickup should compute distance");
	Expect(Near(result.allowedDistance, 4.0F), "extra reach pickup should add extra reach to radius");
}

void TestNegativeExtraReachClampsToZero()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:negative_extra", "item:potion", 1, { 4.0F, 0.0F }, 3.0F);

	const iggy::PickupPlan2DResult result =
		iggy::PickupPlan2D {}.plan(Registry({ drop }), drop.id, { 0.0F, 0.0F }, { -10.0F });

	Expect(result.status == iggy::PickupPlan2DStatus::OutOfRange, "negative extra reach should clamp to zero");
	Expect(!result.ready(), "negative extra reach should not expand pickup range");
	Expect(Near(result.distance, 4.0F), "negative extra reach pickup should compute distance");
	Expect(Near(result.allowedDistance, 3.0F), "negative extra reach pickup should use radius only");
}

void TestNegativePickupRadiusClampsToZeroDefensively()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:negative_radius", "item:potion", 1, { 1.0F, 0.0F }, -5.0F);
	iggy::LevelItemDrop2DRegistry registry;
	registry.drops = { drop };

	const iggy::PickupPlan2DResult result = iggy::PickupPlan2D {}.plan(registry, drop.id, { 1.0F, 0.0F });

	Expect(result.status == iggy::PickupPlan2DStatus::Ready, "negative pickup radius should clamp to zero defensively");
	Expect(result.ready(), "negative pickup radius at same position should be ready after clamp");
	Expect(Near(result.distance, 0.0F), "negative pickup radius pickup should compute distance");
	Expect(Near(result.allowedDistance, 0.0F), "negative pickup radius pickup should clamp allowed distance to zero");
	ExpectDrop(result.drop, drop, "negative pickup radius result should preserve original drop payload");
}

void TestPreservesDropActorAndDistanceData()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:preserve", "item:gem", 7, { -3.0F, -4.0F }, 5.0F, true);
	const iggy::Vec2 actorPosition { 1.25F, -2.5F };

	const iggy::PickupPlan2DResult result = iggy::PickupPlan2D {}.plan(Registry({ drop }), drop.id, actorPosition, { 2.0F });

	Expect(result.dropId == drop.id, "pickup plan should preserve requested drop id");
	Expect(NearVec(result.actorPosition, actorPosition), "pickup plan should preserve actor position exactly");
	ExpectDrop(result.drop, drop, "pickup plan should preserve drop payload exactly");
	Expect(result.distance > 0.0F, "pickup plan should compute a positive distance for separated points");
	Expect(Near(result.allowedDistance, 7.0F), "pickup plan should preserve radius plus extra reach math");
}

void TestNamespacedAndUnqualifiedDropIdsRemainDistinct()
{
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("potion", "item:potion", 1),
		Drop("drop:potion", "item:potion", 1, { 2.0F, 0.0F }, 3.0F),
	};
	const iggy::LevelItemDrop2DRegistry registry = Registry(drops);

	const iggy::PickupPlan2DResult unqualified = iggy::PickupPlan2D {}.plan(registry, iggy::ResourceId { "potion" }, { 0.0F, 0.0F });
	const iggy::PickupPlan2DResult namespaced = iggy::PickupPlan2D {}.plan(registry, iggy::ResourceId { "drop:potion" }, { 0.0F, 0.0F });

	Expect(unqualified.status == iggy::PickupPlan2DStatus::Ready, "unqualified drop id should resolve distinctly");
	Expect(namespaced.status == iggy::PickupPlan2DStatus::Ready, "namespaced drop id should resolve distinctly");
	Expect(unqualified.drop.id == iggy::ResourceId { "potion" }, "unqualified pickup plan should preserve unqualified drop id");
	Expect(namespaced.drop.id == iggy::ResourceId { "drop:potion" }, "namespaced pickup plan should preserve namespaced drop id");
	Expect(unqualified.drop.id != namespaced.drop.id, "pickup plans should resolve distinct drop payloads");
}

} // namespace

int main()
{
	TestMissingEmptyDropId();
	TestMissingNonEmptyDropId();
	TestDisabledDrop();
	TestReadyAtSamePositionWithRadiusZero();
	TestReadyInsideRadius();
	TestReadyExactlyAtBoundary();
	TestOutOfRangeOutsideRadius();
	TestExtraReachMakesOutOfRangeDropReady();
	TestNegativeExtraReachClampsToZero();
	TestNegativePickupRadiusClampsToZeroDefensively();
	TestPreservesDropActorAndDistanceData();
	TestNamespacedAndUnqualifiedDropIdsRemainDistinct();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
