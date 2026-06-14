#include <cstdlib>

#include "scene/npc/NpcMoveMode2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;

void TestDefaultModeIsNone()
{
	const iggy::NpcMoveMode2D mode {};

	Expect(mode == iggy::NpcMoveMode2D::None, "default NPC move mode should be None");
	Expect(!iggy::npcMoveModeMoves(mode), "default NPC move mode should not move");
	Expect(Near(iggy::npcMoveModeSpeedMultiplier(mode), 0.0F), "default NPC move mode should have zero speed multiplier");
}

void TestStationaryModesDoNotMove()
{
	Expect(!iggy::npcMoveModeMoves(iggy::NpcMoveMode2D::None), "None move mode should not move");
	Expect(!iggy::npcMoveModeMoves(iggy::NpcMoveMode2D::Still), "Still move mode should not move");
	Expect(Near(iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::None), 0.0F), "None move mode should have zero multiplier");
	Expect(Near(iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Still), 0.0F), "Still move mode should have zero multiplier");
}

void TestMovingModesMove()
{
	Expect(iggy::npcMoveModeMoves(iggy::NpcMoveMode2D::Walk), "Walk move mode should move");
	Expect(iggy::npcMoveModeMoves(iggy::NpcMoveMode2D::Jog), "Jog move mode should move");
	Expect(iggy::npcMoveModeMoves(iggy::NpcMoveMode2D::Run), "Run move mode should move");
	Expect(iggy::npcMoveModeMoves(iggy::NpcMoveMode2D::Sprint), "Sprint move mode should move");
}

void TestSpeedMultipliersAreDeterministic()
{
	Expect(Near(iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Walk), 1.0F), "Walk move mode should use base multiplier");
	Expect(Near(iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Jog), 1.5F), "Jog move mode should use jog multiplier");
	Expect(Near(iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Run), 2.0F), "Run move mode should use run multiplier");
	Expect(Near(iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Sprint), 3.0F), "Sprint move mode should use sprint multiplier");
}

void TestSpeedMultipliersAreOrdered()
{
	const float still = iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Still);
	const float walk = iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Walk);
	const float jog = iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Jog);
	const float run = iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Run);
	const float sprint = iggy::npcMoveModeSpeedMultiplier(iggy::NpcMoveMode2D::Sprint);

	Expect(still < walk, "Still speed should be less than Walk");
	Expect(walk < jog, "Walk speed should be less than Jog");
	Expect(jog < run, "Jog speed should be less than Run");
	Expect(run < sprint, "Run speed should be less than Sprint");
}

} // namespace

int main()
{
	TestDefaultModeIsNone();
	TestStationaryModesDoNotMove();
	TestMovingModesMove();
	TestSpeedMultipliersAreDeterministic();
	TestSpeedMultipliersAreOrdered();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
