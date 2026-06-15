#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorOccupancyPolicy2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return { Id(npcId), Id("ai-profile:guard"), Id("faction:town"), position, {}, true };
}

iggy::NpcActorOccupancy2D Occupancy(std::vector<iggy::NpcActorState2D> actors)
{
	iggy::NpcActorState2DRegistry registry;
	registry.actors = std::move(actors);
	return iggy::NpcActorOccupancyProjector2D {}.project(registry);
}

void TestDefaultPolicyAllowsEmptyAndSelfOnly()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:mover", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorOccupancyPolicy2DResult empty = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		Id("npc:mover"),
		{ 2, 0 });
	const iggy::NpcActorOccupancyPolicy2DResult self = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		Id("npc:mover"),
		{ 1, 0 });

	Expect(empty.allowed(), "default policy should allow empty tile");
	Expect(empty.occupancyCount == 0, "empty tile should preserve zero occupancy count");
	Expect(self.allowed(), "default policy should allow self-only tile");
	Expect(self.occupancyCount == 1, "self-only tile should preserve occupancy count");
	Expect(self.effectiveOccupancyCount == 0, "self-only tile should not count against moving npc");
}

void TestDefaultPolicyBlocksDifferentNpcAndEmptyMover()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorOccupancyPolicy2DResult blocked = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		Id("npc:mover"),
		{ 1, 0 });
	const iggy::NpcActorOccupancyPolicy2DResult emptyMover = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		{},
		{ 1, 0 });

	Expect(blocked.blocked(), "default policy should block different npc");
	Expect(blocked.blockingNpcId == Id("npc:blocker"), "default policy should preserve blocker id");
	Expect(blocked.effectiveOccupancyCount == 1, "different npc should count as effective occupancy");
	Expect(emptyMover.blocked(), "empty moving npc id should block on occupied tile");
	Expect(emptyMover.blockingNpcId == Id("npc:blocker"), "empty moving npc id should preserve first occupant as blocker");
}

void TestCapacityAllowsSharedTileUntilFull()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:first", { 1.5F, 0.5F }),
		Actor("npc:second", { 1.25F, 0.25F }),
	});
	iggy::NpcActorOccupancyPolicy2DConfig capacity2;
	capacity2.maxOccupantsPerTile = 2;
	iggy::NpcActorOccupancyPolicy2DConfig capacity3;
	capacity3.maxOccupantsPerTile = 3;

	const iggy::NpcActorOccupancyPolicy2DResult full = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		Id("npc:third"),
		{ 1, 0 },
		capacity2);
	const iggy::NpcActorOccupancyPolicy2DResult shared = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		Id("npc:third"),
		{ 1, 0 },
		capacity3);

	Expect(full.blocked(), "capacity policy should block when projected occupants exceed capacity");
	Expect(full.blockingNpcId == Id("npc:first"), "capacity full should report first blocking occupant");
	Expect(full.capacity == 2, "capacity full should preserve configured capacity");
	Expect(shared.allowed(), "capacity policy should allow shared tile before capacity is full");
	Expect(shared.capacity == 3, "shared policy should preserve configured capacity");
}

void TestNamespacedAndUnqualifiedIdsRemainDistinct()
{
	const iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("guard", { 1.5F, 0.5F }),
	});

	const iggy::NpcActorOccupancyPolicy2DResult unqualified = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		Id("guard"),
		{ 1, 0 });
	const iggy::NpcActorOccupancyPolicy2DResult namespaced = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		Id("npc:guard"),
		{ 1, 0 });

	Expect(unqualified.allowed(), "exact unqualified moving id should be self");
	Expect(namespaced.blocked(), "namespaced moving id should differ from unqualified occupant");
	Expect(namespaced.blockingNpcId == Id("guard"), "namespaced id should report unqualified blocker");
}

void TestInputOccupancyNotMutated()
{
	iggy::NpcActorOccupancy2D occupancy = Occupancy({
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	const iggy::NpcActorOccupancy2D before = occupancy;

	const iggy::NpcActorOccupancyPolicy2DResult result = iggy::NpcActorOccupancyPolicy2D {}.evaluate(
		occupancy,
		Id("npc:mover"),
		{ 1, 0 });

	Expect(result.blocked(), "immutability setup should block");
	Expect(occupancy.entries.size() == before.entries.size(), "policy should not mutate occupancy entries");
	Expect(occupancy.occupiedTiles.size() == before.occupiedTiles.size(), "policy should not mutate occupied tiles");
	Expect(occupancy.issues.size() == before.issues.size(), "policy should not mutate occupancy issues");
}

} // namespace

int main()
{
	TestDefaultPolicyAllowsEmptyAndSelfOnly();
	TestDefaultPolicyBlocksDifferentNpcAndEmptyMover();
	TestCapacityAllowsSharedTileUntilFull();
	TestNamespacedAndUnqualifiedIdsRemainDistinct();
	TestInputOccupancyNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
