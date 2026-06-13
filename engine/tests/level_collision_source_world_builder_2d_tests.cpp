#include <cstdlib>
#include <vector>

#include "scene/level/LevelCollisionSourceWorldBuilder2D.hpp"
#include "servers/physics2d/CollisionOverlap2D.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelCollisionSourceBox2D Box(
	const char *wallId,
	std::size_t wallIndex,
	iggy::Vec2 center,
	iggy::Vec2 size,
	float rotationRadians = 0.0F,
	const char *assetId = "asset:wall",
	const char *definitionId = "definition:wall")
{
	return {
		Id(wallId),
		wallIndex,
		center,
		size,
		rotationRadians,
		Id(assetId),
		Id(definitionId),
	};
}

iggy::LevelCollisionSource2D Source(std::vector<iggy::LevelCollisionSourceBox2D> boxes)
{
	return { boxes };
}

iggy::Aabb2 BoundsFor(iggy::Vec2 center, iggy::Vec2 size)
{
	const iggy::Vec2 half { size.x * 0.5F, size.y * 0.5F };
	return {
		{ center.x - half.x, center.y - half.y },
		{ center.x + half.x, center.y + half.y },
	};
}

void ExpectObject(
	const iggy::physics2d::CollisionObject2D &actual,
	const iggy::LevelCollisionSourceBox2D &box,
	const char *message)
{
	Expect(actual.id == box.sourceWallId, message);
	Expect(actual.solid, message);
	Expect(actual.shape.type == iggy::physics2d::CollisionShape2DType::Aabb, message);
	ExpectBounds(actual.shape.bounds, BoundsFor(box.center, box.size), message);
}

bool SameSource(
	const iggy::LevelCollisionSource2D &actual,
	const iggy::LevelCollisionSource2D &expected)
{
	if (actual.boxes.size() != expected.boxes.size())
		return false;

	for (std::size_t index = 0; index < actual.boxes.size(); ++index) {
		const iggy::LevelCollisionSourceBox2D &left = actual.boxes[index];
		const iggy::LevelCollisionSourceBox2D &right = expected.boxes[index];
		if (left.sourceWallId != right.sourceWallId
			|| left.sourceWallIndex != right.sourceWallIndex
			|| !NearVec(left.center, right.center)
			|| !NearVec(left.size, right.size)
			|| left.rotationRadians != right.rotationRadians
			|| left.assetId != right.assetId
			|| left.definitionId != right.definitionId) {
			return false;
		}
	}

	return true;
}

void TestEmptySourceBuildsEmptyWorld()
{
	const iggy::LevelCollisionSource2D source;

	const iggy::LevelCollisionSourceWorldBuilder2DResult result =
		iggy::LevelCollisionSourceWorldBuilder2D {}.build(source);

	Expect(result.built, "empty collision source should build an empty world");
	Expect(result.world.objects().empty(), "empty collision source should produce no world objects");
	Expect(result.issues.empty(), "empty collision source should produce no issues");
	Expect(result.sourceBoxCount == 0, "empty collision source should preserve source box count");
	Expect(result.generatedObjectCount == 0, "empty collision source should report no generated objects");
}

void TestOneBoxCreatesCollidableWorldObject()
{
	const iggy::LevelCollisionSource2D source = Source({
		Box("wall:a", 3, { 2.0F, 3.0F }, { 4.0F, 2.0F }),
	});

	const iggy::LevelCollisionSourceWorldBuilder2DResult result =
		iggy::LevelCollisionSourceWorldBuilder2D {}.build(source);

	Expect(result.built, "one valid source box should build collision world");
	Expect(result.world.objects().size() == 1, "one valid source box should create one world object");
	Expect(result.generatedObjectCount == 1, "one valid source box should count one generated object");
	if (result.world.objects().size() == 1)
		ExpectObject(result.world.objects()[0], source.boxes[0], "world object should preserve source wall id and bounds");

	const iggy::physics2d::CollisionOverlap2DResult overlap = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		result.world,
		{ { 1.5F, 2.5F }, { 2.5F, 3.5F } });
	Expect(overlap.hits.size() == 1, "source-built world should be observable through overlap query");
	if (overlap.hits.size() == 1)
		Expect(overlap.hits[0].object.id == Id("wall:a"), "overlap hit should preserve source wall id");
}

void TestMultipleBoxesPreserveWorldOrderAndCollisionBehavior()
{
	const iggy::LevelCollisionSource2D source = Source({
		Box("wall:first", 0, { 0.0F, 0.0F }, { 2.0F, 2.0F }),
		Box("wall:second", 1, { 4.0F, 0.0F }, { 2.0F, 2.0F }),
		Box("wall:third", 2, { 8.0F, 0.0F }, { 2.0F, 2.0F }),
	});

	const iggy::LevelCollisionSourceWorldBuilder2DResult result =
		iggy::LevelCollisionSourceWorldBuilder2D {}.build(source);

	Expect(result.built, "multiple valid source boxes should build");
	Expect(result.world.objects().size() == 3, "multiple valid source boxes should create one object each");
	if (result.world.objects().size() == 3) {
		ExpectObject(result.world.objects()[0], source.boxes[0], "first world object should preserve source order");
		ExpectObject(result.world.objects()[1], source.boxes[1], "second world object should preserve source order");
		ExpectObject(result.world.objects()[2], source.boxes[2], "third world object should preserve source order");
	}

	const iggy::physics2d::CollisionOverlap2DResult overlap = iggy::physics2d::CollisionOverlap2D {}.queryAabb(
		result.world,
		{ { 3.5F, -0.5F }, { 4.5F, 0.5F } });
	Expect(overlap.hits.size() == 1, "query near second box should hit exactly second object");
	if (overlap.hits.size() == 1) {
		Expect(overlap.hits[0].objectIndex == 1, "query near second box should preserve world object index");
		Expect(overlap.hits[0].object.id == Id("wall:second"), "query near second box should preserve object id");
	}
}

void TestRepeatedSameSourceWallIndexGetsUniqueCollisionObjectIds()
{
	const iggy::LevelCollisionSource2D source = Source({
		Box("wall:split", 4, { -3.0F, 0.0F }, { 4.0F, 2.0F }),
		Box("wall:split", 4, { 3.0F, 0.0F }, { 4.0F, 2.0F }),
	});

	const iggy::LevelCollisionSourceWorldBuilder2DResult result =
		iggy::LevelCollisionSourceWorldBuilder2D {}.build(source);

	Expect(result.built, "split source wall boxes should build");
	Expect(result.world.objects().size() == 2, "split source wall boxes should produce two objects");
	if (result.world.objects().size() == 2) {
		Expect(result.world.objects()[0].id != result.world.objects()[1].id, "split source wall objects should get unique generated ids");
		Expect(!result.world.objects()[0].id.empty() && !result.world.objects()[1].id.empty(), "split source wall generated ids should be non-empty");
		Expect(result.world.objects()[0].solid && result.world.objects()[1].solid, "split source wall objects should remain solid");
		Expect(result.world.objects()[0].shape.type == iggy::physics2d::CollisionShape2DType::Aabb, "first split source wall object should be AABB");
		Expect(result.world.objects()[1].shape.type == iggy::physics2d::CollisionShape2DType::Aabb, "second split source wall object should be AABB");
		ExpectBounds(result.world.objects()[0].shape.bounds, BoundsFor(source.boxes[0].center, source.boxes[0].size), "first split source wall object should preserve bounds");
		ExpectBounds(result.world.objects()[1].shape.bounds, BoundsFor(source.boxes[1].center, source.boxes[1].size), "second split source wall object should preserve bounds");
	}
}

void TestNonPositiveAndUnsupportedBoxesReportIssuesAndCreateNoShapes()
{
	const iggy::LevelCollisionSource2D source = Source({
		Box("wall:zero", 0, { 0.0F, 0.0F }, { 0.0F, 2.0F }),
		Box("wall:rotated", 1, { 4.0F, 0.0F }, { 2.0F, 2.0F }, 0.25F),
	});

	const iggy::LevelCollisionSourceWorldBuilder2DResult result =
		iggy::LevelCollisionSourceWorldBuilder2D {}.build(source);

	Expect(result.built, "invalid-only source should still build empty world from valid subset");
	Expect(result.world.objects().empty(), "invalid-only source should create no shapes");
	Expect(result.generatedObjectCount == 0, "invalid-only source should generate no objects");
	Expect(result.issues.size() == 2, "invalid-only source should report both issues");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::LevelCollisionSourceWorldBuilder2DIssueCode::NonPositiveBoxSize, "first issue should be non-positive size");
		Expect(result.issues[0].boxIndex == 0, "first issue should preserve source box index");
		Expect(result.issues[1].code == iggy::LevelCollisionSourceWorldBuilder2DIssueCode::UnsupportedRotation, "second issue should be unsupported rotation");
		Expect(result.issues[1].boxIndex == 1, "second issue should preserve source box index");
	}
}

void TestMixedValidInvalidSourceBuildsPartialValidWorldAndPreservesIssueOrder()
{
	const iggy::LevelCollisionSource2D source = Source({
		Box("wall:valid_a", 0, { -4.0F, 0.0F }, { 2.0F, 2.0F }),
		Box("wall:zero", 1, { 0.0F, 0.0F }, { 2.0F, 0.0F }),
		Box("wall:rotated", 2, { 4.0F, 0.0F }, { 2.0F, 2.0F }, 0.5F),
		Box("wall:valid_b", 3, { 8.0F, 0.0F }, { 4.0F, 2.0F }),
	});

	const iggy::LevelCollisionSourceWorldBuilder2DResult result =
		iggy::LevelCollisionSourceWorldBuilder2D {}.build(source);

	Expect(result.built, "mixed source should build partial valid world");
	Expect(result.world.objects().size() == 2, "mixed source should build only valid source boxes");
	Expect(result.generatedObjectCount == 2, "mixed source should count generated valid objects");
	Expect(result.issues.size() == 2, "mixed source should preserve invalid issue count");
	if (result.world.objects().size() == 2) {
		Expect(result.world.objects()[0].id == Id("wall:valid_a"), "first valid object should preserve valid source order");
		Expect(result.world.objects()[1].id == Id("wall:valid_b"), "second valid object should preserve valid source order");
	}
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::LevelCollisionSourceWorldBuilder2DIssueCode::NonPositiveBoxSize, "mixed first issue should be non-positive");
		Expect(result.issues[0].boxIndex == 1, "mixed first issue should preserve source index");
		Expect(result.issues[1].code == iggy::LevelCollisionSourceWorldBuilder2DIssueCode::UnsupportedRotation, "mixed second issue should be unsupported rotation");
		Expect(result.issues[1].boxIndex == 2, "mixed second issue should preserve source index");
	}
}

void TestInputSourceIsNotMutated()
{
	iggy::LevelCollisionSource2D source = Source({
		Box("wall:valid", 0, { 0.0F, 0.0F }, { 2.0F, 2.0F }),
		Box("wall:invalid", 1, { 4.0F, 0.0F }, { 0.0F, 2.0F }),
	});
	const iggy::LevelCollisionSource2D before = source;

	const iggy::LevelCollisionSourceWorldBuilder2DResult result =
		iggy::LevelCollisionSourceWorldBuilder2D {}.build(source);

	Expect(result.built, "immutability source should build partial valid world");
	Expect(SameSource(source, before), "source world builder should not mutate input source");
}

} // namespace

int main()
{
	TestEmptySourceBuildsEmptyWorld();
	TestOneBoxCreatesCollidableWorldObject();
	TestMultipleBoxesPreserveWorldOrderAndCollisionBehavior();
	TestRepeatedSameSourceWallIndexGetsUniqueCollisionObjectIds();
	TestNonPositiveAndUnsupportedBoxesReportIssuesAndCreateNoShapes();
	TestMixedValidInvalidSourceBuildsPartialValidWorldAndPreservesIssueOrder();
	TestInputSourceIsNotMutated();

	return Failures;
}
