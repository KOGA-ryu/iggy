#include <cstdlib>

#include "runtime3d/Runtime3DWorldState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestAddFindAndRejectDuplicateEntity()
{
	iggy::runtime3d::Runtime3DWorldState world;
	iggy::runtime3d::Runtime3DEntityState player;
	player.id = { 42 };
	player.kind = iggy::runtime3d::Runtime3DEntityKind::Player;
	player.assetRef = "asset:player";

	Expect(world.add(player), "world should add player entity");
	Expect(!world.add(player), "world should reject duplicate entity ids");

	const iggy::runtime3d::Runtime3DEntityState *found = world.find({ 42 });
	Expect(found != nullptr, "world should find stable entity id");
	if (found != nullptr)
		Expect(found->kind == iggy::runtime3d::Runtime3DEntityKind::Player, "found entity should preserve kind");
}

void TestUpsertAddsAndReplacesEntity()
{
	iggy::runtime3d::Runtime3DWorldState world;
	iggy::runtime3d::Runtime3DEntityState pickup;
	pickup.id = { 7 };
	pickup.kind = iggy::runtime3d::Runtime3DEntityKind::Pickup;
	pickup.assetRef = "asset:key";

	Expect(world.upsert(pickup), "world upsert should add missing pickup");

	pickup.assetRef = "asset:gold_key";
	pickup.persistent = false;
	Expect(world.upsert(pickup), "world upsert should replace existing pickup");

	const iggy::runtime3d::Runtime3DEntityState *found = world.find({ 7 });
	Expect(found != nullptr, "world should find upserted pickup");
	if (found != nullptr) {
		Expect(found->assetRef == "asset:gold_key", "world upsert should replace asset ref");
		Expect(!found->persistent, "world upsert should replace persistence flag");
	}
	Expect(world.entities.size() == 1, "world upsert should not duplicate entity");
}

} // namespace

int main()
{
	TestAddFindAndRejectDuplicateEntity();
	TestUpsertAddsAndReplacesEntity();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
