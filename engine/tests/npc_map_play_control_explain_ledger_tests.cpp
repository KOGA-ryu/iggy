#include <cstdlib>
#include <vector>

#include "scene/ai/NpcMapPlayControlExplainLedger.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	const char *profileId = "profile:explain",
	iggy::Vec2 position = { 0.0F, 0.0F },
	bool present = true)
{
	return {
		Id(npcId),
		Id(profileId),
		Id("faction:explain"),
		position,
		{},
		present,
	};
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::Vec2 target = { 5.0F, 0.0F })
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		iggy::NpcMoveMode::Walk,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "explain ledger actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "explain ledger controls should build");
	return result.registry;
}

iggy::NpcTraitSet Traits(int strength = 10)
{
	iggy::NpcTraitSet traits;
	traits.strength = strength;
	return traits;
}

iggy::NpcAiProfileTraitCatalog Catalog(std::vector<iggy::NpcAiProfileTraitEntry> entries)
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build(entries);
	Expect(result.built, "explain ledger profile catalog should build");
	return result.catalog;
}

iggy::NpcMapPlayControlFramePlanSubject Subject(const char *npcId)
{
	iggy::NpcMapPlayControlFramePlanSubject subject;
	subject.npcId = Id(npcId);
	subject.traits = Traits();
	return subject;
}

iggy::AiMapNode2D Node(const char *id, std::vector<iggy::ResourceId> tags)
{
	return {
		Id(id),
		{ 0.0F, 0.0F },
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
	Expect(result.built, "explain ledger map should build");
	return result.map;
}

iggy::NpcStrengthEnt StrengthEnt(
	const char *entryId,
	const char *actionTag,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, iggy::NpcBehaviorStateType::Seeking, Id(actionTag), weight, mapTags };
}

iggy::NpcWisdomEnt WisdomEnt(
	const char *entryId,
	const char *actionTag,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, iggy::NpcBehaviorStateType::Seeking, Id(actionTag), weight, mapTags };
}

iggy::NpcMapPlayControlFramePlanPools Pools(
	std::vector<iggy::NpcStrengthEnt> strength = {},
	std::vector<iggy::NpcWisdomEnt> wisdom = {})
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
		iggy::NpcWisdomPoolBuilder {}.build(wisdom);
	const iggy::NpcCharismaPoolBuildResult charismaBuild =
		iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strengthBuild.built && dexterityBuild.built && constitutionBuild.built && intelligenceBuild.built && wisdomBuild.built && charismaBuild.built, "explain ledger pools should build");
	pools.strength = strengthBuild.pool;
	pools.dexterity = dexterityBuild.pool;
	pools.constitution = constitutionBuild.pool;
	pools.intelligence = intelligenceBuild.pool;
	pools.wisdom = wisdomBuild.pool;
	pools.charisma = charismaBuild.pool;
	return pools;
}

iggy::NpcMapPlayControlFramePlanResult Plan(
	const iggy::NpcActorState2DRegistry &actors,
	const iggy::NpcActorControlState2DRegistry &controls,
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> &subjects,
	const iggy::NpcMapPlayControlFramePlanPools &pools,
	const iggy::AiMap2D &map,
	iggy::NpcMapPlayControlFramePlanConfig config = {})
{
	return iggy::NpcMapPlayControlFramePlanner {}.plan(
		actors,
		controls,
		subjects,
		pools,
		map,
		config);
}

iggy::NpcMapPlayControlFrameStep2DResult Step(
	const iggy::NpcActorControlState2DRegistry &controls,
	const iggy::NpcMapPlayControlFramePlanResult &plan)
{
	return iggy::NpcMapPlayControlFrameStepper2D {}.step(
		controls,
		plan.requests,
		plan.config.step);
}

iggy::NpcMapPlayControlExplainLedger Ledger(
	const iggy::NpcMapPlayControlFramePlanResult &plan,
	const iggy::NpcMapPlayControlFrameStep2DResult &step)
{
	return iggy::NpcMapPlayControlExplainLedgerReporter {}.report(plan, step);
}

bool HasEvent(
	const iggy::NpcMapPlayControlExplainLedger &ledger,
	iggy::NpcMapPlayControlExplainEvent event)
{
	for (const iggy::NpcMapPlayControlExplainRow &row : ledger.rows) {
		if (row.event == event)
			return true;
	}
	return false;
}

void TestEmptyPlanAndStepProduceEmptyLedger()
{
	const iggy::NpcMapPlayControlFramePlanResult plan;
	const iggy::NpcMapPlayControlFrameStep2DResult step;

	const iggy::NpcMapPlayControlExplainLedger ledger = Ledger(plan, step);

	Expect(ledger.empty(), "empty explain ledger should be empty");
	Expect(!ledger.hasRows(), "empty explain ledger should not have rows");
	Expect(ledger.plan.requestCount == 0 && ledger.step.requestCount == 0, "empty explain ledger should preserve source facts");
}

void TestProfilePlanAndStepFactsAreExplained()
{
	const iggy::NpcActorState2DRegistry actors =
		Actors({ Actor("npc:guard", "profile:guard") });
	const iggy::NpcActorControlState2DRegistry controls =
		Controls({ Control("npc:guard") });
	const iggy::NpcAiProfileTraitCatalog catalog =
		Catalog({ { Id("profile:guard"), Traits(12) } });
	const iggy::NpcAiProfileTraitResolveResult profile =
		iggy::NpcAiProfileTraitResolver {}.resolve(actors, catalog);
	iggy::NpcMapPlayControlFramePlanConfig config;
	config.step.mapRead.matchedMapTagBonus = 4.0F;
	const iggy::NpcMapPlayControlFramePlanResult plan = Plan(
		actors,
		controls,
		profile.subjects,
		Pools(
			{ StrengthEnt("strength:seek", "action:seek", 2.0F, { Id("zone:cover") }) },
			{ WisdomEnt("wisdom:wait", "action:wait", 5.0F, { Id("zone:loud") }) }),
		Map({ Node("node:cover", { Id("zone:cover") }) }),
		config);
	const iggy::NpcMapPlayControlFrameStep2DResult step = Step(controls, plan);

	const iggy::NpcMapPlayControlExplainLedger ledger =
		iggy::NpcMapPlayControlExplainLedgerReporter {}.report(profile, plan, step);

	Expect(ledger.status == iggy::NpcMapPlayControlExplainLedgerStatus::Reported, "profile explain ledger should report");
	Expect(ledger.profileResolvedCount == 1, "profile explain ledger should count resolved profile");
	Expect(ledger.handDrawnCount == 1 && ledger.mapQueryCount == 1, "profile explain ledger should count hand and map query");
	Expect(ledger.mapChangedSelectionCount == 1, "profile explain ledger should count changed map selection");
	Expect(ledger.playKeptCount == 1 && ledger.controlProposedCount == 1, "profile explain ledger should count kept/proposed control");
	Expect(ledger.controlAppliedCount == 1 && ledger.hasControlChanges(), "profile explain ledger should count applied control");
	Expect(HasEvent(ledger, iggy::NpcMapPlayControlExplainEvent::ProfileResolved), "profile explain ledger should emit profile event");
	Expect(HasEvent(ledger, iggy::NpcMapPlayControlExplainEvent::MapSelectionChanged), "profile explain ledger should emit map selection event");
	Expect(HasEvent(ledger, iggy::NpcMapPlayControlExplainEvent::ControlApplied), "profile explain ledger should emit control applied event");
	if (!ledger.rows.empty()) {
		Expect(ledger.rows[0].event == iggy::NpcMapPlayControlExplainEvent::ProfileResolved, "profile explain ledger should emit profile rows first");
	}
}

void TestMissingAndSkippedInputsAreExplained()
{
	const iggy::NpcActorState2DRegistry actors = Actors({
		Actor("npc:missing-profile", "profile:missing"),
		Actor("npc:absent", "profile:absent", { 1.0F, 0.0F }, false),
	});
	const iggy::NpcActorControlState2DRegistry controls =
		Controls({ Control("npc:missing-profile"), Control("npc:absent") });
	const iggy::NpcAiProfileTraitResolveResult profile =
		iggy::NpcAiProfileTraitResolver {}.resolve(actors, Catalog({}));
	const iggy::NpcMapPlayControlFramePlanResult plan =
		Plan(actors, controls, profile.subjects, Pools(), Map());
	const iggy::NpcMapPlayControlFrameStep2DResult step = Step(controls, plan);

	const iggy::NpcMapPlayControlExplainLedger ledger =
		iggy::NpcMapPlayControlExplainLedgerReporter {}.report(profile, plan, step);

	Expect(ledger.profileMissingCount == 1, "missing profile should be explained");
	Expect(ledger.profileAbsentSkippedCount == 1, "absent profile actor should be explained");
	Expect(ledger.missingTraitCount == 1, "missing trait plan entry should be explained");
	Expect(ledger.actorNotPresentCount == 1, "absent plan entry should be explained");
	Expect(HasEvent(ledger, iggy::NpcMapPlayControlExplainEvent::ProfileMissing), "missing profile event should be present");
	Expect(HasEvent(ledger, iggy::NpcMapPlayControlExplainEvent::ActorNotPresent), "actor-not-present event should be present");
}

void TestFoldedAndFailedProposalAreExplained()
{
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:folded") });
	const iggy::NpcActorControlState2DRegistry controls = Controls({ Control("npc:folded") });
	const iggy::NpcMapPlayControlFramePlanResult plan =
		Plan(
			actors,
			controls,
			{ Subject("npc:folded") },
			Pools(),
			Map());
	const iggy::NpcMapPlayControlFrameStep2DResult step = Step(controls, plan);

	const iggy::NpcMapPlayControlExplainLedger ledger = Ledger(plan, step);

	Expect(ledger.handDrawnCount == 1 && ledger.mapQueryCount == 1, "folded explain ledger should still count prepared request diagnostics");
	Expect(ledger.playFoldedCount == 1, "folded explain ledger should count folded play");
	Expect(ledger.controlFailedCount == 1, "folded explain ledger should count failed control proposal");
	Expect(ledger.controlApplyFailedCount == 1, "folded explain ledger should count failed apply");
	Expect(HasEvent(ledger, iggy::NpcMapPlayControlExplainEvent::PlayFolded), "folded event should be present");
	Expect(HasEvent(ledger, iggy::NpcMapPlayControlExplainEvent::ControlFailed), "control failed event should be present");
}

void TestMissingControlIsExplained()
{
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:no-control") });
	const iggy::NpcActorControlState2DRegistry controls;
	const iggy::NpcMapPlayControlFramePlanResult plan =
		Plan(
			actors,
			controls,
			{ Subject("npc:no-control") },
			Pools({ StrengthEnt("strength:seek", "action:seek", 1.0F) }),
			Map());
	const iggy::NpcMapPlayControlFrameStep2DResult step = Step(controls, plan);

	const iggy::NpcMapPlayControlExplainLedger ledger = Ledger(plan, step);

	Expect(ledger.missingControlCount == 1, "missing control should be counted");
	Expect(HasEvent(ledger, iggy::NpcMapPlayControlExplainEvent::MissingControl), "missing control event should be present");
}

void TestInputsAreNotMutated()
{
	const iggy::NpcActorState2DRegistry actors = Actors({ Actor("npc:copy") });
	const iggy::NpcActorControlState2DRegistry controls = Controls({ Control("npc:copy") });
	const iggy::NpcMapPlayControlFramePlanResult plan =
		Plan(
			actors,
			controls,
			{ Subject("npc:copy") },
			Pools({ StrengthEnt("strength:copy", "action:copy", 1.0F) }),
			Map());
	const iggy::NpcMapPlayControlFrameStep2DResult step = Step(controls, plan);
	const std::size_t planRequestCount = plan.requestCount;
	const std::size_t stepRequestCount = step.requestCount;
	const iggy::NpcActorControlState2DRegistry stepRegistry = step.registry;

	const iggy::NpcMapPlayControlExplainLedger ledger = Ledger(plan, step);

	Expect(ledger.hasRows(), "immutability setup should produce rows");
	Expect(plan.requestCount == planRequestCount, "explain ledger should not mutate plan request count");
	Expect(step.requestCount == stepRequestCount, "explain ledger should not mutate step request count");
	Expect(step.registry.entries.size() == stepRegistry.entries.size(), "explain ledger should not mutate step registry");
}

} // namespace

int main()
{
	TestEmptyPlanAndStepProduceEmptyLedger();
	TestProfilePlanAndStepFactsAreExplained();
	TestMissingAndSkippedInputsAreExplained();
	TestFoldedAndFailedProposalAreExplained();
	TestMissingControlIsExplained();
	TestInputsAreNotMutated();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
