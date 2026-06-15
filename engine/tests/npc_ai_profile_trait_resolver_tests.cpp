#include <cstdlib>
#include <vector>

#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcAiProfileTraitResolver.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
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
	Expect(result.built, "profile trait test catalog should build");
	return result.catalog;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	const char *profileId,
	bool present = true)
{
	return {
		Id(npcId),
		Id(profileId),
		Id("faction:profile-trait"),
		{ 0.5F, 0.5F },
		Id("goal:profile-trait"),
		present,
	};
}

iggy::NpcActorState2D ActorWithEmptyProfile(const char *npcId)
{
	iggy::NpcActorState2D actor = Actor(npcId, "profile:placeholder");
	actor.aiProfileId = {};
	return actor;
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "profile trait resolver actors should build");
	return result.registry;
}

iggy::NpcActorControlState2D Control(const char *npcId)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective({ 3.5F, 0.5F }),
		iggy::seekingNpcBehaviorState({ 3.5F, 0.5F }),
		iggy::NpcMoveMode::Walk,
	};
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "profile trait resolver controls should build");
	return result.registry;
}

iggy::NpcMapPlayControlFramePlanPools EmptyPools()
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strength = iggy::NpcStrengthPoolBuilder {}.build({});
	const iggy::NpcDexterityPoolBuildResult dexterity = iggy::NpcDexterityPoolBuilder {}.build({});
	const iggy::NpcConstitutionPoolBuildResult constitution = iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligence = iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdom = iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charisma = iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strength.built && dexterity.built && constitution.built && intelligence.built && wisdom.built && charisma.built, "empty profile trait pools should build");
	pools.strength = strength.pool;
	pools.dexterity = dexterity.pool;
	pools.constitution = constitution.pool;
	pools.intelligence = intelligence.pool;
	pools.wisdom = wisdom.pool;
	pools.charisma = charisma.pool;
	return pools;
}

iggy::AiMap2D AiMap()
{
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build({});
	Expect(result.built, "profile trait AI map should build");
	return result.map;
}

iggy::NpcAiProfileTraitResolveResult Resolve(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcAiProfileTraitCatalog &catalog,
	const iggy::NpcAiProfileTraitResolverConfig &config = {})
{
	return iggy::NpcAiProfileTraitResolver {}.resolve(actors, catalog, config);
}

void TestEmptyCatalogReportsMissingProfiles()
{
	const iggy::NpcActorState2DRegistry actors =
		Actors({ Actor("npc:missing", "profile:missing") });
	const iggy::NpcAiProfileTraitCatalog catalog = Catalog({});

	const iggy::NpcAiProfileTraitResolveResult result = Resolve(actors, catalog);

	Expect(!result.hasSubjects(), "empty catalog should produce no subjects");
	Expect(result.missingProfileCount == 1, "empty catalog should report missing profile for present actor");
	Expect(result.hasIssues(), "empty catalog missing profile should preserve issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::NpcAiProfileTraitResolveIssueCode::MissingProfile, "missing profile issue should use MissingProfile");
		Expect(result.issues[0].profileId == Id("profile:missing"), "missing profile issue should preserve profile id");
	}
}

void TestResolverMapsProfilesToSubjectsInActorOrder()
{
	const iggy::NpcActorState2DRegistry actors =
		Actors({
			Actor("npc:first", "profile:first"),
			Actor("npc:second", "profile:second"),
		});
	const iggy::NpcAiProfileTraitCatalog catalog =
		Catalog({
			Profile("profile:second", Traits(14)),
			Profile("profile:first", Traits(11)),
		});

	const iggy::NpcAiProfileTraitResolveResult result = Resolve(actors, catalog);

	Expect(result.resolvedCount == 2 && result.subjects.size() == 2, "resolver should resolve both actors");
	if (result.subjects.size() == 2) {
		Expect(result.subjects[0].npcId == Id("npc:first") && result.subjects[0].traits.strength == 11, "resolver should preserve first actor order and traits");
		Expect(result.subjects[1].npcId == Id("npc:second") && result.subjects[1].traits.strength == 14, "resolver should preserve second actor order and traits");
	}
}

void TestEmptyActorProfileIdRecordsIssue()
{
	const iggy::NpcActorState2DRegistry actors {
		{ ActorWithEmptyProfile("npc:empty-profile") },
	};
	const iggy::NpcAiProfileTraitCatalog catalog = Catalog({});

	const iggy::NpcAiProfileTraitResolveResult result = Resolve(actors, catalog);

	Expect(result.emptyActorProfileIdCount == 1, "empty actor profile id should increment count");
	Expect(result.subjects.empty(), "empty actor profile id should skip subject");
	Expect(!result.issues.empty() && result.issues[0].code == iggy::NpcAiProfileTraitResolveIssueCode::EmptyActorProfileId, "empty actor profile id should preserve issue");
}

void TestAbsentActorsSkippedByDefaultAndIncludedByConfig()
{
	const iggy::NpcActorState2DRegistry actors =
		Actors({ Actor("npc:absent", "profile:absent", false) });
	const iggy::NpcAiProfileTraitCatalog catalog =
		Catalog({ Profile("profile:absent", Traits(13)) });

	const iggy::NpcAiProfileTraitResolveResult skipped = Resolve(actors, catalog);
	iggy::NpcAiProfileTraitResolverConfig config;
	config.includeAbsentActors = true;
	const iggy::NpcAiProfileTraitResolveResult included = Resolve(actors, catalog, config);

	Expect(skipped.absentSkippedCount == 1 && skipped.subjects.empty(), "absent actor should be skipped by default");
	Expect(included.resolvedCount == 1 && included.subjects.size() == 1, "include absent config should resolve absent actor");
}

void TestNamespacedAndUnqualifiedProfileIdsDistinct()
{
	const iggy::NpcActorState2DRegistry actors =
		Actors({
			Actor("npc:namespaced", "profile:guard"),
			Actor("npc:unqualified", "guard"),
		});
	const iggy::NpcAiProfileTraitCatalog catalog =
		Catalog({
			Profile("profile:guard", Traits(12)),
			Profile("guard", Traits(15)),
		});

	const iggy::NpcAiProfileTraitResolveResult result = Resolve(actors, catalog);

	Expect(result.subjects.size() == 2, "namespaced and unqualified profiles should both resolve");
	if (result.subjects.size() == 2) {
		Expect(result.subjects[0].traits.strength == 12, "namespaced profile should use namespaced traits");
		Expect(result.subjects[1].traits.strength == 15, "unqualified profile should use unqualified traits");
	}
}

void TestResolvedSubjectsFeedMapPlayControlFramePlanner()
{
	const iggy::NpcActorState2DRegistry actors =
		Actors({ Actor("npc:planner", "profile:planner") });
	const iggy::NpcActorControlState2DRegistry controls =
		Controls({ Control("npc:planner") });
	const iggy::NpcAiProfileTraitCatalog catalog =
		Catalog({ Profile("profile:planner", Traits(16)) });
	const iggy::NpcAiProfileTraitResolveResult resolved = Resolve(actors, catalog);

	const iggy::NpcMapPlayControlFramePlanResult plan =
		iggy::NpcMapPlayControlFramePlanner {}.plan(
			actors,
			controls,
			resolved.subjects,
			EmptyPools(),
			AiMap());

	Expect(plan.hasRequests(), "resolved profile trait subjects should feed map play control planner");
	Expect(plan.requests.size() == 1 && plan.requests[0].npcId == Id("npc:planner"), "planner request should preserve npc id from resolved subject");
}

void TestResolverDoesNotMutateInputs()
{
	const iggy::NpcActorState2DRegistry actors =
		Actors({ Actor("npc:immutable", "profile:immutable") });
	const iggy::NpcAiProfileTraitCatalog catalog =
		Catalog({ Profile("profile:immutable", Traits(17)) });
	const iggy::NpcActorState2DRegistry beforeActors = actors;
	const iggy::NpcAiProfileTraitCatalog beforeCatalog = catalog;

	const iggy::NpcAiProfileTraitResolveResult result = Resolve(actors, catalog);

	Expect(result.hasSubjects(), "immutability setup should resolve subject");
	Expect(actors.actors[0].npcId == beforeActors.actors[0].npcId && actors.actors[0].aiProfileId == beforeActors.actors[0].aiProfileId, "resolver should not mutate actors");
	Expect(catalog.entries[0].profileId == beforeCatalog.entries[0].profileId && catalog.entries[0].traits.strength == beforeCatalog.entries[0].traits.strength, "resolver should not mutate catalog");
}

} // namespace

int main()
{
	TestEmptyCatalogReportsMissingProfiles();
	TestResolverMapsProfilesToSubjectsInActorOrder();
	TestEmptyActorProfileIdRecordsIssue();
	TestAbsentActorsSkippedByDefaultAndIncludedByConfig();
	TestNamespacedAndUnqualifiedProfileIdsDistinct();
	TestResolvedSubjectsFeedMapPlayControlFramePlanner();
	TestResolverDoesNotMutateInputs();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
