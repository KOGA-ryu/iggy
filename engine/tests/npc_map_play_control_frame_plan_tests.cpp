#include <cstdlib>
#include <vector>

#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

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
		Id("profile:basic"),
		Id("faction:neutral"),
		position,
		{},
		present,
	};
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::NpcBehaviorState behavior = iggy::seekingNpcBehaviorState({ 5.0F, 0.0F }),
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Walk)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(behavior.targetPosition),
		behavior,
		moveMode,
	};
}

iggy::NpcMapPlayControlFramePlanSubject Subject(
	const char *npcId,
	iggy::NpcTraitSet traits = {})
{
	iggy::NpcMapPlayControlFramePlanSubject subject;
	subject.npcId = Id(npcId);
	subject.traits = traits;
	return subject;
}

iggy::NpcMapPlayControlFramePlanSubject SubjectWithTarget(
	const char *npcId,
	iggy::Vec2 position)
{
	iggy::NpcMapPlayControlFramePlanSubject subject = Subject(npcId);
	subject.hasProposalContextOverride = true;
	subject.proposalContext.hasTargetPosition = true;
	subject.proposalContext.targetPosition = position;
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
	const iggy::AiMap2DBuildResult build = iggy::AiMap2DBuilder {}.build(nodes);
	Expect(build.built, "test ai map should build");
	return build.map;
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

iggy::NpcConstitutionEnt ConstitutionEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behavior,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, mapTags };
}

iggy::NpcIntelligenceEnt IntelligenceEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behavior,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, mapTags };
}

iggy::NpcWisdomEnt WisdomEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behavior,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, mapTags };
}

iggy::NpcCharismaEnt CharismaEnt(
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
	std::vector<iggy::NpcDexterityEnt> dexterity = {},
	std::vector<iggy::NpcConstitutionEnt> constitution = {},
	std::vector<iggy::NpcIntelligenceEnt> intelligence = {},
	std::vector<iggy::NpcWisdomEnt> wisdom = {},
	std::vector<iggy::NpcCharismaEnt> charisma = {})
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strengthBuild =
		iggy::NpcStrengthPoolBuilder {}.build(strength);
	const iggy::NpcDexterityPoolBuildResult dexterityBuild =
		iggy::NpcDexterityPoolBuilder {}.build(dexterity);
	const iggy::NpcConstitutionPoolBuildResult constitutionBuild =
		iggy::NpcConstitutionPoolBuilder {}.build(constitution);
	const iggy::NpcIntelligencePoolBuildResult intelligenceBuild =
		iggy::NpcIntelligencePoolBuilder {}.build(intelligence);
	const iggy::NpcWisdomPoolBuildResult wisdomBuild =
		iggy::NpcWisdomPoolBuilder {}.build(wisdom);
	const iggy::NpcCharismaPoolBuildResult charismaBuild =
		iggy::NpcCharismaPoolBuilder {}.build(charisma);

	Expect(strengthBuild.built, "strength pool should build");
	Expect(dexterityBuild.built, "dexterity pool should build");
	Expect(constitutionBuild.built, "constitution pool should build");
	Expect(intelligenceBuild.built, "intelligence pool should build");
	Expect(wisdomBuild.built, "wisdom pool should build");
	Expect(charismaBuild.built, "charisma pool should build");

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
	const iggy::NpcMapPlayControlFramePlanConfig &config = {})
{
	return iggy::NpcMapPlayControlFramePlanner {}.plan(
		actors,
		controls,
		subjects,
		pools,
		map,
		config);
}

void TestEmptyInputsProduceNoRequests()
{
	const iggy::NpcActorState2DRegistry actors;
	const iggy::NpcActorControlState2DRegistry controls;
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects;
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools();
	const iggy::AiMap2D map = Map();

	const iggy::NpcMapPlayControlFramePlanResult result =
		Plan(actors, controls, subjects, pools, map);

	Expect(result.status == iggy::NpcMapPlayControlFramePlanStatus::NoRequests, "empty plan should have NoRequests status");
	Expect(!result.hasRequests(), "empty plan should have no requests");
	Expect(result.frameEntryCount == 0 && result.subjectCount == 0, "empty plan should count no frames or subjects");
	Expect(result.entries.empty() && result.issues.empty(), "empty plan should have no entries or issues");
	Expect(result.requests.empty(), "empty plan should preserve empty request vector");
}

void TestOneActorPreparesRequestAndQueriesMap()
{
	const iggy::NpcActorState2DRegistry actors { { Actor("npc:guard", { 1.0F, 0.0F }) } };
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:guard") } };
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects { Subject("npc:guard") };
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools({
		StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }),
	});
	const iggy::AiMap2D map = Map({ Node("ai:cover", { 1.0F, 0.0F }, { Id("zone:cover") }) });

	const iggy::NpcMapPlayControlFramePlanResult result =
		Plan(actors, controls, subjects, pools, map);

	Expect(result.status == iggy::NpcMapPlayControlFramePlanStatus::Planned, "matching actor should plan request");
	Expect(result.requestCount == 1 && result.preparedCount == 1, "matching actor should count one prepared request");
	Expect(result.mapQueryCount == 1, "matching actor should run one ai map query");
	Expect(result.entries.size() == 1 && result.requests.size() == 1, "matching actor should preserve one entry and request");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].status == iggy::NpcMapPlayControlFramePlanEntryStatus::RequestPrepared, "entry should be prepared");
		Expect(result.entries[0].requestIndex && *result.entries[0].requestIndex == 0, "entry should preserve request index");
		Expect(result.entries[0].map.status == iggy::AiMapQuery2DStatus::Matched, "entry should preserve map query match");
		Expect(result.entries[0].map.tags == std::vector<iggy::ResourceId> { Id("zone:cover") }, "entry should preserve map query tags");
		Expect(result.entries[0].request.npcId == Id("npc:guard"), "request should preserve npc id");
		Expect(result.entries[0].request.hand.ents.size() == 1, "request hand should contain drawn ent");
		Expect(result.entries[0].request.hand.ents[0].actionTag == Id("action:seek"), "request hand should preserve action tag");
		Expect(result.entries[0].request.proposalContext.hasTargetPosition, "request context should derive target position");
		Expect(NearVec(result.entries[0].request.proposalContext.targetPosition, { 5.0F, 0.0F }), "request context should preserve control target position");
	}
}

void TestTraitDrawsAssembleInTraitOrderAndFilterBehavior()
{
	const iggy::NpcActorState2DRegistry actors { { Actor("npc:order") } };
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:order") } };
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects { Subject("npc:order") };
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools(
		{
			StrengthEnt("strength:idle-filtered", "action:idle", iggy::NpcBehaviorStateType::Idle, 100.0F),
			StrengthEnt("strength:seek", "action:strength", iggy::NpcBehaviorStateType::Seeking, 1.0F),
		},
		{ DexterityEnt("dexterity:seek", "action:dexterity", iggy::NpcBehaviorStateType::Seeking, 2.0F) },
		{ ConstitutionEnt("constitution:seek", "action:constitution", iggy::NpcBehaviorStateType::Seeking, 3.0F) },
		{ IntelligenceEnt("intelligence:seek", "action:intelligence", iggy::NpcBehaviorStateType::Seeking, 4.0F) },
		{ WisdomEnt("wisdom:seek", "action:wisdom", iggy::NpcBehaviorStateType::Seeking, 5.0F) },
		{ CharismaEnt("charisma:seek", "action:charisma", iggy::NpcBehaviorStateType::Seeking, 6.0F) });

	const iggy::NpcMapPlayControlFramePlanResult result =
		Plan(actors, controls, subjects, pools, Map());

	Expect(result.requests.size() == 1, "trait order plan should prepare one request");
	if (result.requests.size() == 1) {
		const std::vector<iggy::NpcHandEnt> &ents = result.requests[0].hand.ents;
		Expect(ents.size() == 6, "trait order hand should contain six matching ents");
		if (ents.size() == 6) {
			Expect(ents[0].source == iggy::NpcHandTraitSource::Strength && ents[0].actionTag == Id("action:strength"), "strength ent should be first");
			Expect(ents[1].source == iggy::NpcHandTraitSource::Dexterity && ents[1].actionTag == Id("action:dexterity"), "dexterity ent should be second");
			Expect(ents[2].source == iggy::NpcHandTraitSource::Constitution && ents[2].actionTag == Id("action:constitution"), "constitution ent should be third");
			Expect(ents[3].source == iggy::NpcHandTraitSource::Intelligence && ents[3].actionTag == Id("action:intelligence"), "intelligence ent should be fourth");
			Expect(ents[4].source == iggy::NpcHandTraitSource::Wisdom && ents[4].actionTag == Id("action:wisdom"), "wisdom ent should be fifth");
			Expect(ents[5].source == iggy::NpcHandTraitSource::Charisma && ents[5].actionTag == Id("action:charisma"), "charisma ent should be sixth");
		}
	}
}

void TestMissingTraitAndControlDiagnostics()
{
	const iggy::NpcActorState2DRegistry actors {
		{
			Actor("npc:missing-traits"),
			Actor("npc:no-control"),
		}
	};
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:missing-traits") } };
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects;

	const iggy::NpcMapPlayControlFramePlanResult result =
		Plan(actors, controls, subjects, Pools(), Map());

	Expect(result.requests.empty(), "missing trait/control plan should prepare no requests");
	Expect(result.missingTraitCount == 1, "missing trait should be counted");
	Expect(result.missingControlCount == 1, "missing control should be counted from frame projection");
	Expect(result.frameState.hasIssues(), "missing control should preserve frame-state issue");
	Expect(result.issues.size() == 2, "missing trait/control plan should preserve two issues");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].status == iggy::NpcMapPlayControlFramePlanEntryStatus::MissingTraitSet, "first actor should report missing trait set");
		Expect(result.entries[1].status == iggy::NpcMapPlayControlFramePlanEntryStatus::MissingControl, "second actor should report missing control");
	}
}

void TestAbsentActorsSkippedByDefault()
{
	const iggy::NpcActorState2DRegistry actors { { Actor("npc:absent", { 0.0F, 0.0F }, false) } };
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:absent") } };
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects { Subject("npc:absent") };
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools({
		StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F),
	});

	const iggy::NpcMapPlayControlFramePlanResult result =
		Plan(actors, controls, subjects, pools, Map());

	Expect(result.requests.empty(), "absent actor should not prepare request by default");
	Expect(result.actorNotPresentCount == 1, "absent actor should be counted");
	Expect(result.entries.size() == 1 && result.entries[0].status == iggy::NpcMapPlayControlFramePlanEntryStatus::ActorNotPresent, "absent actor should preserve entry status");
}

void TestOrphanControlIssueIsPreserved()
{
	const iggy::NpcActorState2DRegistry actors;
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:orphan") } };

	const iggy::NpcMapPlayControlFramePlanResult result =
		Plan(actors, controls, {}, Pools(), Map());

	Expect(result.requests.empty(), "orphan control plan should prepare no requests");
	Expect(result.orphanControlCount == 1, "orphan control should be counted");
	Expect(result.frameState.hasIssues(), "orphan control should preserve frame-state issue");
	Expect(result.issues.size() == 1 && result.issues[0].code == iggy::NpcMapPlayControlFramePlanIssueCode::OrphanControlState, "orphan control should be mapped into planner issues");
	Expect(result.issues.size() == 1 && result.issues[0].npcId == Id("npc:orphan"), "orphan control should preserve npc id");
}

void TestInvalidTraitsPreserveDrawIssuesAndStillPrepareRequest()
{
	iggy::NpcTraitSet traits;
	traits.strength = 21;
	const iggy::NpcActorState2DRegistry actors { { Actor("npc:invalid-traits") } };
	const iggy::NpcActorControlState2DRegistry controls { { Control("npc:invalid-traits") } };
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects { Subject("npc:invalid-traits", traits) };
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools({
		StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F),
	});

	const iggy::NpcMapPlayControlFramePlanResult result =
		Plan(actors, controls, subjects, pools, Map());

	Expect(result.requests.size() == 1, "invalid trait draw should still prepare request for downstream diagnostics");
	Expect(result.drawIssueCount == 1, "invalid trait draw should count hand draw issue");
	Expect(result.entries.size() == 1 && result.entries[0].traitValidation.status == iggy::NpcTraitSetStatus::ScoreOutOfRange, "invalid trait draw should preserve trait validation");
	if (result.requests.size() == 1) {
		Expect(result.requests[0].hand.issues.size() == 1, "invalid trait draw should preserve hand issue");
		Expect(result.requests[0].hand.issues[0].source == iggy::NpcHandTraitSource::Strength, "invalid trait draw should identify strength source");
	}
}

void TestNamespacedAndUnqualifiedIdsStayDistinct()
{
	const iggy::NpcActorState2DRegistry actors {
		{
			Actor("npc"),
			Actor("game:npc"),
		}
	};
	const iggy::NpcActorControlState2DRegistry controls {
		{
			Control("npc"),
			Control("game:npc"),
		}
	};
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects {
		Subject("npc"),
		Subject("game:npc"),
	};
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools({
		StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 1.0F),
	});

	const iggy::NpcMapPlayControlFramePlanResult result =
		Plan(actors, controls, subjects, pools, Map());

	Expect(result.requests.size() == 2, "distinct ids should prepare two requests");
	if (result.requests.size() == 2) {
		Expect(result.requests[0].npcId == Id("npc"), "unqualified id should stay exact");
		Expect(result.requests[1].npcId == Id("game:npc"), "namespaced id should stay exact");
	}
}

void TestPreparedRequestsFeedMapPlayControlFrameStep()
{
	const iggy::NpcActorState2DRegistry actors { { Actor("npc:guard", { 0.0F, 0.0F }) } };
	const iggy::NpcActorControlState2DRegistry controls {
		{ Control("npc:guard", iggy::seekingNpcBehaviorState({ 6.0F, 0.0F }), iggy::NpcMoveMode::None) }
	};
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects { SubjectWithTarget("npc:guard", { 6.0F, 0.0F }) };
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools(
		{
			StrengthEnt("strength:map-seek", "action:seek-cover", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }),
		},
		{
			DexterityEnt("dexterity:raw-seek", "action:seek-raw", iggy::NpcBehaviorStateType::Seeking, 5.0F, { Id("zone:loud") }),
		});
	const iggy::AiMap2D map = Map({ Node("ai:cover", { 0.0F, 0.0F }, { Id("zone:cover") }) });
	iggy::NpcMapPlayControlFramePlanConfig config;
	config.step.mapRead.matchedMapTagBonus = 4.0F;

	const iggy::NpcMapPlayControlFramePlanResult plan =
		Plan(actors, controls, subjects, pools, map, config);
	const iggy::NpcMapPlayControlFrameStep2DResult step =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			controls,
			plan.requests,
			plan.config.step);

	Expect(plan.requests.size() == 1, "acceptance plan should produce one prepared request");
	Expect(step.entries.size() == 1, "acceptance step should consume one prepared request");
	if (step.entries.size() == 1) {
		Expect(step.entries[0].mapPlay.rawSelectedActionTag == Id("action:seek-raw"), "acceptance step should preserve raw-weight winner");
		Expect(step.entries[0].mapPlay.mapSelectedActionTag == Id("action:seek-cover"), "acceptance step should preserve map-aware winner");
		Expect(step.entries[0].mapPlay.mapChangedSelection, "acceptance step should report map changed selection");
		Expect(step.entries[0].proposal.hasProposal(), "acceptance step should produce control proposal");
		Expect(step.entries[0].frameProposal.proposal.actionTag == Id("action:seek-cover"), "acceptance proposal should use map-selected action");
	}
	Expect(step.mapChangedSelectionCount == 1, "acceptance step should count changed map selection");
	Expect(step.proposedCount == 1, "acceptance step should count proposal");
}

void TestInputsAreNotMutated()
{
	iggy::NpcActorState2DRegistry actors { { Actor("npc:guard", { 1.0F, 0.0F }) } };
	iggy::NpcActorControlState2DRegistry controls { { Control("npc:guard") } };
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects { Subject("npc:guard") };
	iggy::NpcMapPlayControlFramePlanPools pools = Pools({
		StrengthEnt("strength:seek", "action:seek", iggy::NpcBehaviorStateType::Seeking, 2.0F),
	});
	iggy::AiMap2D map = Map({ Node("ai:cover", { 1.0F, 0.0F }, { Id("zone:cover") }) });

	const iggy::NpcActorState2DRegistry beforeActors = actors;
	const iggy::NpcActorControlState2DRegistry beforeControls = controls;
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> beforeSubjects = subjects;
	const iggy::NpcMapPlayControlFramePlanPools beforePools = pools;
	const iggy::AiMap2D beforeMap = map;

	(void)Plan(actors, controls, subjects, pools, map);

	Expect(actors.actors.size() == beforeActors.actors.size() && actors.actors[0].npcId == beforeActors.actors[0].npcId, "planner should not mutate actor registry");
	Expect(controls.entries.size() == beforeControls.entries.size() && controls.entries[0].npcId == beforeControls.entries[0].npcId, "planner should not mutate control registry");
	Expect(subjects.size() == beforeSubjects.size() && subjects[0].npcId == beforeSubjects[0].npcId, "planner should not mutate subjects");
	Expect(pools.strength.entries.size() == beforePools.strength.entries.size(), "planner should not mutate pools");
	Expect(map.nodes.size() == beforeMap.nodes.size() && map.nodes[0].id == beforeMap.nodes[0].id, "planner should not mutate map");
}

} // namespace

int main()
{
	TestEmptyInputsProduceNoRequests();
	TestOneActorPreparesRequestAndQueriesMap();
	TestTraitDrawsAssembleInTraitOrderAndFilterBehavior();
	TestMissingTraitAndControlDiagnostics();
	TestAbsentActorsSkippedByDefault();
	TestOrphanControlIssueIsPreserved();
	TestInvalidTraitsPreserveDrawIssuesAndStillPrepareRequest();
	TestNamespacedAndUnqualifiedIdsStayDistinct();
	TestPreparedRequestsFeedMapPlayControlFrameStep();
	TestInputsAreNotMutated();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
