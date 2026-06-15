#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcAiProfileControlPlannedFrameStep.hpp"
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

iggy::NpcTraitSet Traits(int strength = 10, int dexterity = 10)
{
	iggy::NpcTraitSet traits;
	traits.strength = strength;
	traits.dexterity = dexterity;
	return traits;
}

iggy::NpcAiProfileTraitEntry Profile(const char *profileId, iggy::NpcTraitSet traits = {})
{
	return { Id(profileId), traits };
}

iggy::NpcAiProfileTraitCatalog Catalog(std::vector<iggy::NpcAiProfileTraitEntry> entries)
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build(entries);
	Expect(result.built, "runtime profile control catalog should build");
	return result.catalog;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	const char *profileId,
	iggy::Vec2 position = { 0.0F, 0.0F },
	bool present = true)
{
	return {
		Id(npcId),
		Id(profileId),
		Id("faction:runtime-profile-control"),
		position,
		Id("goal:runtime-profile-control"),
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
	Expect(result.built, "runtime profile control actor registry should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime profile control control registry should build");
	return result.registry;
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
	Expect(result.built, "runtime profile control AI map should build");
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
	Expect(strengthBuild.built, "runtime profile control strength pool should build");
	Expect(dexterityBuild.built, "runtime profile control dexterity pool should build");
	Expect(constitutionBuild.built, "runtime profile control constitution pool should build");
	Expect(intelligenceBuild.built, "runtime profile control intelligence pool should build");
	Expect(wisdomBuild.built, "runtime profile control wisdom pool should build");
	Expect(charismaBuild.built, "runtime profile control charisma pool should build");
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
	state.session.tickIndex = 77;
	state.commandQueue.frames = { { { iggy::runtime::GameplayCommand2DFactory {}.wait(Id("player:queued")) } } };
	state.inventory.inventory.stacks = { { Id("item:runtime-profile-control"), 3 } };
	return state;
}

iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameInput Input(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::NpcAiProfileTraitCatalog &profileTraits,
	const iggy::NpcMapPlayControlFramePlanPools &pools,
	const iggy::AiMap2D &map,
	const iggy::NpcAiProfileTraitResolverConfig &profileConfig = {},
	const iggy::NpcMapPlayControlFramePlanConfig &controlConfig = {})
{
	return {
		state,
		profileTraits,
		pools,
		map,
		profileConfig,
		controlConfig,
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
	Expect(actual.session.tickIndex == expected.session.tickIndex, "runtime profile control should preserve session tick");
	Expect(actual.commandQueue.frames.size() == expected.commandQueue.frames.size(), "runtime profile control should preserve command queue");
	Expect(actual.inventory.inventory.stacks.size() == expected.inventory.inventory.stacks.size(), "runtime profile control should preserve inventory stacks");
	Expect(SameActors(actual.npcActors, expected.npcActors), "runtime profile control should preserve npc actors");
}

void TestEmptyDefaultStateNoOps()
{
	const iggy::runtime::RuntimeGameplayState state;
	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(
			Input(state, Catalog({}), Pools(), Map()));

	Expect(result.status == iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStatus::NoResolvedSubjects, "empty profile control should report no resolved subjects");
	Expect(!result.resolvedSubjects(), "empty profile control should not resolve subjects");
	Expect(!result.changedState(), "empty profile control should not change state");
	Expect(result.resolvedSubjectCount == 0 && result.plannedRequestCount == 0, "empty profile control should count no subjects or requests");
	Expect(SameControls(result.state.npcControls, state.npcControls), "empty profile control should preserve controls");
}

void TestOneActorProfileResolvesAndUpdatesControl()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:guard", "profile:guard", { 0.0F, 0.0F }) });
	state.npcControls = Controls({ Control("npc:guard", iggy::seekingNpcBehaviorState({ 6.0F, 0.0F }), iggy::NpcMoveMode::None) });
	iggy::NpcMapPlayControlFramePlanConfig controlConfig;
	controlConfig.step.mapRead.matchedMapTagBonus = 4.0F;

	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(
			Input(
				state,
				Catalog({ Profile("profile:guard", Traits(12, 15)) }),
				Pools(
					{ StrengthEnt("strength:map-seek", "action:seek-cover", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }) },
					{ DexterityEnt("dexterity:raw-seek", "action:seek-raw", iggy::NpcBehaviorStateType::Seeking, 5.0F, { Id("zone:loud") }) }),
				Map({ Node("ai:cover", { 0.0F, 0.0F }, { Id("zone:cover") }) }),
				{},
				controlConfig));

	Expect(result.status == iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStatus::Ran, "matching profile control should run");
	Expect(result.resolvedSubjectCount == 1 && result.profileSubjects.subjects.size() == 1, "matching profile control should resolve one subject");
	Expect(result.plannedRequestCount == 1 && result.appliedControlCount == 1, "matching profile control should plan and apply one control");
	Expect(result.mapChangedSelectionCount == 1, "matching profile control should preserve map changed selection count");
	Expect(result.changedControls, "matching profile control should change controls");
	if (result.state.npcControls.entries.size() == 1) {
		Expect(result.state.npcControls.entries[0].npcId == Id("npc:guard"), "matching profile control should preserve npc id");
		Expect(result.state.npcControls.entries[0].moveMode == iggy::NpcMoveMode::Walk, "matching profile control should apply proposal move mode");
		Expect(result.control.step.entries[0].mapPlay.rawSelectedActionTag == Id("action:seek-raw"), "matching profile control should preserve raw selected action");
		Expect(result.control.step.entries[0].mapPlay.mapSelectedActionTag == Id("action:seek-cover"), "matching profile control should preserve map selected action");
	}
	ExpectNonControlStatePreserved(result.state, state);
}

void TestMissingProfileRecordsIssueAndPreservesControls()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:missing", "profile:missing") });
	state.npcControls = Controls({ Control("npc:missing") });

	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(
			Input(state, Catalog({}), Pools(), Map()));

	Expect(result.missingProfileCount == 1, "missing profile should be mirrored");
	Expect(!result.profileSubjects.issues.empty() && result.profileSubjects.issues[0].code == iggy::NpcAiProfileTraitResolveIssueCode::MissingProfile, "missing profile should preserve resolver issue");
	Expect(result.plannedRequestCount == 0, "missing profile should not plan control request");
	Expect(result.control.plan.missingTraitCount == 1, "missing profile should surface as missing trait to delegated control planner");
	Expect(SameControls(result.state.npcControls, state.npcControls), "missing profile should preserve controls");
}

void TestMultipleActorsPreserveActorOrder()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({
		Actor("npc:first", "profile:first"),
		Actor("npc:second", "profile:second"),
	});
	state.npcControls = Controls({
		Control("npc:first"),
		Control("npc:second"),
	});

	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(
			Input(
				state,
				Catalog({
					Profile("profile:second", Traits(14)),
					Profile("profile:first", Traits(11)),
				}),
				Pools({ StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
				Map()));

	Expect(result.resolvedSubjectCount == 2 && result.profileSubjects.subjects.size() == 2, "multiple actors should resolve two subjects");
	if (result.profileSubjects.subjects.size() == 2) {
		Expect(result.profileSubjects.subjects[0].npcId == Id("npc:first"), "resolved subjects should preserve first actor order");
		Expect(result.profileSubjects.subjects[1].npcId == Id("npc:second"), "resolved subjects should preserve second actor order");
	}
	Expect(result.plannedRequestCount == 2, "multiple actors should feed two planned requests");
	Expect(result.appliedControlCount == 2, "multiple actors should apply two controls");
}

void TestAbsentActorSkippedByDefaultAndIncludedByConfig()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:absent", "profile:absent", { 0.0F, 0.0F }, false) });
	state.npcControls = Controls({ Control("npc:absent") });
	const iggy::NpcAiProfileTraitCatalog catalog = Catalog({ Profile("profile:absent", Traits(13)) });

	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameResult skipped =
		iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(
			Input(state, catalog, Pools(), Map()));
	iggy::NpcAiProfileTraitResolverConfig profileConfig;
	profileConfig.includeAbsentActors = true;
	iggy::NpcMapPlayControlFramePlanConfig controlConfig;
	controlConfig.includeAbsentActors = true;
	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameResult included =
		iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(
			Input(
				state,
				catalog,
				Pools({ StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
				Map(),
				profileConfig,
				controlConfig));

	Expect(skipped.absentSkippedCount == 1 && skipped.resolvedSubjectCount == 0, "absent actor should be skipped by default");
	Expect(included.resolvedSubjectCount == 1 && included.plannedRequestCount == 1, "include absent config should resolve and plan absent actor");
}

void TestNonControlStatePreserved()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:preserved", "profile:preserved") });
	state.npcControls = Controls({ Control("npc:preserved") });

	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(
			Input(
				state,
				Catalog({ Profile("profile:preserved", Traits(12)) }),
				Pools({ StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
				Map()));

	Expect(result.changedControls, "preservation setup should change controls");
	ExpectNonControlStatePreserved(result.state, state);
}

void TestInputsAreNotMutated()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:immutable", "profile:immutable") });
	state.npcControls = Controls({ Control("npc:immutable") });
	iggy::NpcAiProfileTraitCatalog catalog = Catalog({ Profile("profile:immutable", Traits(12)) });
	iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F) });
	iggy::AiMap2D map = Map({ Node("ai:immutable", { 0.0F, 0.0F }, { Id("zone:immutable") }) });
	iggy::NpcAiProfileTraitResolverConfig profileConfig;
	iggy::NpcMapPlayControlFramePlanConfig controlConfig;

	const iggy::runtime::RuntimeGameplayState beforeState = state;
	const iggy::NpcAiProfileTraitCatalog beforeCatalog = catalog;
	const iggy::NpcMapPlayControlFramePlanPools beforePools = pools;
	const iggy::AiMap2D beforeMap = map;
	const iggy::NpcAiProfileTraitResolverConfig beforeProfileConfig = profileConfig;
	const iggy::NpcMapPlayControlFramePlanConfig beforeControlConfig = controlConfig;

	(void)iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(
		Input(state, catalog, pools, map, profileConfig, controlConfig));

	Expect(SameActors(state.npcActors, beforeState.npcActors), "profile control should not mutate input actors");
	Expect(SameControls(state.npcControls, beforeState.npcControls), "profile control should not mutate input controls");
	Expect(catalog.entries.size() == beforeCatalog.entries.size() && catalog.entries[0].profileId == beforeCatalog.entries[0].profileId, "profile control should not mutate catalog");
	Expect(pools.strength.entries.size() == beforePools.strength.entries.size(), "profile control should not mutate pools");
	Expect(map.nodes.size() == beforeMap.nodes.size() && map.nodes[0].id == beforeMap.nodes[0].id, "profile control should not mutate AI map");
	Expect(profileConfig.includeAbsentActors == beforeProfileConfig.includeAbsentActors, "profile control should not mutate profile config");
	Expect(controlConfig.includeAbsentActors == beforeControlConfig.includeAbsentActors, "profile control should not mutate control config");
}

void TestManualCompositionMatchesHelper()
{
	iggy::runtime::RuntimeGameplayState state = GameplayState();
	state.npcActors = Actors({ Actor("npc:manual", "profile:manual", { 0.0F, 0.0F }) });
	state.npcControls = Controls({ Control("npc:manual", iggy::seekingNpcBehaviorState({ 4.0F, 0.0F }), iggy::NpcMoveMode::None) });
	iggy::NpcMapPlayControlFramePlanConfig controlConfig;
	controlConfig.step.mapRead.matchedMapTagBonus = 4.0F;
	const iggy::NpcAiProfileTraitCatalog catalog = Catalog({ Profile("profile:manual", Traits(12)) });
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools({
		StrengthEnt("strength:manual", "action:manual", iggy::NpcBehaviorStateType::Seeking, 1.0F, { Id("zone:manual") }),
	});
	const iggy::AiMap2D map = Map({ Node("ai:manual", { 0.0F, 0.0F }, { Id("zone:manual") }) });

	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameInput input =
		Input(state, catalog, pools, map, {}, controlConfig);
	const iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameResult helper =
		iggy::runtime::RuntimeNpcAiProfileControlPlannedFrameStep {}.run(input);

	const iggy::NpcAiProfileTraitResolveResult manualResolve =
		iggy::NpcAiProfileTraitResolver {}.resolve(state.npcActors, catalog);
	const iggy::runtime::RuntimeNpcAiControlPlannedFrameResult manualControl =
		iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run({
			state,
			manualResolve.subjects,
			pools,
			map,
			controlConfig,
		});

	Expect(SameControls(helper.state.npcControls, manualControl.state.npcControls), "profile helper should match manual composition controls");
	Expect(helper.resolvedSubjectCount == manualResolve.resolvedCount, "profile helper should match manual resolved count");
	Expect(helper.plannedRequestCount == manualControl.plannedRequestCount, "profile helper should match manual planned count");
	Expect(helper.appliedControlCount == manualControl.appliedControlCount, "profile helper should match manual applied count");
	Expect(helper.mapChangedSelectionCount == manualControl.mapChangedSelectionCount, "profile helper should match manual map selection count");
}

} // namespace

int main()
{
	TestEmptyDefaultStateNoOps();
	TestOneActorProfileResolvesAndUpdatesControl();
	TestMissingProfileRecordsIssueAndPreservesControls();
	TestMultipleActorsPreserveActorOrder();
	TestAbsentActorSkippedByDefaultAndIncludedByConfig();
	TestNonControlStatePreserved();
	TestInputsAreNotMutated();
	TestManualCompositionMatchesHelper();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
