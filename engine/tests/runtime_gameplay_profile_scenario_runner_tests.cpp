#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
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

iggy::LevelTileMap LevelMap(const char *id, std::vector<std::string_view> rows = { "...." })
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
	Expect(result.built, "profile scenario runner catalog should build");
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
		Id("faction:profile-runner"),
		position,
		Id("goal:profile-runner"),
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
	Expect(result.built, "profile scenario runner actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "profile scenario runner controls should build");
	return result.registry;
}

iggy::NpcStrengthEnt StrengthEnt(const char *entryId)
{
	return { Id(entryId), 0, iggy::NpcBehaviorStateType::Seeking, Id("action:profile-runner"), 1.0F, {} };
}

iggy::NpcMapPlayControlFramePlanPools Pools()
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strength =
		iggy::NpcStrengthPoolBuilder {}.build({ StrengthEnt("strength:profile-runner") });
	const iggy::NpcDexterityPoolBuildResult dexterity =
		iggy::NpcDexterityPoolBuilder {}.build({});
	const iggy::NpcConstitutionPoolBuildResult constitution =
		iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligence =
		iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdom =
		iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charisma =
		iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strength.built && dexterity.built && constitution.built && intelligence.built && wisdom.built && charisma.built, "profile scenario runner pools should build");
	pools.strength = strength.pool;
	pools.dexterity = dexterity.pool;
	pools.constitution = constitution.pool;
	pools.intelligence = intelligence.pool;
	pools.wisdom = wisdom.pool;
	pools.charisma = charisma.pool;
	return pools;
}

iggy::runtime::RuntimeGameplayState State(
	iggy::NpcActorState2DRegistry actors = {},
	iggy::NpcActorControlState2DRegistry controls = {})
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.level.map = LevelMap("level:profile-runner-state");
	state.npcActors = actors;
	state.npcControls = controls;
	return state;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition Frame(const char *id)
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id(id);
	frame.pools = Pools();
	frame.movementMap = LevelMap(id);
	return frame;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition FrameWithOverride(
	const char *id,
	iggy::NpcActorControlState2D overrideControl)
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame = Frame(id);
	frame.controlOverrides = { overrideControl };
	return frame;
}

iggy::runtime::RuntimeGameplayProfileScenarioDefinition Definition(
	iggy::runtime::RuntimeGameplayState state,
	iggy::NpcAiProfileTraitCatalog catalog,
	std::vector<iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition> frames)
{
	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id("scenario:profile-runner");
	definition.initialState = state;
	definition.profileTraits = catalog;
	definition.frames = frames;
	return definition;
}

iggy::runtime::RuntimeGameplayProfileScenarioRunResult Run(
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition &definition)
{
	return iggy::runtime::RuntimeGameplayProfileScenarioRunner {}.run(definition);
}

bool HasEvent(
	const iggy::runtime::RuntimeGameplayScenarioLedger &ledger,
	iggy::runtime::RuntimeGameplayScenarioLedgerEvent event)
{
	for (const iggy::runtime::RuntimeGameplayScenarioLedgerEvent actual : ledger.events) {
		if (actual == event)
			return true;
	}
	return false;
}

void TestEmptyDefaultProfileScenarioRunsNoOpLedger()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result = Run(definition);

	Expect(result.ran(), "empty profile scenario should run");
	Expect(result.validation.ok(), "empty profile scenario should validate");
	Expect(result.build.built, "empty profile scenario should build");
	Expect(result.frameCount == 0 && result.ledger.empty(), "empty profile scenario should return empty ledger");
	Expect(result.ledger.hasProfileValidation, "empty profile scenario ledger should preserve validation facts");
	Expect(result.scenario.runner.frameResults.empty(), "empty profile scenario should not create frame results");
}

void TestValidProfileScenarioRunsAndMovesNpc()
{
	const iggy::runtime::RuntimeGameplayState state =
		State(
			Actors({ Actor("npc:runner", "profile:runner", { 0.5F, 0.5F }) }),
			Controls({ Control("npc:runner", { 2.5F, 0.5F }) }));
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			state,
			Catalog({ Profile("profile:runner", Traits(12)) }),
			{ Frame("level:profile-runner-one") });

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result = Run(definition);

	Expect(result.ran(), "valid profile scenario should run");
	Expect(result.validation.ok(), "valid profile scenario should validate");
	Expect(result.build.resolvedSubjectCount == 1, "valid profile scenario should resolve one subject");
	Expect(result.npcControlAppliedCount == 1, "valid profile scenario should apply one control");
	Expect(result.npcMovedCount == 1, "valid profile scenario should move one actor");
	Expect(result.changed(), "valid profile scenario should report changed");
	Expect(HasEvent(result.ledger, iggy::runtime::RuntimeGameplayScenarioLedgerEvent::NpcActorMoved), "valid profile scenario should emit moved ledger event");
	Expect(result.ledger.hasNpcAiExplanations(), "valid profile scenario should expose AI explanation ledger facts");
	Expect(result.ledger.npcAiProfileResolvedCount == 1, "valid profile scenario should explain profile resolution");
	Expect(result.ledger.npcAiControlProposedCount == 1 && result.ledger.npcAiControlAppliedCount == 1, "valid profile scenario should explain control proposal/application");
	Expect(!result.state.npcActors.actors.empty() && NearVec(result.state.npcActors.actors[0].position, { 1.5F, 0.5F }), "valid profile scenario should update actor position");
}

void TestInvalidProfileScenarioDoesNotRunGameplay()
{
	const iggy::runtime::RuntimeGameplayState state =
		State(
			Actors({ Actor("npc:missing", "profile:missing", { 0.5F, 0.5F }) }),
			Controls({ Control("npc:missing", { 2.5F, 0.5F }) }));
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			state,
			Catalog({}),
			{ Frame("level:profile-runner-missing") });

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result = Run(definition);

	Expect(!result.ran(), "invalid profile scenario should not run");
	Expect(result.status == iggy::runtime::RuntimeGameplayProfileScenarioRunStatus::ValidationFailed, "invalid profile scenario should preserve failed status");
	Expect(!result.validation.ok(), "invalid profile scenario should preserve validation failure");
	Expect(result.validation.missingProfileTraitCount == 1, "invalid profile scenario should preserve missing profile count");
	Expect(result.scenario.runner.frameResults.empty(), "invalid profile scenario should not run ordinary scenario");
	Expect(result.npcMovedCount == 0 && result.changedFrameCount == 0, "invalid profile scenario should not mirror movement changes");
	Expect(!result.state.npcActors.actors.empty() && NearVec(result.state.npcActors.actors[0].position, { 0.5F, 0.5F }), "invalid profile scenario should preserve input actor state");
	Expect(result.ledger.hasProfileValidation, "invalid profile scenario ledger should preserve validation facts");
}

void TestMultiFrameScenarioPreservesLedgerRows()
{
	const iggy::runtime::RuntimeGameplayState state =
		State(
			Actors({ Actor("npc:multi", "profile:multi", { 0.5F, 0.5F }) }),
			Controls({ Control("npc:multi", { 2.5F, 0.5F }) }));
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			state,
			Catalog({ Profile("profile:multi", Traits(12)) }),
			{ Frame("level:profile-runner-first"), Frame("level:profile-runner-second") });

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result = Run(definition);

	Expect(result.ran(), "multi-frame profile scenario should run");
	Expect(result.frameCount == 2, "multi-frame profile scenario should report two frames");
	Expect(result.ledger.frames.size() == 2, "multi-frame profile scenario should preserve ledger rows");
	if (result.ledger.frames.size() == 2) {
		Expect(result.ledger.frames[0].frameIndex == 0, "multi-frame profile scenario should preserve first row index");
		Expect(result.ledger.frames[1].frameIndex == 1, "multi-frame profile scenario should preserve second row index");
	}
	Expect(result.npcMovedCount == 2, "multi-frame profile scenario should aggregate moved count");
	Expect(!result.state.npcActors.actors.empty() && NearVec(result.state.npcActors.actors[0].position, { 2.5F, 0.5F }), "multi-frame profile scenario should carry actor across frames");
}

void TestFrameControlOverridesMoveNpcAcrossFrames()
{
	const iggy::runtime::RuntimeGameplayState state =
		State(
			Actors({ Actor("npc:override", "profile:override", { 1.5F, 0.5F }) }),
			Controls({ Control("npc:override", { 1.5F, 0.5F }, iggy::NpcMoveMode::Still) }));
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			state,
			Catalog({ Profile("profile:override", Traits(12)) }),
			{
				FrameWithOverride(
					"frame:override-one",
					Control("npc:override", { 2.5F, 0.5F }, iggy::NpcMoveMode::Walk)),
				FrameWithOverride(
					"frame:override-two",
					Control("npc:override", { 3.5F, 0.5F }, iggy::NpcMoveMode::Walk)),
			});

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result = Run(definition);

	Expect(result.ran(), "profile scenario frame overrides should run");
	Expect(result.validation.ok(), "profile scenario frame overrides should validate");
	Expect(result.frameCount == 2, "profile scenario frame overrides should preserve frame count");
	Expect(result.npcMovedCount == 2, "profile scenario frame overrides should move once per frame");
	Expect(result.npcActorsChanged, "profile scenario frame overrides should mark actor changes");
	Expect(result.build.scenarioDefinition.frames.size() == 2, "profile scenario frame overrides should build two ordinary frames");
	if (result.build.scenarioDefinition.frames.size() == 2) {
		Expect(result.build.scenarioDefinition.frames[0].frame.controlOverrides.size() == 1, "first ordinary frame should preserve override");
		Expect(result.build.scenarioDefinition.frames[1].frame.controlOverrides.size() == 1, "second ordinary frame should preserve override");
		Expect(result.build.scenarioDefinition.frames[0].frame.controlOverrides[0].objective.targetPosition.x == 2.5F, "first ordinary frame should preserve first target");
		Expect(result.build.scenarioDefinition.frames[1].frame.controlOverrides[0].objective.targetPosition.x == 3.5F, "second ordinary frame should preserve second target");
	}
	Expect(!result.state.npcActors.actors.empty() && NearVec(result.state.npcActors.actors[0].position, { 3.5F, 0.5F }), "profile scenario frame overrides should carry actor to final target");
}

void TestManualCompositionParity()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			State(
				Actors({ Actor("npc:manual", "profile:manual", { 0.5F, 0.5F }) }),
				Controls({ Control("npc:manual", { 2.5F, 0.5F }) })),
			Catalog({ Profile("profile:manual", Traits(12)) }),
			{ Frame("level:profile-runner-manual") });

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result = Run(definition);
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult validation =
		iggy::runtime::RuntimeGameplayProfileScenarioValidator {}.validate(definition);
	const iggy::runtime::RuntimeGameplayScenarioResult scenario =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(validation.build.scenario);
	const iggy::runtime::RuntimeGameplayScenarioLedger ledger =
		iggy::runtime::RuntimeGameplayScenarioLedgerReporter {}.report(validation, scenario);

	Expect(result.ran() && validation.ok(), "manual parity setup should run");
	Expect(result.npcMovedCount == scenario.npcMovedCount, "profile runner should match manual scenario moved count");
	Expect(result.npcControlAppliedCount == scenario.npcControlAppliedCount, "profile runner should match manual control count");
	Expect(result.ledger.frameCount == ledger.frameCount, "profile runner should match manual ledger frame count");
	Expect(result.ledger.npcAiProfileResolvedCount == ledger.npcAiProfileResolvedCount, "profile runner should match manual AI explanation profile count");
	Expect(result.ledger.npcAiControlAppliedCount == ledger.npcAiControlAppliedCount, "profile runner should match manual AI explanation apply count");
	Expect(!result.state.npcActors.actors.empty() && !scenario.state.npcActors.actors.empty()
		&& NearVec(result.state.npcActors.actors[0].position, scenario.state.npcActors.actors[0].position), "profile runner should match manual final actor position");
}

void TestNamespacedAndUnqualifiedIdsRemainExact()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			State(
				Actors({ Actor("npc:guard", "profile:guard") }),
				Controls({ Control("npc:guard") })),
			Catalog({ Profile("guard", Traits(12)) }),
			{ Frame("level:profile-runner-id") });

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result = Run(definition);

	Expect(!result.ran(), "profile runner should not collapse namespaced and unqualified profile ids");
	Expect(result.validation.missingProfileTraitCount == 1, "profile runner should preserve exact missing profile diagnostic");
	Expect(result.validation.issues[0].profileId == Id("profile:guard"), "profile runner should preserve namespaced profile id");
}

void TestRunnerDoesNotMutateDefinition()
{
	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition =
		Definition(
			State(
				Actors({ Actor("npc:copy", "profile:copy") }),
				Controls({ Control("npc:copy") })),
			Catalog({ Profile("profile:copy", Traits(12)) }),
			{ Frame("level:profile-runner-copy") });
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition before = definition;

	const iggy::runtime::RuntimeGameplayProfileScenarioRunResult result = Run(definition);

	Expect(result.ran(), "profile runner immutability setup should run");
	Expect(definition.scenarioId == before.scenarioId, "profile runner should not mutate scenario id");
	Expect(definition.profileTraits.entries.size() == before.profileTraits.entries.size(), "profile runner should not mutate catalog");
	Expect(definition.frames.size() == before.frames.size(), "profile runner should not mutate frames");
	Expect(definition.frames[0].frameId == before.frames[0].frameId, "profile runner should not mutate frame id");
	Expect(definition.frames[0].controlOverrides.size() == before.frames[0].controlOverrides.size(), "profile runner should not mutate frame overrides");
}

} // namespace

int main()
{
	TestEmptyDefaultProfileScenarioRunsNoOpLedger();
	TestValidProfileScenarioRunsAndMovesNpc();
	TestInvalidProfileScenarioDoesNotRunGameplay();
	TestMultiFrameScenarioPreservesLedgerRows();
	TestFrameControlOverridesMoveNpcAcrossFrames();
	TestManualCompositionParity();
	TestNamespacedAndUnqualifiedIdsRemainExact();
	TestRunnerDoesNotMutateDefinition();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
