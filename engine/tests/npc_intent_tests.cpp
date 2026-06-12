#include <cstdlib>
#include <iostream>
#include <string_view>

#include "modules/npc_ai/NpcIntentSelector.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool SameTile(iggy::TileCoord actual, int x, int y)
{
	return actual.x == x && actual.y == y;
}

void TestVisibleTargetPursues()
{
	iggy::npc_ai::AwarenessState awareness;
	awareness.playerVisible = true;
	awareness.alerted = true;
	awareness.lastSeenTile = { 4, 2 };

	const iggy::npc_ai::NpcIntent intent = iggy::npc_ai::NpcIntentSelector {}.select(awareness);

	Expect(intent.type == iggy::npc_ai::NpcIntentType::PursueVisibleTarget, "visible target should select pursue intent");
	Expect(SameTile(intent.targetTile, 4, 2), "pursue intent should target visible player's tile");
}

void TestAlertMemoryInvestigatesLastSeen()
{
	iggy::npc_ai::AwarenessState awareness;
	awareness.playerVisible = false;
	awareness.alerted = true;
	awareness.alertTicksRemaining = 2;
	awareness.lastSeenTile = { 3, 1 };

	const iggy::npc_ai::NpcIntent intent = iggy::npc_ai::NpcIntentSelector {}.select(awareness);

	Expect(intent.type == iggy::npc_ai::NpcIntentType::InvestigateLastSeen, "alert memory should select investigate intent");
	Expect(SameTile(intent.targetTile, 3, 1), "investigate intent should target last seen tile");
}

void TestRemainingAlertTicksInvestigatesEvenIfAlertFlagIsStale()
{
	iggy::npc_ai::AwarenessState awareness;
	awareness.playerVisible = false;
	awareness.alerted = false;
	awareness.alertTicksRemaining = 1;
	awareness.lastSeenTile = { 2, 5 };

	const iggy::npc_ai::NpcIntent intent = iggy::npc_ai::NpcIntentSelector {}.select(awareness);

	Expect(intent.type == iggy::npc_ai::NpcIntentType::InvestigateLastSeen, "remaining alert ticks should select investigate intent");
	Expect(SameTile(intent.targetTile, 2, 5), "remaining alert ticks should preserve last seen target");
}

void TestNoAwarenessIdlesByDefault()
{
	const iggy::npc_ai::NpcIntent intent = iggy::npc_ai::NpcIntentSelector {}.select({});

	Expect(intent.type == iggy::npc_ai::NpcIntentType::Idle, "no awareness should idle by default");
	Expect(SameTile(intent.targetTile, -1, -1), "idle intent should not have a target tile");
}

void TestNoAwarenessCanReturnToPost()
{
	const iggy::npc_ai::NpcIntent intent = iggy::npc_ai::NpcIntentSelector { { true } }.select({});

	Expect(intent.type == iggy::npc_ai::NpcIntentType::ReturnToPost, "configured calm NPC should return to post");
	Expect(SameTile(intent.targetTile, -1, -1), "return-to-post intent should not need awareness target");
}

} // namespace

int main()
{
	TestVisibleTargetPursues();
	TestAlertMemoryInvestigatesLastSeen();
	TestRemainingAlertTicksInvestigatesEvenIfAlertFlagIsStale();
	TestNoAwarenessIdlesByDefault();
	TestNoAwarenessCanReturnToPost();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
