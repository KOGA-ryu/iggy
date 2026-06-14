#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiProfile2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcAiProfile2D Profile(
	const char *profileId,
	const char *factionId = "faction:neutral",
	float aggression = 0.5F,
	float bravery = 0.5F,
	float alertness = 0.5F,
	float preferredRange = 2.0F,
	std::vector<iggy::ResourceId> behaviorTags = {})
{
	return {
		Id(profileId),
		Id(factionId),
		aggression,
		bravery,
		alertness,
		preferredRange,
		behaviorTags,
	};
}

iggy::NpcAiCurrentState2D State(
	const char *npcId,
	iggy::Vec2 position = { 0.0F, 0.0F },
	const char *currentGoalId = "",
	bool enabled = true)
{
	return {
		Id(npcId),
		position,
		Id(currentGoalId),
		enabled,
	};
}

bool SameProfile(const iggy::NpcAiProfile2D &actual, const iggy::NpcAiProfile2D &expected)
{
	return actual.profileId == expected.profileId
		&& actual.factionId == expected.factionId
		&& actual.aggression == expected.aggression
		&& actual.bravery == expected.bravery
		&& actual.alertness == expected.alertness
		&& actual.preferredRange == expected.preferredRange
		&& actual.behaviorTags == expected.behaviorTags;
}

bool SameState(const iggy::NpcAiCurrentState2D &actual, const iggy::NpcAiCurrentState2D &expected)
{
	return actual.npcId == expected.npcId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.enabled == expected.enabled;
}

void TestValidProfilePreservesFieldsAndTags()
{
	const iggy::NpcAiProfile2D profile =
		Profile("ai-profile:guard", "faction:town", 1.0F, 0.25F, 0.75F, 6.0F, { Id("tag:patrol"), Id("cover"), Id("tag:patrol") });

	const iggy::NpcAiProfile2DValidationResult result = iggy::validate(profile);

	Expect(result.valid, "valid npc ai profile should validate");
	Expect(result.ok(), "valid npc ai profile ok helper should be true");
	Expect(result.issues.empty(), "valid npc ai profile should have no issues");
	Expect(SameProfile(result.profile, profile), "valid npc ai profile validation should preserve profile data");
	Expect(result.profile.behaviorTags == std::vector<iggy::ResourceId>({ Id("tag:patrol"), Id("cover"), Id("tag:patrol") }), "valid npc ai profile should preserve exact tag order and duplicates");
	Expect(iggy::valid(profile), "valid npc ai profile bool helper should be true");
}

void TestDefaultProfileIsInvalidBecauseProfileIdIsRequired()
{
	const iggy::NpcAiProfile2DValidationResult result = iggy::validate(iggy::NpcAiProfile2D {});

	Expect(!result.valid, "default npc ai profile should be invalid because profile id is required");
	Expect(!result.ok(), "default npc ai profile ok helper should be false");
	Expect(result.issues.size() == 1, "default npc ai profile should report one issue");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::NpcAiProfile2DIssueCode::EmptyProfileId, "default npc ai profile issue should be EmptyProfileId");
}

void TestProfileWeightsValidateUnitRangeDeterministically()
{
	const iggy::NpcAiProfile2D lowAggression = Profile("ai-profile:low", "faction:x", -0.01F);
	const iggy::NpcAiProfile2D highAggression = Profile("ai-profile:high", "faction:x", 1.01F);
	const iggy::NpcAiProfile2D lowBravery = Profile("ai-profile:low-bravery", "faction:x", 0.0F, -0.01F);
	const iggy::NpcAiProfile2D highAlertness = Profile("ai-profile:high-alertness", "faction:x", 0.0F, 0.0F, 1.01F);
	const iggy::NpcAiProfile2D edgeValues = Profile("ai-profile:edge", "faction:x", 0.0F, 1.0F, 0.0F);

	const iggy::NpcAiProfile2DValidationResult lowAggressionResult = iggy::validate(lowAggression);
	const iggy::NpcAiProfile2DValidationResult highAggressionResult = iggy::validate(highAggression);
	const iggy::NpcAiProfile2DValidationResult lowBraveryResult = iggy::validate(lowBravery);
	const iggy::NpcAiProfile2DValidationResult highAlertnessResult = iggy::validate(highAlertness);
	const iggy::NpcAiProfile2DValidationResult edgeResult = iggy::validate(edgeValues);

	Expect(lowAggressionResult.issues.size() == 1 && lowAggressionResult.issues[0].code == iggy::NpcAiProfile2DIssueCode::AggressionOutOfRange, "negative aggression should be out of range");
	Expect(highAggressionResult.issues.size() == 1 && highAggressionResult.issues[0].code == iggy::NpcAiProfile2DIssueCode::AggressionOutOfRange, "aggression above one should be out of range");
	Expect(lowBraveryResult.issues.size() == 1 && lowBraveryResult.issues[0].code == iggy::NpcAiProfile2DIssueCode::BraveryOutOfRange, "negative bravery should be out of range");
	Expect(highAlertnessResult.issues.size() == 1 && highAlertnessResult.issues[0].code == iggy::NpcAiProfile2DIssueCode::AlertnessOutOfRange, "alertness above one should be out of range");
	Expect(edgeResult.valid, "zero and one AI trait weights should be valid bounds");
}

void TestNegativePreferredRangeReportsInvalid()
{
	const iggy::NpcAiProfile2D profile = Profile("ai-profile:ranged", "faction:x", 0.5F, 0.5F, 0.5F, -1.0F);

	const iggy::NpcAiProfile2DValidationResult result = iggy::validate(profile);

	Expect(!result.valid, "negative preferred range should invalidate npc ai profile");
	Expect(result.issues.size() == 1, "negative preferred range should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::NpcAiProfile2DIssueCode::NegativePreferredRange, "negative preferred range issue should use NegativePreferredRange");
		Expect(SameProfile(result.issues[0].profile, profile), "negative preferred range issue should preserve profile data");
	}
}

void TestMultipleProfileIssuesPreserveFieldOrder()
{
	const iggy::NpcAiProfile2D profile = Profile("", "faction:x", -1.0F, 2.0F, -0.5F, -3.0F);

	const iggy::NpcAiProfile2DValidationResult result = iggy::validate(profile);

	Expect(result.issues.size() == 5, "invalid profile should report all deterministic issues");
	if (result.issues.size() == 5) {
		Expect(result.issues[0].code == iggy::NpcAiProfile2DIssueCode::EmptyProfileId, "empty profile id should be first issue");
		Expect(result.issues[1].code == iggy::NpcAiProfile2DIssueCode::AggressionOutOfRange, "aggression issue should follow empty id");
		Expect(result.issues[2].code == iggy::NpcAiProfile2DIssueCode::BraveryOutOfRange, "bravery issue should follow aggression");
		Expect(result.issues[3].code == iggy::NpcAiProfile2DIssueCode::AlertnessOutOfRange, "alertness issue should follow bravery");
		Expect(result.issues[4].code == iggy::NpcAiProfile2DIssueCode::NegativePreferredRange, "preferred range issue should be last");
	}
	Expect(SameProfile(result.profile, profile), "invalid profile validation should still preserve profile data");
}

void TestNamespacedAndUnqualifiedProfileTagsRemainDistinct()
{
	const iggy::NpcAiProfile2D profile =
		Profile("ai-profile:tags", "faction:x", 0.5F, 0.5F, 0.5F, 1.0F, { Id("guard"), Id("tag:guard") });

	const iggy::NpcAiProfile2DValidationResult result = iggy::validate(profile);

	Expect(result.valid, "namespaced and unqualified tags should not affect profile validity");
	Expect(result.profile.behaviorTags == std::vector<iggy::ResourceId>({ Id("guard"), Id("tag:guard") }), "namespaced and unqualified profile tags should remain distinct");
}

void TestValidCurrentStatePreservesFields()
{
	const iggy::NpcAiCurrentState2D state = State("npc:guard", { 3.0F, 4.0F }, "goal:patrol", false);

	const iggy::NpcAiCurrentState2DValidationResult result = iggy::validate(state);

	Expect(result.valid, "valid npc ai current state should validate");
	Expect(result.ok(), "valid npc ai current state ok helper should be true");
	Expect(result.issues.empty(), "valid npc ai current state should have no issues");
	Expect(SameState(result.state, state), "valid npc ai current state should preserve state data");
	Expect(iggy::valid(state), "valid npc ai current state bool helper should be true");
}

void TestDefaultCurrentStateIsInvalidBecauseNpcIdIsRequired()
{
	const iggy::NpcAiCurrentState2DValidationResult result = iggy::validate(iggy::NpcAiCurrentState2D {});

	Expect(!result.valid, "default npc ai current state should be invalid because npc id is required");
	Expect(result.issues.size() == 1, "default npc ai current state should report one issue");
	if (result.issues.size() == 1)
		Expect(result.issues[0].code == iggy::NpcAiCurrentState2DIssueCode::EmptyNpcId, "default npc ai current state issue should be EmptyNpcId");
}

void TestCurrentStateAllowsEmptyGoalAndDisabledState()
{
	const iggy::NpcAiCurrentState2D state = State("npc:idle", { -1.0F, 2.0F }, "", false);

	const iggy::NpcAiCurrentState2DValidationResult result = iggy::validate(state);

	Expect(result.valid, "npc ai current state should allow empty current goal");
	Expect(!result.state.enabled, "npc ai current state should preserve disabled flag");
	Expect(result.state.currentGoalId.empty(), "npc ai current state should preserve empty current goal as data");
}

void TestValidationDoesNotMutateInputs()
{
	iggy::NpcAiProfile2D profile =
		Profile("ai-profile:immutable", "faction:x", 0.1F, 0.2F, 0.3F, 4.0F, { Id("tag:a"), Id("tag:b") });
	iggy::NpcAiCurrentState2D state = State("npc:immutable", { 8.0F, 9.0F }, "goal:wait", true);
	const iggy::NpcAiProfile2D profileBefore = profile;
	const iggy::NpcAiCurrentState2D stateBefore = state;

	const iggy::NpcAiProfile2DValidationResult profileResult = iggy::validate(profile);
	const iggy::NpcAiCurrentState2DValidationResult stateResult = iggy::validate(state);

	Expect(profileResult.valid && stateResult.valid, "npc ai validation immutability setup should be valid");
	Expect(SameProfile(profile, profileBefore), "profile validation should not mutate input");
	Expect(SameState(state, stateBefore), "current state validation should not mutate input");
}

} // namespace

int main()
{
	TestValidProfilePreservesFieldsAndTags();
	TestDefaultProfileIsInvalidBecauseProfileIdIsRequired();
	TestProfileWeightsValidateUnitRangeDeterministically();
	TestNegativePreferredRangeReportsInvalid();
	TestMultipleProfileIssuesPreserveFieldOrder();
	TestNamespacedAndUnqualifiedProfileTagsRemainDistinct();
	TestValidCurrentStatePreservesFields();
	TestDefaultCurrentStateIsInvalidBecauseNpcIdIsRequired();
	TestCurrentStateAllowsEmptyGoalAndDisabledState();
	TestValidationDoesNotMutateInputs();

	return Failures;
}
