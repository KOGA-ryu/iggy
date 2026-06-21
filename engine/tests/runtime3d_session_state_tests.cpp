#include <cstdlib>

#include "runtime3d/Runtime3DSession.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestDefaultSessionStateIsNotLoaded()
{
	const iggy::runtime3d::Runtime3DSessionState state;

	Expect(state.lifecycle == iggy::runtime3d::Runtime3DSessionLifecycle::NotLoaded, "default runtime3d session should be not loaded");
	Expect(state.world.entities.empty(), "default runtime3d session should have an empty world");
	Expect(state.commandLog.empty(), "default runtime3d session should have an empty command log");
}

void TestCreateSessionWithWorldEntity()
{
	iggy::runtime3d::Runtime3DWorldState world;
	iggy::runtime3d::Runtime3DEntityState player;
	player.id = { 1 };
	player.kind = iggy::runtime3d::Runtime3DEntityKind::Player;
	player.assetRef = "asset:player";

	Expect(world.add(player), "world should add a stable player entity");

	const iggy::runtime3d::Runtime3DSession session =
		iggy::runtime3d::Runtime3DSession::Create(world, iggy::runtime3d::Runtime3DSessionLifecycle::PlayingRealtime);

	Expect(session.state().lifecycle == iggy::runtime3d::Runtime3DSessionLifecycle::PlayingRealtime, "created runtime3d session should enter realtime play");
	const iggy::runtime3d::Runtime3DEntityState *found = session.state().world.find({ 1 });
	Expect(found != nullptr, "created runtime3d session should preserve world entity");
	if (found != nullptr)
		Expect(found->assetRef == "asset:player", "created runtime3d session should preserve entity asset ref");
}

} // namespace

int main()
{
	TestDefaultSessionStateIsNotLoaded();
	TestCreateSessionWithWorldEntity();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
