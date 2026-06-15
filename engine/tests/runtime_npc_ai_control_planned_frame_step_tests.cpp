#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcAiControlPlannedFrameStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

template <typename T, typename = void>
struct HasNpcActorsField : std::false_type {
};

template <typename T>
struct HasNpcActorsField<T, std::void_t<decltype(&T::npcActors)>> : std::true_type {
};

template <typename T, typename = void>
struct HasNpcControlsField : std::false_type {
};

template <typename T>
struct HasNpcControlsField<T, std::void_t<decltype(&T::npcControls)>> : std::true_type {
};

static_assert(!HasNpcActorsField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasNpcControlsField<iggy::runtime::RuntimeSessionState>::value);

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position = { 0.0F, 0.0F },
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:runtime-ai-control"),
		Id("faction:runtime-ai-control"),
		position,
		Id("goal:runtime-ai-control"),
		present,
	};
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::NpcBehaviorState behavior = iggy::seekingNpcBehaviorState({ 5.0F, 0.0F }),
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::None)
{
	return {
		Id(npcId),
		behavior.type == iggy::NpcBehaviorStateType::Fleeing
			? iggy::fleeNpcObjective(behavior.targetPosition)
			: iggy::moveToNpcObjective(behavior.targetPosition),
		behavior,
		moveMode,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "runtime ai control actor registry should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime ai control control registry should build");
	return result.registry;
}

iggy::NpcMapPlayControlFramePlanSubject Subject(const char *npcId, iggy::NpcTraitSet traits = {})
{
	iggy::NpcMapPlayControlFramePlanSubject subject;
	subject.npcId = Id(npcId);
	subject.traits = traits;
	return subject;
}

iggy::NpcMapPlayControlFramePlanSubject SubjectWithTarget(
	const char *npcId,
	iggy::Vec2 target)
{
	iggy::NpcMapPlayControlFramePlanSubject subject = Subject(npcId);
	subject.hasProposalContextOverride = true;
	subject.proposalContext.hasTargetPosition = true;
	subject.proposalContext.targetPosition = target;
	return subject;
}

iggy::NpcMapPlayControlFramePlanSubject SubjectWithEmptyContext(const char *npcId)
{
	iggy::NpcMapPlayControlFramePlanSubject subject = Subject(npcId);
	subject.hasProposalContextOverride = true;
	return subject;
}

iggy::AiMapNode2D Node(
	const char *id,
	iggy::Vec2 position,
	std::vector<iggy::ResourceId> tags)
{
	return {
		Id(id),
		position,
		3.0F,
		1.0F,
		2.0F,
		3.0F,
		4.0F,
		tags,
		{},
		true,
	};
}

iggy::AiMap2D Map(std::vector<iggy::AiMapNode2D> nodes = {})
{
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);
	Expect(result.built, "runtime ai control map should build");
	return result.map;
}

iggy::NpcStrengthEnt StrengthEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behavior,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, mapTags };
}

iggy::NpcDexterityEnt DexterityEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behavior,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, mapTags };
}

iggy::NpcMapPlayControlFramePlanPools Pools(
	std::vector<iggy::NpcStrengthEnt> strength = {},
	std::vector<iggy::NpcDexterityEnt> dexterity = {})
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strengthBuild =
		iggy::NpcStrengthPoolBuilder {}.build(strength);
	const iggy::NpcDexterityPoolBuildResult dexterityBuild =
		iggy::NpcDexterityPoolBuilder {}.build(dexterity);
	const iggy::NpcConstitutionPoolBuildResult constitutionBuild =
		iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligenceBuild =
		iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdomBuild =
		iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charismaBuild =
		iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strengthBuild.built, "runtime ai control strength pool should build");
	Expect(dexterityBuild.built, "runtime ai control dexterity pool should build");
	Expect(constitutionBuild.built, "runtime ai control constitution pool should build");
	Expect(intelligenceBuild.built, "runtime ai control intelligence pool should build");
	Expect(wisdomBuild.built, "runtime ai control wisdom pool should build");
	Expect(charismaBuild.built, "runtime ai control charisma pool should build");
	pools.strength = strengthBuild.pool;
	pools.dexterity = dexterityBuild.pool;
	pools.constitution = constitutionBuild.pool;
	pools.intelligence = intelligenceBuild.pool;
	pools.wisdom = wisdomBuild.pool;
	pools.charisma = charismaBuild.pool;
	return pools;
}

iggy::runtime::RuntimeGameplayState GameplayState()
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.tickIndex = 42;
	state.commandQueue.frames = { { { iggy::runtime::GameplayCommand2DFactory {}.wait(Id("player:queued")) } } };
	state.inventory.inventory.stacks = { { Id("item:runtime-ai-control"), 2 } };
	return state;
}

iggy::runtime::RuntimeNpcAiControlPlannedFrameInput Input(
	const iggy::runtime::RuntimeGameplayState &state,
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects,
	const iggy::NpcMapPlayControlFramePlanPools &pools,
	const iggy::AiMap2D &map,
	const iggy::NpcMapPlayControlFramePlanConfig &config = {})
{
	return {
		state,
		subjects,
		pools,
		map,
		config,
	};
}

bool SameActors(
	const iggy::NpcActorState2DRegistry &actual,
	const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size())
		return false;
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		const iggy::NpcActorState2D &left = actual.actors[index];
		const iggy::NpcActorState2D &right = expected.actors[index];
		if (left.npcId != right.npcId
			|| left.aiProfileId != right.aiProfileId
			|| left.factionId != right.factionId
			|| !NearVec(left.position, right.position)
			|| left.currentGoalId != right.currentGoalId
			|| left.present != right.present)
			return false;
	}
	return true;
}

bool SameControls(
	const iggy::NpcActorControlState2DRegistry &actual,
	const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size())
		return false;
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		const iggy::NpcActorControlState2D &left = actual.entries[index];
		const iggy::NpcActorControlState2D &right = expected.entries[index];
		if (left.npcId != right.npcId
			|| left.objective.type != right.objective.type
			|| left.objective.targetId != right.objective.targetId
			|| !NearVec(left.objective.targetPosition, right.objective.targetPosition)
			|| left.behavior.type != right.behavior.type
			|| left.behavior.targetId != right.behavior.targetId
			|| !NearVec(left.behavior.targetPosition, right.behavior.targetPosition)
			|| left.moveMode != right.moveMode)
			return false;
	}
	return true;
}

void ExpectNonControlStatePreserved(
	const iggy::runtime::RuntimeGameplayState &actual,
	const iggy::runtime::RuntimeGameplayState &expected)
{
	Expect(actual.session.tickIndex == expected.session.tickIndex, "runtime ai control should preserve session tick");
	Expect(actual.commandQueue.frames.size() == expected.commandQueue.frames.size(), "runtime ai control should preserve command queue");
	Expect(actual.inventory.inventory.stacks.size() == expected.inventory.inventory.stacks.size(), "runtime ai control should preserve inventory stacks");
	Expect(SameActors(actual.npcActors, expected.npcActors), "runtime ai control should preserve npc actors");
}

void TestEmptyDefaultStateNoOps()
{
	const iggy::runtime::RuntimeGameplayState state;
	const iggy::runtime::RuntimeNpcAiControlPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run(
			Input(state, {}, Pools(), Map()));

	Expect(result.status == iggy::runtime::RuntimeNpcAiControlPlannedFrameStatus::NoRequests, "empty ai control frame should report no requests");
	Expect(!result.ranControlRequests(), "empty ai control frame should not run requests");
	Expect(!result.changedState(), "empty ai control frame should not change state");
	Expect(result.plannedRequestCount == 0 && result.appliedControlCount == 0, "empty ai control frame should count no requests or apply");
	Expect(SameControls(result.state.npcControls, state.npcControls), "empty ai control frame should preserve controls");
}

void TestOneNpcPlansAndAppliesControlUpdate()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:guard", { 0.0F, 0.0F }) });
	state.npcControls = Controls({ Control("npc:guard", iggy::seekingNpcBehaviorState({ 6.0F, 0.0F }), iggy::NpcMoveMode::None) });
	iggy::NpcMapPlayControlFramePlanConfig config;
	config.step.mapRead.matchedMapTagBonus = 4.0F;
	const iggy::runtime::RuntimeNpcAiControlPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run(
			Input(
				state,
				{ SubjectWithTarget("npc:guard", { 6.0F, 0.0F }) },
				Pools(
					{ StrengthEnt("strength:map-seek", "action:seek-cover", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }) },
					{ DexterityEnt("dexterity:raw-seek", "action:seek-raw", iggy::NpcBehaviorStateType::Seeking, 5.0F, { Id("zone:loud") }) }),
				Map({ Node("ai:cover", { 0.0F, 0.0F }, { Id("zone:cover") }) }),
				config));

	Expect(result.status == iggy::runtime::RuntimeNpcAiControlPlannedFrameStatus::Ran, "matching ai control frame should run");
	Expect(result.plannedRequestCount == 1 && result.appliedControlCount == 1, "matching ai control frame should plan and apply one control");
	Expect(result.mapChangedSelectionCount == 1, "matching ai control frame should preserve changed map-selection count");
	Expect(result.changedControls, "matching ai control frame should change controls");
	Expect(result.state.npcControls.entries.size() == 1, "matching ai control frame should preserve one control");
	if (result.state.npcControls.entries.size() == 1) {
		Expect(result.state.npcControls.entries[0].npcId == Id("npc:guard"), "matching ai control frame should preserve npc id");
		Expect(result.state.npcControls.entries[0].moveMode == iggy::NpcMoveMode::Walk, "matching ai control frame should use proposal move mode");
		Expect(result.step.entries[0].mapPlay.rawSelectedActionTag == Id("action:seek-raw"), "matching ai control frame should preserve raw winner");
		Expect(result.step.entries[0].mapPlay.mapSelectedActionTag == Id("action:seek-cover"), "matching ai control frame should preserve map winner");
	}
	ExpectNonControlStatePreserved(result.state, state);
}

void TestMissingTraitAndControlDiagnosticsPreserveControls()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({
		Actor("npc:missing-traits"),
		Actor("npc:no-control"),
	});
	state.npcControls = Controls({
		Control("npc:missing-traits"),
		Control("npc:orphan"),
	});

	const iggy::runtime::RuntimeNpcAiControlPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run(
			Input(state, {}, Pools(), Map()));

	Expect(result.status == iggy::runtime::RuntimeNpcAiControlPlannedFrameStatus::NoRequests, "diagnostic-only ai control frame should report no requests");
	Expect(result.plan.missingTraitCount == 1, "diagnostic-only ai control frame should preserve missing trait count");
	Expect(result.plan.missingControlCount == 1, "diagnostic-only ai control frame should preserve missing control count");
	Expect(result.plan.orphanControlCount == 1, "diagnostic-only ai control frame should preserve orphan control count");
	Expect(result.planIssueCount == result.plan.issues.size(), "diagnostic-only ai control frame should mirror plan issue count");
	Expect(SameControls(result.state.npcControls, state.npcControls), "diagnostic-only ai control frame should preserve controls");
	ExpectNonControlStatePreserved(result.state, state);
}

void TestFoldedNoPlayAndInvalidContextDoNotMutateControls()
{
	iggy::runtime::RuntimeGameplayState foldedState = GameplayState();
	foldedState.npcActors = Actors({ Actor("npc:folded") });
	foldedState.npcControls = Controls({ Control("npc:folded") });
	const iggy::runtime::RuntimeNpcAiControlPlannedFrameResult folded =
		iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run(
			Input(foldedState, { Subject("npc:folded") }, Pools(), Map()));

	Expect(folded.plannedRequestCount == 1, "folded no-play should still prepare diagnostic request");
	Expect(folded.failedControlCount == 1, "folded no-play should count failed control apply");
	Expect(!folded.changedControls, "folded no-play should not change controls");
	Expect(SameControls(folded.state.npcControls, foldedState.npcControls), "folded no-play should preserve controls");

	iggy::runtime::RuntimeGameplayState invalidState = GameplayState();
	invalidState.npcActors = Actors({ Actor("npc:invalid-context") });
	invalidState.npcControls = Controls({ Control("npc:invalid-context") });
	const iggy::runtime::RuntimeNpcAiControlPlannedFrameResult invalid =
		iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run(
			Input(
				invalidState,
				{ SubjectWithEmptyContext("npc:invalid-context") },
				Pools({ StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
				Map()));

	Expect(invalid.plannedRequestCount == 1, "invalid context should prepare request");
	Expect(invalid.failedControlCount == 1, "invalid context should count failed apply");
	Expect(!invalid.changedControls, "invalid context should not change controls");
	Expect(invalid.step.entries.size() == 1 && invalid.step.entries[0].proposal.status == iggy::NpcPlayControlProposalStatus::MissingTargetPosition, "invalid context should preserve missing target diagnostics");
	Expect(SameControls(invalid.state.npcControls, invalidState.npcControls), "invalid context should preserve controls");
}

void TestManualCompositionMatchesHelper()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:manual", { 0.0F, 0.0F }) });
	state.npcControls = Controls({ Control("npc:manual", iggy::seekingNpcBehaviorState({ 4.0F, 0.0F }), iggy::NpcMoveMode::None) });
	iggy::NpcMapPlayControlFramePlanConfig config;
	config.step.mapRead.matchedMapTagBonus = 4.0F;
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects {
		SubjectWithTarget("npc:manual", { 4.0F, 0.0F }),
	};
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools({
		StrengthEnt("strength:manual", "action:manual", iggy::NpcBehaviorStateType::Seeking, 1.0F, { Id("zone:manual") }),
	});
	const iggy::AiMap2D map = Map({ Node("ai:manual", { 0.0F, 0.0F }, { Id("zone:manual") }) });

	const iggy::runtime::RuntimeNpcAiControlPlannedFrameInput input =
		Input(state, subjects, pools, map, config);
	const iggy::runtime::RuntimeNpcAiControlPlannedFrameResult helper =
		iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run(input);

	const iggy::NpcMapPlayControlFramePlanResult manualPlan =
		iggy::NpcMapPlayControlFramePlanner {}.plan(
			state.npcActors,
			state.npcControls,
			subjects,
			pools,
			map,
			config);
	const iggy::NpcMapPlayControlFrameStep2DResult manualStep =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			state.npcControls,
			manualPlan.requests,
			config.step);
	iggy::runtime::RuntimeGameplayState manualState = state;
	manualState.npcControls = manualStep.registry;

	Expect(SameControls(helper.state.npcControls, manualState.npcControls), "helper should match manual composition controls");
	Expect(helper.plannedRequestCount == manualPlan.requestCount, "helper should match manual planned count");
	Expect(helper.appliedControlCount == manualStep.appliedCount, "helper should match manual applied count");
	Expect(helper.mapChangedSelectionCount == manualStep.mapChangedSelectionCount, "helper should match manual map selection count");
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:guard", { 0.0F, 0.0F }) });
	state.npcControls = Controls({ Control("npc:guard") });
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects { Subject("npc:guard") };
	iggy::NpcMapPlayControlFramePlanPools pools = Pools({
		StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F),
	});
	iggy::AiMap2D map = Map({ Node("ai:cover", { 0.0F, 0.0F }, { Id("zone:cover") }) });
	iggy::NpcMapPlayControlFramePlanConfig config;

	const iggy::runtime::RuntimeGameplayState beforeState = state;
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> beforeSubjects = subjects;
	const iggy::NpcMapPlayControlFramePlanPools beforePools = pools;
	const iggy::AiMap2D beforeMap = map;
	const iggy::NpcMapPlayControlFramePlanConfig beforeConfig = config;

	(void)iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run(
		Input(state, subjects, pools, map, config));

	Expect(SameActors(state.npcActors, beforeState.npcActors), "helper should not mutate input actors");
	Expect(SameControls(state.npcControls, beforeState.npcControls), "helper should not mutate input controls");
	Expect(subjects.size() == beforeSubjects.size() && subjects[0].npcId == beforeSubjects[0].npcId, "helper should not mutate subjects");
	Expect(pools.strength.entries.size() == beforePools.strength.entries.size(), "helper should not mutate pools");
	Expect(map.nodes.size() == beforeMap.nodes.size() && map.nodes[0].id == beforeMap.nodes[0].id, "helper should not mutate map");
	Expect(config.includeAbsentActors == beforeConfig.includeAbsentActors, "helper should not mutate config");
}

} // namespace

int main()
{
	TestEmptyDefaultStateNoOps();
	TestOneNpcPlansAndAppliesControlUpdate();
	TestMissingTraitAndControlDiagnosticsPreserveControls();
	TestFoldedNoPlayAndInvalidContextDoNotMutateControls();
	TestManualCompositionMatchesHelper();
	TestInputsAreNotMutated();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
