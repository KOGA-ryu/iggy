#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioDefinition.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap LevelMap(const char *id, std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = Id(id);
	return map;
}

iggy::NpcTraitSet Traits(int strength = 10)
{
	iggy::NpcTraitSet traits;
	traits.strength = strength;
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
	Expect(result.built, "profile scenario catalog should build");
	return result.catalog;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	const char *profileId,
	iggy::Vec2 position = { 0.5F, 0.5F },
	bool present = true)
{
	return {
		Id(npcId),
		Id(profileId),
		Id("faction:profile-scenario"),
		position,
		Id("goal:profile-scenario"),
		present,
	};
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::Vec2 target = { 2.5F, 0.5F },
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		moveMode,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "profile scenario actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "profile scenario controls should build");
	return result.registry;
}

iggy::NpcStrengthEnt StrengthEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behavior,
	float weight)
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, {} };
}

iggy::NpcMapPlayControlFramePlanPools Pools(std::vector<iggy::NpcStrengthEnt> strength = {})
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strengthBuild =
		iggy::NpcStrengthPoolBuilder {}.build(strength);
	const iggy::NpcDexterityPoolBuildResult dexterityBuild =
		iggy::NpcDexterityPoolBuilder {}.build({});
	const iggy::NpcConstitutionPoolBuildResult constitutionBuild =
		iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligenceBuild =
		iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdomBuild =
		iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charismaBuild =
		iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strengthBuild.built && dexterityBuild.built && constitutionBuild.built && intelligenceBuild.built && wisdomBuild.built && charismaBuild.built, "profile scenario pools should build");
	pools.strength = strengthBuild.pool;
	pools.dexterity = dexterityBuild.pool;
	pools.constitution = constitutionBuild.pool;
	pools.intelligence = intelligenceBuild.pool;
	pools.wisdom = wisdomBuild.pool;
	pools.charisma = charismaBuild.pool;
	return pools;
}

iggy::runtime::RuntimeGameplayState State(
	iggy::NpcActorState2DRegistry actors = {},
	iggy::NpcActorControlState2DRegistry controls = {})
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level.map = LevelMap("level:profile-scenario-initial", { "..." });
	state.npcActors = actors;
	state.npcControls = controls;
	return state;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition Frame(const char *levelId)
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id(levelId);
	frame.pools = Pools({ StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F) });
	frame.movementMap = LevelMap(levelId, { "..." });
	return frame;
}

iggy::runtime::RuntimeGameplayProfileScenarioDefinition Definition(
	iggy::runtime::RuntimeGameplayState state,
	iggy::NpcAiProfileTraitCatalog catalog,
	std::vector<iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition> frames)
{
	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id("scenario:profile");
	definition.initialState = state;
	definition.profileTraits = catalog;
	definition.frames = frames;
	return definition;
}

void TestEmptyProfileScenarioBuildsEmptyNormalScenario()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;

	const iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuildResult result =
		iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(definition);

	Expect(result.built, "empty profile scenario should build");
	Expect(result.frameCount == 0, "empty profile scenario should report zero frames");
	Expect(result.scenario.frames.empty(), "empty profile scenario should produce empty normal scenario");
}

void TestOneActorResolvesSubjectsIntoBuiltScenarioAndRuns()
{
	const iggy::LevelTileMap map = LevelMap("level:profile-run", { "..." });
	iggy::runtime::RuntimeGameplayState state =
		State(
			Actors({ Actor("npc:runner", "profile:runner", { 0.5F, 0.5F }) }),
			Controls({ Control("npc:runner", { 2.5F, 0.5F }, iggy::NpcMoveMode::Still) }));
	state.session.level.map = map;
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			state,
			Catalog({ Profile("profile:runner", Traits(12)) }),
			{ Frame("level:profile-run") });

	const iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuildResult build =
		iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(definition);
	const iggy::runtime::RuntimeGameplayScenarioResult run =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(build.scenario);

	Expect(build.built, "profile scenario should build");
	Expect(build.resolvedSubjectCount == 1, "profile scenario should resolve one subject");
	Expect(build.scenarioDefinition.frames.size() == 1, "profile scenario should build one normal frame");
	Expect(build.scenarioDefinition.frames[0].frame.subjects.size() == 1, "profile scenario should inject one subject");
	Expect(build.scenarioDefinition.frames[0].frame.subjects[0].npcId == Id("npc:runner"), "profile scenario should preserve resolved npc id");
	Expect(run.npcControlAppliedCount == 1 && run.npcMovedCount == 1, "profile scenario should run through existing scenario runner");
	Expect(!run.state.npcActors.actors.empty() && NearVec(run.state.npcActors.actors[0].position, { 1.5F, 0.5F }), "profile scenario run should move actor");
}

void TestMultipleFramesPreserveOrderAndResolvedSubjects()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			State(
				Actors({ Actor("npc:ordered", "profile:ordered") }),
				Controls({ Control("npc:ordered") })),
			Catalog({ Profile("profile:ordered", Traits(11)) }),
			{ Frame("level:first"), Frame("level:second") });

	const iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuildResult result =
		iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(definition);

	Expect(result.frameCount == 2 && result.scenario.frames.size() == 2, "profile scenario should preserve frame count");
	Expect(result.profileFrames.size() == 2, "profile scenario should preserve per-frame resolver diagnostics");
	Expect(result.resolvedSubjectCount == 2, "profile scenario should resolve subjects once per frame");
	if (result.scenarioDefinition.frames.size() == 2) {
		Expect(result.scenarioDefinition.frames[0].frame.movementMap.id == Id("level:first"), "profile scenario should preserve first frame");
		Expect(result.scenarioDefinition.frames[1].frame.movementMap.id == Id("level:second"), "profile scenario should preserve second frame");
	}
}

void TestMissingProfileBuildsNoSubjectForActor()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			State(
				Actors({ Actor("npc:missing", "profile:missing") }),
				Controls({ Control("npc:missing") })),
			Catalog({}),
			{ Frame("level:missing") });

	const iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuildResult result =
		iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(definition);

	Expect(result.missingProfileCount == 1, "missing profile should be counted in build result");
	Expect(result.scenarioDefinition.frames.size() == 1, "missing profile setup should still build frame");
	Expect(result.scenarioDefinition.frames[0].frame.subjects.empty(), "missing profile should not inject subject");
}

void TestBuilderDoesNotMutateDefinition()
{
	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			State(
				Actors({ Actor("npc:copy", "profile:copy") }),
				Controls({ Control("npc:copy") })),
			Catalog({ Profile("profile:copy", Traits(12)) }),
			{ Frame("level:copy") });
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition before = definition;

	const iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuildResult result =
		iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(definition);

	Expect(result.built, "profile scenario immutability setup should build");
	Expect(definition.scenarioId == before.scenarioId, "profile scenario builder should not mutate scenario id");
	Expect(definition.profileTraits.entries.size() == before.profileTraits.entries.size(), "profile scenario builder should not mutate catalog");
	Expect(definition.frames.size() == before.frames.size(), "profile scenario builder should not mutate frame count");
	Expect(definition.frames[0].frameId == before.frames[0].frameId, "profile scenario builder should not mutate frame id");
}

void TestManualResolutionParity()
{
	const iggy::runtime::RuntimeGameplayState state =
		State(
			Actors({ Actor("npc:manual", "profile:manual") }),
			Controls({ Control("npc:manual") }));
	const iggy::NpcAiProfileTraitCatalog catalog = Catalog({ Profile("profile:manual", Traits(12)) });
	const iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition profileFrame = Frame("level:manual");
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(state, catalog, { profileFrame });

	const iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuildResult profileBuild =
		iggy::runtime::RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(definition);
	const iggy::NpcAiProfileTraitResolveResult resolved =
		iggy::NpcAiProfileTraitResolver {}.resolve(state.npcActors, catalog, profileFrame.profileConfig);
	iggy::runtime::RuntimeGameplayScenarioDefinition manualDefinition;
	manualDefinition.hasScenarioId = definition.hasScenarioId;
	manualDefinition.scenarioId = definition.scenarioId;
	manualDefinition.initialState = state;
	manualDefinition.frames = profileBuild.scenarioDefinition.frames;
	manualDefinition.frames[0].frame.subjects = resolved.subjects;
	const iggy::runtime::RuntimeGameplayScenarioDefinitionBuildResult manualBuild =
		iggy::runtime::RuntimeGameplayScenarioDefinitionBuilder {}.build(manualDefinition);

	Expect(profileBuild.resolvedSubjectCount == resolved.resolvedCount, "profile scenario should match manual resolved count");
	Expect(profileBuild.scenario.frames.size() == manualBuild.scenario.frames.size(), "profile scenario should match manual frame count");
	Expect(profileBuild.scenario.frames[0].frame.subjects[0].npcId == manualBuild.scenario.frames[0].frame.subjects[0].npcId, "profile scenario should match manual subject id");
}

} // namespace

int main()
{
	TestEmptyProfileScenarioBuildsEmptyNormalScenario();
	TestOneActorResolvesSubjectsIntoBuiltScenarioAndRuns();
	TestMultipleFramesPreserveOrderAndResolvedSubjects();
	TestMissingProfileBuildsNoSubjectForActor();
	TestBuilderDoesNotMutateDefinition();
	TestManualResolutionParity();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
