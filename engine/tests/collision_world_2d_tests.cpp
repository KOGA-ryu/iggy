#include <cstdlib>
#include <vector>

#include "servers/physics2d/CollisionWorld2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

void ExpectBounds(iggy::Aabb2 actual, iggy::Aabb2 expected, const char *message)
{
	Expect(NearVec(actual.min, expected.min) && NearVec(actual.max, expected.max), message);
}

void ExpectObject(const iggy::physics2d::CollisionObject2D &actual, const iggy::physics2d::CollisionObject2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.shape.type == expected.shape.type, message);
	ExpectBounds(actual.shape.bounds, expected.shape.bounds, message);
	Expect(actual.solid == expected.solid, message);
}

iggy::physics2d::CollisionObject2D Object(
	iggy::ResourceId id,
	iggy::Aabb2 bounds,
	bool solid = true)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), solid };
}

void TestEmptyInputBuildsEmptyWorld()
{
	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build({});

	Expect(result.built, "empty collision world input should build successfully");
	Expect(result.world.objects().empty(), "empty collision world should have no objects");
	Expect(result.issues.empty(), "empty collision world input should produce no issues");
}

void TestOneValidAabbObjectBuildsAndPreservesFields()
{
	const iggy::physics2d::CollisionObject2D object = Object(
		iggy::ResourceId("player"),
		{ { 1.0F, 2.0F }, { 3.0F, 4.0F } },
		false);

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build({ object });

	Expect(result.built, "one valid object should build a collision world");
	Expect(result.issues.empty(), "one valid object should produce no issues");
	Expect(result.world.objects().size() == 1, "one valid object should produce one world object");
	ExpectObject(result.world.objects()[0], object, "world object should preserve id, shape, bounds, and solid flag");
}

void TestMultipleValidObjectsPreserveInputOrder()
{
	const std::vector<iggy::physics2d::CollisionObject2D> objects {
		Object(iggy::ResourceId("first"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
		Object(iggy::ResourceId("second"), { { 2.0F, 2.0F }, { 3.0F, 3.0F } }, false),
		Object(iggy::ResourceId("third"), { { -3.0F, -2.0F }, { -1.0F, 0.0F } }),
	};

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);

	Expect(result.built, "multiple valid objects should build");
	Expect(result.world.objects().size() == objects.size(), "all valid objects should be stored");
	for (std::size_t index = 0; index < objects.size(); ++index)
		ExpectObject(result.world.objects()[index], objects[index], "world objects should preserve input order");
}

void TestEmptyIdIsAllowedAndPreserved()
{
	const iggy::physics2d::CollisionObject2D object = Object({}, { { 0.0F, 0.0F }, { 1.0F, 1.0F } });

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build({ object });

	Expect(result.built, "empty object id should be allowed");
	Expect(result.world.objects().size() == 1, "empty id object should be stored");
	Expect(result.world.objects()[0].id.empty(), "empty id should be preserved");
}

void TestDuplicateIdsAreAllowedAndPreserved()
{
	const iggy::ResourceId duplicateId("door");
	const std::vector<iggy::physics2d::CollisionObject2D> objects {
		Object(duplicateId, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
		Object(duplicateId, { { 2.0F, 0.0F }, { 3.0F, 1.0F } }),
	};

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);

	Expect(result.built, "duplicate object ids should be allowed");
	Expect(result.world.objects().size() == 2, "duplicate id objects should both be stored");
	Expect(result.world.objects()[0].id == duplicateId && result.world.objects()[1].id == duplicateId, "duplicate ids should be preserved");
}

void TestSolidFalseIsMetadataAndPreserved()
{
	const iggy::physics2d::CollisionObject2D object = Object(
		iggy::ResourceId("trigger"),
		{ { 4.0F, 5.0F }, { 6.0F, 7.0F } },
		false);

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build({ object });

	Expect(result.built, "non-solid object should still build");
	Expect(result.world.objects()[0].solid == false, "solid=false should be preserved as metadata");
}

void TestInvalidInvertedShapeReportsIssueWithOriginalIndexAndObject()
{
	const iggy::physics2d::CollisionObject2D valid = Object(
		iggy::ResourceId("valid"),
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } });
	const iggy::physics2d::CollisionObject2D invalid {
		iggy::ResourceId("bad"),
		iggy::physics2d::makeAabbShape({ { 4.0F, 0.0F }, { 2.0F, 1.0F } }),
		false,
	};

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build({ valid, invalid });

	Expect(!result.built, "invalid shape should fail world build");
	Expect(result.world.objects().empty(), "invalid shape should leave world empty");
	Expect(result.issues.size() == 1, "invalid shape should produce one issue");
	Expect(result.issues[0].code == iggy::physics2d::CollisionWorldBuildIssueCode::InvalidShape, "issue code should be InvalidShape");
	Expect(result.issues[0].index == 1, "issue index should preserve original invalid object index");
	ExpectObject(result.issues[0].object, invalid, "issue should preserve offending object copy");
}

void TestMultipleInvalidShapesReportInInputOrder()
{
	const iggy::physics2d::CollisionObject2D invalidX {
		iggy::ResourceId("invalid-x"),
		iggy::physics2d::makeAabbShape({ { 3.0F, 0.0F }, { 2.0F, 1.0F } }),
		true,
	};
	const iggy::physics2d::CollisionObject2D valid = Object(
		iggy::ResourceId("valid"),
		{ { 10.0F, 10.0F }, { 12.0F, 12.0F } });
	const iggy::physics2d::CollisionObject2D invalidY {
		iggy::ResourceId("invalid-y"),
		iggy::physics2d::makeAabbShape({ { 0.0F, 5.0F }, { 1.0F, 4.0F } }),
		false,
	};
	iggy::physics2d::CollisionObject2D unknown;
	unknown.id = iggy::ResourceId("unknown");

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build({ invalidX, valid, invalidY, unknown });

	Expect(!result.built, "multiple invalid shapes should fail world build");
	Expect(result.world.objects().empty(), "failed build should publish an empty world");
	Expect(result.issues.size() == 3, "multiple invalid shapes should report all issues");
	Expect(result.issues[0].index == 0, "first invalid issue should preserve input index");
	Expect(result.issues[1].index == 2, "second invalid issue should preserve input index");
	Expect(result.issues[2].index == 3, "third invalid issue should preserve input index");
	ExpectObject(result.issues[0].object, invalidX, "first invalid issue should preserve offending object");
	ExpectObject(result.issues[1].object, invalidY, "second invalid issue should preserve offending object");
	ExpectObject(result.issues[2].object, unknown, "unknown shape issue should preserve offending object");
}

void TestAnyInvalidShapePreventsPartialWorld()
{
	const std::vector<iggy::physics2d::CollisionObject2D> objects {
		Object(iggy::ResourceId("valid-a"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
		{ iggy::ResourceId("invalid"), iggy::physics2d::makeAabbShape({ { 2.0F, 3.0F }, { 1.0F, 4.0F } }), true },
		Object(iggy::ResourceId("valid-b"), { { 5.0F, 5.0F }, { 6.0F, 6.0F } }),
	};

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);

	Expect(!result.built, "any invalid shape should fail the whole build");
	Expect(result.world.objects().empty(), "failed build should not publish a partial valid subset");
}

void TestUnknownDefaultShapeIsReportedInvalid()
{
	iggy::physics2d::CollisionObject2D object;
	object.id = iggy::ResourceId("unknown-shape");

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build({ object });

	Expect(!result.built, "unknown default shape should fail world build");
	Expect(result.issues.size() == 1, "unknown default shape should report one invalid shape issue");
	Expect(result.issues[0].index == 0, "unknown default shape issue should preserve original index");
	ExpectObject(result.issues[0].object, object, "unknown default shape issue should preserve object copy");
}

void TestInputObjectsAreNotMutated()
{
	std::vector<iggy::physics2d::CollisionObject2D> objects {
		Object(iggy::ResourceId("first"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
		Object(iggy::ResourceId("second"), { { -2.0F, -3.0F }, { -1.0F, -2.0F } }, false),
	};
	const std::vector<iggy::physics2d::CollisionObject2D> before = objects;

	const iggy::physics2d::CollisionWorldBuildResult result = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);

	Expect(result.built, "valid input should build while checking mutation");
	Expect(objects.size() == before.size(), "input vector size should not change");
	for (std::size_t index = 0; index < objects.size(); ++index)
		ExpectObject(objects[index], before[index], "input objects should not be mutated");
}

} // namespace

int main()
{
	TestEmptyInputBuildsEmptyWorld();
	TestOneValidAabbObjectBuildsAndPreservesFields();
	TestMultipleValidObjectsPreserveInputOrder();
	TestEmptyIdIsAllowedAndPreserved();
	TestDuplicateIdsAreAllowedAndPreserved();
	TestSolidFalseIsMetadataAndPreserved();
	TestInvalidInvertedShapeReportsIssueWithOriginalIndexAndObject();
	TestMultipleInvalidShapesReportInInputOrder();
	TestAnyInvalidShapePreventsPartialWorld();
	TestUnknownDefaultShapeIsReportedInvalid();
	TestInputObjectsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
