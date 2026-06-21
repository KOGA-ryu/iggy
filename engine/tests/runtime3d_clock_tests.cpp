#include <cstdlib>

#include "runtime3d/Runtime3DClock.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

void TestNormalRunsAutomaticTick()
{
	iggy::runtime3d::Runtime3DClockState clock;
	clock.mode = iggy::runtime3d::Runtime3DClockMode::Normal;

	const iggy::runtime3d::Runtime3DClockTickDecision decision = iggy::runtime3d::DecideRuntime3DClockTick(clock);

	Expect(decision.shouldRunTick, "normal runtime3d clock should run a tick");
	Expect(decision.automaticTick, "normal runtime3d clock should run automatically");
	Expect(!decision.stepRequestConsumed, "normal runtime3d clock should not consume a step request");
}

void TestPausedDoesNotRunAutomaticTick()
{
	iggy::runtime3d::Runtime3DClockState clock;
	clock.mode = iggy::runtime3d::Runtime3DClockMode::Paused;

	const iggy::runtime3d::Runtime3DClockTickDecision decision = iggy::runtime3d::DecideRuntime3DClockTick(clock);

	Expect(!decision.shouldRunTick, "paused runtime3d clock should not run a tick");
	Expect(!decision.automaticTick, "paused runtime3d clock should not run automatically");
	Expect(!decision.stepRequestConsumed, "paused runtime3d clock should not consume a step request");
}

void TestStepRequestedConsumesOneDecision()
{
	iggy::runtime3d::Runtime3DClockState clock;
	clock.mode = iggy::runtime3d::Runtime3DClockMode::StepRequested;

	const iggy::runtime3d::Runtime3DClockTickDecision decision = iggy::runtime3d::DecideRuntime3DClockTick(clock);

	Expect(decision.shouldRunTick, "step-requested runtime3d clock should run one tick");
	Expect(!decision.automaticTick, "step-requested runtime3d clock should not be automatic");
	Expect(decision.stepRequestConsumed, "step-requested runtime3d clock should consume the step request");
}

} // namespace

int main()
{
	TestNormalRunsAutomaticTick();
	TestPausedDoesNotRunAutomaticTick();
	TestStepRequestedConsumesOneDecision();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
