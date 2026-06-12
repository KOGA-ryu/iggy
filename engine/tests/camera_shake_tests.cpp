#include <cstdlib>

#include "scene/camera/CameraRig.hpp"
#include "scene/camera/CameraShake.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;
using iggy::test::NearVec;

iggy::CameraShakeState State(float remaining, float elapsed = 0.0F, std::uint32_t seed = 7)
{
	return { remaining, elapsed, seed };
}

iggy::CameraShakeConfig Config(float amplitude = 4.0F, float frequency = 8.0F, float decay = 0.0F)
{
	return { amplitude, frequency, decay };
}

iggy::CameraShakeResult Step(iggy::CameraShakeState state, float delta, iggy::CameraShakeConfig config = Config())
{
	return iggy::CameraShake {}.step({ state, delta, config });
}

void ExpectState(iggy::CameraShakeState actual, iggy::CameraShakeState expected, const char *message)
{
	Expect(Near(actual.remaining, expected.remaining) && Near(actual.elapsed, expected.elapsed) && actual.seed == expected.seed, message);
}

void TestNoRemainingReturnsInactiveAndUnchanged()
{
	const iggy::CameraShakeState state = State(0.0F, 1.0F, 3);
	const iggy::CameraShakeResult result = Step(state, 0.1F);

	Expect(!result.active, "shake with no remaining time should be inactive");
	Expect(NearVec(result.offset, {}), "shake with no remaining time should have zero offset");
	ExpectState(result.state, state, "shake with no remaining time should preserve state");
}

void TestNonPositiveDeltaReturnsInactiveAndUnchanged()
{
	const iggy::CameraShakeState state = State(2.0F, 1.0F, 3);
	const iggy::CameraShakeResult zero = Step(state, 0.0F);
	const iggy::CameraShakeResult negative = Step(state, -0.1F);

	Expect(!zero.active && !negative.active, "nonpositive delta should be inactive");
	Expect(NearVec(zero.offset, {}) && NearVec(negative.offset, {}), "nonpositive delta should have zero offset");
	ExpectState(zero.state, state, "zero delta should preserve state");
	ExpectState(negative.state, state, "negative delta should preserve state");
}

void TestNonPositiveAmplitudeAdvancesButReturnsInactive()
{
	const iggy::CameraShakeResult result = Step(State(2.0F, 1.0F, 3), 0.25F, Config(0.0F, 8.0F));

	Expect(!result.active, "nonpositive amplitude should be inactive");
	Expect(NearVec(result.offset, {}), "nonpositive amplitude should have zero offset");
	ExpectState(result.state, State(1.75F, 1.25F, 3), "nonpositive amplitude should still advance state");
}

void TestNonPositiveFrequencyAdvancesButReturnsInactive()
{
	const iggy::CameraShakeResult result = Step(State(2.0F, 1.0F, 3), 0.25F, Config(4.0F, 0.0F));

	Expect(!result.active, "nonpositive frequency should be inactive");
	Expect(NearVec(result.offset, {}), "nonpositive frequency should have zero offset");
	ExpectState(result.state, State(1.75F, 1.25F, 3), "nonpositive frequency should still advance state");
}

void TestPositiveStepAdvancesElapsedAndRemaining()
{
	const iggy::CameraShakeResult result = Step(State(2.0F, 1.0F, 3), 0.25F);

	Expect(result.active, "positive shake step should be active with positive config");
	ExpectState(result.state, State(1.75F, 1.25F, 3), "positive shake step should advance elapsed and decrease remaining");
}

void TestActiveOffsetIsBoundedByAmplitude()
{
	const float amplitude = 4.0F;
	const iggy::CameraShakeResult result = Step(State(2.0F), 0.25F, Config(amplitude, 8.0F));

	Expect(result.active, "active shake should report active");
	Expect(result.offset.length() <= amplitude + 0.0001F, "active shake offset magnitude should not exceed amplitude");
}

void TestSameInputProducesSameOffset()
{
	const iggy::CameraShakeState state = State(2.0F, 0.5F, 123);
	const iggy::CameraShakeConfig config = Config(4.0F, 8.0F);
	const iggy::CameraShakeResult first = Step(state, 0.25F, config);
	const iggy::CameraShakeResult second = Step(state, 0.25F, config);

	Expect(first.active && second.active, "determinism setup should be active");
	Expect(NearVec(first.offset, second.offset), "same shake input should produce same offset");
	ExpectState(first.state, second.state, "same shake input should produce same next state");
}

void TestDifferentSeedsProduceDifferentOffsets()
{
	const iggy::CameraShakeConfig config = Config(4.0F, 8.0F);
	const iggy::CameraShakeResult first = Step(State(2.0F, 0.5F, 1), 0.25F, config);
	const iggy::CameraShakeResult second = Step(State(2.0F, 0.5F, 2), 0.25F, config);

	Expect(first.active && second.active, "different seed setup should be active");
	Expect(!NearVec(first.offset, second.offset), "different seeds should produce different offsets");
}

void TestDecayReducesEffectiveOffsetBound()
{
	const iggy::CameraShakeResult result = Step(State(3.0F, 0.0F, 5), 1.0F, Config(10.0F, 8.0F, 4.0F));

	Expect(result.active, "decay setup should stay active");
	Expect(Near(result.state.remaining, 2.0F), "decay setup should enter decay window");
	Expect(result.offset.length() <= 5.0F + 0.0001F, "decay should reduce effective offset bound");
}

void TestExhaustingRemainingReturnsInactiveAndZeroOffset()
{
	const iggy::CameraShakeResult result = Step(State(0.25F, 1.0F, 3), 0.5F);

	Expect(!result.active, "exhausting remaining time should be inactive");
	Expect(NearVec(result.offset, {}), "exhausting remaining time should return zero offset");
	ExpectState(result.state, State(0.0F, 1.5F, 3), "exhausting remaining time should clamp remaining to zero");
}

void TestManualRigThenShakeComposition()
{
	iggy::CameraRigConfig rigConfig;
	rigConfig.follow = { 10.0F, 0.0F };
	const iggy::CameraRigResult rig = iggy::CameraRig {}.update({ { 0.0F, 0.0F } }, { 2.0F, 0.0F }, rigConfig);
	const iggy::CameraShakeResult shake = Step(State(2.0F, 0.0F, 9), 0.25F, Config(1.0F, 8.0F));
	const iggy::Vec2 displayedPosition = rig.state.position + shake.offset;

	Expect(rig.followed, "composition rig should follow target");
	Expect(shake.active, "composition shake should be active");
	Expect(NearVec(rig.state.position, { 2.0F, 0.0F }), "shake should not mutate base rig state");
	Expect(!NearVec(displayedPosition, rig.state.position), "active shake should change displayed position when applied manually");
}

} // namespace

int main()
{
	TestNoRemainingReturnsInactiveAndUnchanged();
	TestNonPositiveDeltaReturnsInactiveAndUnchanged();
	TestNonPositiveAmplitudeAdvancesButReturnsInactive();
	TestNonPositiveFrequencyAdvancesButReturnsInactive();
	TestPositiveStepAdvancesElapsedAndRemaining();
	TestActiveOffsetIsBoundedByAmplitude();
	TestSameInputProducesSameOffset();
	TestDifferentSeedsProduceDifferentOffsets();
	TestDecayReducesEffectiveOffsetBound();
	TestExhaustingRemainingReturnsInactiveAndZeroOffset();
	TestManualRigThenShakeComposition();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
