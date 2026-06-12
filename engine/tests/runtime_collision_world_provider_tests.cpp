#include <cstdlib>
#include <vector>

#include "runtime/RuntimeCollisionWorldProvider.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::Failures;

iggy::physics2d::CollisionObject2D Object(
	iggy::ResourceId id,
	iggy::Aabb2 bounds,
	bool solid = true)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), solid };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "test fixture collision world should build");
	return build.world;
}

void ExpectObject(
	const iggy::physics2d::CollisionObject2D &actual,
	const iggy::physics2d::CollisionObject2D &expected,
	const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.solid == expected.solid, message);
	Expect(actual.shape.type == expected.shape.type, message);
	ExpectBounds(actual.shape.bounds, expected.shape.bounds, message);
}

void ExpectWorldMatches(
	const iggy::physics2d::CollisionWorld2D &actual,
	const iggy::physics2d::CollisionWorld2D &expected,
	const char *message)
{
	Expect(actual.objects().size() == expected.objects().size(), message);
	for (std::size_t index = 0; index < actual.objects().size() && index < expected.objects().size(); ++index)
		ExpectObject(actual.objects()[index], expected.objects()[index], message);
}

iggy::runtime::RuntimeSessionState SessionWithCollisionCache(const iggy::physics2d::CollisionWorld2D &world)
{
	iggy::runtime::RuntimeSessionState session;
	session.derivedCaches.hasCollisionCache = true;
	session.derivedCaches.collision.world = world;
	return session;
}

void TestNoExplicitWorldAndNoSessionCacheReturnsEmptyWorld()
{
	const iggy::runtime::RuntimeSessionState session;

	const iggy::runtime::RuntimeCollisionWorldResult result = iggy::runtime::RuntimeCollisionWorldProvider {}.resolve(session, {});

	Expect(result.source == iggy::runtime::RuntimeCollisionWorldSource::Empty, "no explicit world and no session cache should resolve to Empty");
	Expect(result.world.objects().empty(), "empty collision world resolution should return empty world");
}

void TestExplicitWorldReturnsCopiedExplicitWorld()
{
	const iggy::physics2d::CollisionObject2D object = Object(
		iggy::ResourceId("explicit:wall"),
		{ { 1.0F, 2.0F }, { 3.0F, 4.0F } },
		false);
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({ object });

	const iggy::runtime::RuntimeCollisionWorldResult result = iggy::runtime::RuntimeCollisionWorldProvider {}.resolve(
		{},
		{ &explicitWorld });

	Expect(result.source == iggy::runtime::RuntimeCollisionWorldSource::Explicit, "explicit world should resolve to Explicit source");
	ExpectWorldMatches(result.world, explicitWorld, "explicit world should be copied into result");
	if (result.world.objects().size() == 1)
		ExpectObject(result.world.objects()[0], object, "explicit world result should preserve id, solid flag, and bounds");
}

void TestSessionDerivedCollisionCacheReturnsCopiedSessionCache()
{
	const iggy::physics2d::CollisionObject2D object = Object(
		iggy::ResourceId("session:wall"),
		{ { -1.0F, 0.0F }, { 0.0F, 1.0F } });
	const iggy::physics2d::CollisionWorld2D cachedWorld = World({ object });
	const iggy::runtime::RuntimeSessionState session = SessionWithCollisionCache(cachedWorld);

	const iggy::runtime::RuntimeCollisionWorldResult result = iggy::runtime::RuntimeCollisionWorldProvider {}.resolve(session, {});

	Expect(result.source == iggy::runtime::RuntimeCollisionWorldSource::SessionCache, "session cache should resolve to SessionCache source");
	ExpectWorldMatches(result.world, cachedWorld, "session collision cache should be copied into result");
}

void TestExplicitWorldTakesPrecedenceOverSessionCache()
{
	const iggy::physics2d::CollisionWorld2D sessionWorld = World({
		Object(iggy::ResourceId("session:wall"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
	});
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({
		Object(iggy::ResourceId("explicit:wall"), { { 2.0F, 2.0F }, { 3.0F, 3.0F } }),
	});
	const iggy::runtime::RuntimeSessionState session = SessionWithCollisionCache(sessionWorld);

	const iggy::runtime::RuntimeCollisionWorldResult result = iggy::runtime::RuntimeCollisionWorldProvider {}.resolve(
		session,
		{ &explicitWorld });

	Expect(result.source == iggy::runtime::RuntimeCollisionWorldSource::Explicit, "explicit world should take precedence over session cache");
	ExpectWorldMatches(result.world, explicitWorld, "explicit precedence should copy explicit world");
	Expect(result.world.objects().size() == 1 && result.world.objects()[0].id == iggy::ResourceId("explicit:wall"), "explicit precedence should not return session cache object");
}

void TestSessionWithOnlyRenderCacheReturnsEmptyWorld()
{
	iggy::runtime::RuntimeSessionState session;
	session.hasRenderCache = true;
	session.derivedCaches.hasRenderCache = true;

	const iggy::runtime::RuntimeCollisionWorldResult result = iggy::runtime::RuntimeCollisionWorldProvider {}.resolve(session, {});

	Expect(result.source == iggy::runtime::RuntimeCollisionWorldSource::Empty, "session without collision cache should resolve to Empty even with render cache");
	Expect(result.world.objects().empty(), "session without collision cache should return empty world");
}

void TestInputsAreNotMutated()
{
	const iggy::physics2d::CollisionWorld2D sessionWorld = World({
		Object(iggy::ResourceId("session:wall"), { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
	});
	const iggy::physics2d::CollisionWorld2D explicitWorld = World({
		Object(iggy::ResourceId("explicit:wall"), { { 2.0F, 2.0F }, { 3.0F, 3.0F } }, false),
	});
	iggy::runtime::RuntimeSessionState session = SessionWithCollisionCache(sessionWorld);
	const iggy::runtime::RuntimeSessionState beforeSession = session;
	const std::vector<iggy::physics2d::CollisionObject2D> explicitObjects = explicitWorld.objects();

	const iggy::runtime::RuntimeCollisionWorldResult result = iggy::runtime::RuntimeCollisionWorldProvider {}.resolve(
		session,
		{ &explicitWorld });

	Expect(result.source == iggy::runtime::RuntimeCollisionWorldSource::Explicit, "immutability setup should resolve explicit world");
	Expect(session.derivedCaches.hasCollisionCache == beforeSession.derivedCaches.hasCollisionCache, "provider should not mutate session collision cache flag");
	ExpectWorldMatches(session.derivedCaches.collision.world, beforeSession.derivedCaches.collision.world, "provider should not mutate session cached world");
	Expect(explicitWorld.objects().size() == explicitObjects.size(), "provider should not mutate explicit world object count");
	if (explicitWorld.objects().size() == explicitObjects.size() && !explicitObjects.empty())
		ExpectObject(explicitWorld.objects()[0], explicitObjects[0], "provider should not mutate explicit world object");
}

} // namespace

int main()
{
	TestNoExplicitWorldAndNoSessionCacheReturnsEmptyWorld();
	TestExplicitWorldReturnsCopiedExplicitWorld();
	TestSessionDerivedCollisionCacheReturnsCopiedSessionCache();
	TestExplicitWorldTakesPrecedenceOverSessionCache();
	TestSessionWithOnlyRenderCacheReturnsEmptyWorld();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
