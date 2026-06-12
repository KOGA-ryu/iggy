#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

#include "modules/npc_ai/NpcMovementPlanner.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void ExpectDestination(const iggy::npc_ai::NpcMovementPlan &plan, iggy::Vec2 expected, std::string_view message)
{
	Expect(plan.destination.has_value(), message);
	if (!plan.destination.has_value())
		return;
	Expect(*plan.destination == expected, message);
}

void TestIdleProducesNoMovement()
{
	const iggy::npc_ai::NpcMovementPlan plan = iggy::npc_ai::NpcMovementPlanner {}.plan({ iggy::npc_ai::NpcIntentType::Idle, { 2, 1 } }, { 4, 4 });

	Expect(plan.type == iggy::npc_ai::NpcMovementPlanType::None, "idle intent should produce no movement plan");
	Expect(!plan.destination.has_value(), "idle movement plan should not have a destination");
}

void TestPursueMovesTowardTargetTile()
{
	const iggy::npc_ai::NpcMovementPlan plan = iggy::npc_ai::NpcMovementPlanner {}.plan({ iggy::npc_ai::NpcIntentType::PursueVisibleTarget, { 5, 2 } }, { 1, 1 });

	Expect(plan.type == iggy::npc_ai::NpcMovementPlanType::MoveTo, "pursue intent should move toward target tile");
	ExpectDestination(plan, { 5.5F, 2.5F }, "pursue movement should target center of visible target tile");
}

void TestInvestigateMovesTowardLastSeenTile()
{
	const iggy::npc_ai::NpcMovementPlan plan = iggy::npc_ai::NpcMovementPlanner {}.plan({ iggy::npc_ai::NpcIntentType::InvestigateLastSeen, { 3, 4 } }, { 1, 1 });

	Expect(plan.type == iggy::npc_ai::NpcMovementPlanType::MoveTo, "investigate intent should move toward last seen tile");
	ExpectDestination(plan, { 3.5F, 4.5F }, "investigate movement should target center of last seen tile");
}

void TestReturnToPostMovesTowardHomeTile()
{
	const iggy::npc_ai::NpcMovementPlan plan = iggy::npc_ai::NpcMovementPlanner {}.plan({ iggy::npc_ai::NpcIntentType::ReturnToPost, { -1, -1 } }, { 7, 6 });

	Expect(plan.type == iggy::npc_ai::NpcMovementPlanType::MoveTo, "return-to-post intent should move toward home tile");
	ExpectDestination(plan, { 7.5F, 6.5F }, "return-to-post movement should target center of home tile");
}

void TestInvalidTargetTileProducesNoMovement()
{
	const iggy::npc_ai::NpcMovementPlan plan = iggy::npc_ai::NpcMovementPlanner {}.plan({ iggy::npc_ai::NpcIntentType::InvestigateLastSeen, { -1, -1 } }, { 1, 1 });

	Expect(plan.type == iggy::npc_ai::NpcMovementPlanType::None, "invalid intent target should produce no movement plan");
	Expect(!plan.destination.has_value(), "invalid intent target should not have a destination");
}

} // namespace

int main()
{
	TestIdleProducesNoMovement();
	TestPursueMovesTowardTargetTile();
	TestInvestigateMovesTowardLastSeenTile();
	TestReturnToPostMovesTowardHomeTile();
	TestInvalidTargetTileProducesNoMovement();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
