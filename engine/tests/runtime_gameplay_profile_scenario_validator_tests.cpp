#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "runtime/RuntimeGameplayScenarioRunner.hpp"
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

iggy::LevelTileMap LevelMap(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = Id("level:profile-scenario-validator");
	return map;
}

iggy::LevelTileMap InvalidMap()
{
	iggy::LevelTileMap map;
	map.id = Id("level:profile-invalid");
	map.width = 2;
	map.height = 2;
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
	Expect(result.built, "profile scenario validator catalog should build");
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
		Id("faction:profile-scenario-validator"),
		position,
		Id("goal:profile-scenario-validator"),
		present,
	};
}

iggy::NpcActorState2D ActorWithEmptyProfile(const char *npcId)
{
	iggy::NpcActorState2D actor = Actor(npcId, "profile:placeholder");
	actor.aiProfileId = {};
	return actor;
}

iggy::NpcActorControlState2D Control(const char *npcId, iggy::Vec2 target = { 2.5F, 0.5F })
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		iggy::NpcMoveMode::Still,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "profile scenario validator actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "profile scenario validator controls should build");
	return result.registry;
}

iggy::NpcStrengthEnt StrengthEnt()
{
	return { Id("strength:validator"), 0, iggy::NpcBehaviorStateType::Seeking, Id("action:validator"), 1.0F, {} };
}

iggy::NpcMapPlayControlFramePlanPools Pools()
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strengthBuild =
		iggy::NpcStrengthPoolBuilder {}.build({ StrengthEnt() });
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
	Expect(strengthBuild.built && dexterityBuild.built && constitutionBuild.built && intelligenceBuild.built && wisdomBuild.built && charismaBuild.built, "profile scenario validator pools should build");
	pools.strength = strengthBuild.pool;
	pools.dexterity = dexterityBuild.pool;
	pools.constitution = constitutionBuild.pool;
	pools.intelligence = intelligenceBuild.pool;
	pools.wisdom = wisdomBuild.pool;
	pools.charisma = charismaBuild.pool;
	return pools;
}

iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition Frame(iggy::LevelTileMap map = LevelMap({ "..." }))
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:valid");
	frame.pools = Pools();
	frame.movementMap = map;
	return frame;
}

iggy::runtime::RuntimeGameplayProfileScenarioDefinition Definition(
	iggy::NpcActorState2DRegistry actors = {},
	iggy::NpcActorControlState2DRegistry controls = {},
	iggy::NpcAiProfileTraitCatalog catalog = {},
	std::vector<iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition> frames = {})
{
	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = true;
	definition.scenarioId = Id("scenario:valid-profile");
	definition.initialState.npcActors = actors;
	definition.initialState.npcControls = controls;
	definition.profileTraits = catalog;
	definition.frames = frames;
	return definition;
}

iggy::runtime::RuntimeGameplayProfileScenarioValidationResult Validate(
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition &definition)
{
	return iggy::runtime::RuntimeGameplayProfileScenarioValidator {}.validate(definition);
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult &result,
	iggy::runtime::RuntimeGameplayProfileScenarioIssueCode code,
	std::size_t frameIndex)
{
	for (const iggy::runtime::RuntimeGameplayProfileScenarioIssue &issue : result.issues) {
		if (issue.code == code && issue.frameIndex == frameIndex)
			return true;
	}
	return false;
}

void TestEmptyDefaultProfileScenarioValidatesAndRuns()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition;

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult validation = Validate(definition);
	const iggy::runtime::RuntimeGameplayScenarioResult run =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(validation.build.scenario);

	Expect(validation.ok(), "empty profile scenario should validate");
	Expect(validation.frameCount == 0, "empty profile scenario should report zero frames");
	Expect(run.runner.frameResults.empty(), "empty profile scenario should run as no-op");
}

void TestValidProfileScenarioValidates()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition = Definition(
		Actors({ Actor("npc:valid", "profile:valid") }),
		Controls({ Control("npc:valid") }),
		Catalog({ Profile("profile:valid", Traits(12)) }),
		{ Frame() });

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult result = Validate(definition);

	Expect(result.ok(), "valid profile scenario should validate");
	Expect(result.build.resolvedSubjectCount == 1, "valid profile scenario should resolve one subject");
	Expect(result.scenario.ok(), "valid profile scenario should preserve nested valid scenario");
}

void TestMissingProfileTraitIssue()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition = Definition(
		Actors({ Actor("npc:missing", "profile:missing") }),
		Controls({ Control("npc:missing") }),
		Catalog({}),
		{ Frame() });

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "missing profile scenario should be invalid");
	Expect(result.missingProfileTraitCount == 1, "missing profile should increment profile issue count");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::MissingProfileTrait, "missing profile should be first profile issue");
	Expect(result.issues[0].npcId == Id("npc:missing"), "missing profile should preserve npc id");
	Expect(result.issues[0].profileId == Id("profile:missing"), "missing profile should preserve profile id");
	Expect(result.nestedScenarioIssueCount == 1, "missing profile should preserve nested missing subject issue");
}

void TestEmptyActorProfileIdIssue()
{
	iggy::NpcActorState2DRegistry actors;
	actors.actors = { ActorWithEmptyProfile("npc:empty-profile") };
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition = Definition(
		actors,
		Controls({ Control("npc:empty-profile") }),
		Catalog({}),
		{ Frame() });

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "empty actor profile scenario should be invalid");
	Expect(result.emptyActorProfileIdCount == 1, "empty actor profile should increment profile issue count");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::EmptyActorProfileId, "empty actor profile should be first profile issue");
	Expect(result.issues[0].npcId == Id("npc:empty-profile"), "empty actor profile should preserve npc id");
}

void TestAbsentActorPolicy()
{
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition defaultFrame = Frame();
	iggy::runtime::RuntimeGameplayProfileScenarioFrameDefinition includeFrame = Frame();
	includeFrame.profileConfig.includeAbsentActors = true;
	includeFrame.controlConfig.includeAbsentActors = true;
	const iggy::NpcActorState2DRegistry actors =
		Actors({ Actor("npc:absent", "profile:absent", { 0.5F, 0.5F }, false) });
	const iggy::NpcActorControlState2DRegistry controls =
		Controls({ Control("npc:absent") });
	const iggy::NpcAiProfileTraitCatalog catalog =
		Catalog({ Profile("profile:absent", Traits(13)) });

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult skipped =
		Validate(Definition(actors, controls, catalog, { defaultFrame }));
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult included =
		Validate(Definition(actors, controls, catalog, { includeFrame }));

	Expect(skipped.ok(), "absent actor skipped by profile resolver should validate");
	Expect(skipped.build.absentSkippedCount == 1 && skipped.build.resolvedSubjectCount == 0, "absent actor should be skipped by default");
	Expect(included.ok(), "include absent profile scenario should validate");
	Expect(included.build.resolvedSubjectCount == 1, "include absent profile config should resolve subject");
}

void TestNestedScenarioIssuesArePreserved()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition missingControl = Definition(
		Actors({ Actor("npc:missing-control", "profile:valid") }),
		Controls({}),
		Catalog({ Profile("profile:valid", Traits(12)) }),
		{ Frame() });
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition invalidMap = Definition(
		Actors({ Actor("npc:map", "profile:valid") }),
		Controls({ Control("npc:map") }),
		Catalog({ Profile("profile:valid", Traits(12)) }),
		{ Frame(InvalidMap()) });

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult missingControlResult =
		Validate(missingControl);
	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult invalidMapResult =
		Validate(invalidMap);

	Expect(!missingControlResult.ok(), "missing control should invalidate profile scenario through nested validator");
	Expect(missingControlResult.nestedScenarioIssueCount == 1, "missing control should preserve one nested issue");
	Expect(missingControlResult.issues[0].code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid, "missing control should be nested issue");
	Expect(missingControlResult.issues[0].nestedIssue.code == iggy::runtime::RuntimeGameplayScenarioIssueCode::MissingControl, "missing control nested issue should preserve code");
	Expect(!invalidMapResult.ok(), "invalid map should invalidate profile scenario through nested validator");
	Expect(HasIssue(invalidMapResult, iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid, 0), "invalid map should preserve frame index");
}

void TestIssueOrdering()
{
	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition = Definition(
		Actors({ Actor("npc:ordered", "profile:missing") }),
		Controls({ Control("npc:ordered") }),
		Catalog({}),
		{ Frame(InvalidMap()) });
	definition.hasScenarioId = true;
	definition.scenarioId = {};
	definition.frames[0].hasFrameId = true;
	definition.frames[0].frameId = {};

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult result = Validate(definition);

	Expect(result.issues.size() >= 4, "ordered profile scenario should produce multiple issues");
	if (result.issues.size() >= 4) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::EmptyScenarioId, "empty scenario id should be first");
		Expect(result.issues[1].code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::EmptyFrameId, "empty frame id should be second");
		Expect(result.issues[2].code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::MissingProfileTrait, "profile resolution issue should follow id issues");
		Expect(result.issues[3].code == iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid, "nested issues should follow profile issues");
	}
}

void TestNamespacedAndUnqualifiedProfileIdsRemainDistinct()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition = Definition(
		Actors({ Actor("npc:guard", "profile:guard") }),
		Controls({ Control("npc:guard") }),
		Catalog({ Profile("guard", Traits(12)) }),
		{ Frame() });

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult result = Validate(definition);

	Expect(!result.ok(), "namespaced and unqualified profile ids should remain distinct");
	Expect(result.missingProfileTraitCount == 1, "namespaced actor should not match unqualified profile");
}

void TestValidatorDoesNotMutateDefinition()
{
	iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition = Definition(
		Actors({ Actor("npc:copy", "profile:copy") }),
		Controls({ Control("npc:copy") }),
		Catalog({ Profile("profile:copy", Traits(12)) }),
		{ Frame() });
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition before = definition;

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult result = Validate(definition);

	Expect(result.ok(), "profile scenario immutability setup should validate");
	Expect(definition.scenarioId == before.scenarioId, "profile validator should not mutate scenario id");
	Expect(definition.profileTraits.entries[0].profileId == before.profileTraits.entries[0].profileId, "profile validator should not mutate catalog");
	Expect(definition.frames[0].frameId == before.frames[0].frameId, "profile validator should not mutate frame id");
}

void TestValidProfileScenarioRunsThroughScenarioRunner()
{
	const iggy::runtime::RuntimeGameplayProfileScenarioDefinition definition = Definition(
		Actors({ Actor("npc:acceptance", "profile:acceptance", { 0.5F, 0.5F }) }),
		Controls({ Control("npc:acceptance", { 2.5F, 0.5F }) }),
		Catalog({ Profile("profile:acceptance", Traits(12)) }),
		{ Frame(LevelMap({ "..." })) });

	const iggy::runtime::RuntimeGameplayProfileScenarioValidationResult validation = Validate(definition);
	const iggy::runtime::RuntimeGameplayScenarioResult run =
		iggy::runtime::RuntimeGameplayScenarioRunner {}.run(validation.build.scenario);

	Expect(validation.ok(), "profile scenario acceptance should validate");
	Expect(run.npcMovedCount == 1, "profile scenario acceptance should move NPC");
	Expect(run.state.npcActors.actors.size() == 1 && NearVec(run.state.npcActors.actors[0].position, { 1.5F, 0.5F }), "profile scenario acceptance should update actor position");
}

} // namespace

int main()
{
	TestEmptyDefaultProfileScenarioValidatesAndRuns();
	TestValidProfileScenarioValidates();
	TestMissingProfileTraitIssue();
	TestEmptyActorProfileIdIssue();
	TestAbsentActorPolicy();
	TestNestedScenarioIssuesArePreserved();
	TestIssueOrdering();
	TestNamespacedAndUnqualifiedProfileIdsRemainDistinct();
	TestValidatorDoesNotMutateDefinition();
	TestValidProfileScenarioRunsThroughScenarioRunner();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
