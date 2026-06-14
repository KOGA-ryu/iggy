#include <cstdlib>
#include <vector>

#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/AiMapQuery2D.hpp"
#include "scene/ai/NpcConstitutionDraw.hpp"
#include "scene/ai/NpcConstitutionPool.hpp"
#include "scene/ai/NpcDexterityDraw.hpp"
#include "scene/ai/NpcDexterityPool.hpp"
#include "scene/ai/NpcHand.hpp"
#include "scene/ai/NpcMapPlayControlFrameStep2D.hpp"
#include "scene/ai/NpcRead.hpp"
#include "scene/ai/NpcStrengthDraw.hpp"
#include "scene/ai/NpcStrengthPool.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::AiMapNode2D Node(
	const char *id,
	iggy::Vec2 position,
	float radius,
	std::vector<iggy::ResourceId> tags)
{
	return {
		Id(id),
		position,
		radius,
		1.0F,
		2.0F,
		3.0F,
		4.0F,
		tags,
		{},
		true,
	};
}

iggy::NpcStrengthEnt StrengthEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behaviorState,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcDexterityEnt DexterityEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behaviorState,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcConstitutionEnt ConstitutionEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behaviorState,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return {
		Id(entryId),
		0,
		behaviorState,
		Id(actionTag),
		weight,
		mapTags,
	};
}

iggy::NpcActorControlState2D Control(
	const char *npcId,
	iggy::NpcObjective objective = iggy::waitNpcObjective(),
	iggy::NpcBehaviorState behavior = iggy::idleNpcBehaviorState(),
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still)
{
	return {
		Id(npcId),
		objective,
		behavior,
		moveMode,
	};
}

iggy::NpcPlayControlProposalContext TargetPosition(iggy::Vec2 position)
{
	iggy::NpcPlayControlProposalContext context;
	context.hasTargetPosition = true;
	context.targetPosition = position;
	return context;
}

iggy::NpcPlayControlProposalContext TargetId(const char *targetId)
{
	iggy::NpcPlayControlProposalContext context;
	context.targetId = Id(targetId);
	return context;
}

struct BuiltMap {
	iggy::AiMap2DBuildResult build;
	iggy::AiMapQuery2DResult namespacedCover;
	iggy::AiMapQuery2DResult unqualifiedCover;
	iggy::AiMapQuery2DResult quiet;
};

BuiltMap BuildTacticalMap()
{
	BuiltMap map;
	map.build = iggy::AiMap2DBuilder {}.build({
		Node("ai:zone-cover", { 0.0F, 0.0F }, 3.0F, { Id("zone:cover"), Id("zone:quiet") }),
		Node("ai:plain-cover", { 10.0F, 0.0F }, 3.0F, { Id("cover") }),
		Node("ai:quiet", { 20.0F, 0.0F }, 3.0F, { Id("zone:quiet") }),
	});
	Expect(map.build.built, "acceptance map should build");
	map.namespacedCover = iggy::AiMapQuery2D {}.query(map.build.map, { 1.0F, 0.0F });
	map.unqualifiedCover = iggy::AiMapQuery2D {}.query(map.build.map, { 10.0F, 0.0F });
	map.quiet = iggy::AiMapQuery2D {}.query(map.build.map, { 20.0F, 0.0F });
	return map;
}

iggy::NpcHand AssembleHand(
	std::vector<iggy::NpcStrengthEnt> strengthEnts,
	iggy::NpcBehaviorStateType strengthState,
	std::vector<iggy::NpcDexterityEnt> dexterityEnts,
	iggy::NpcBehaviorStateType dexterityState,
	std::vector<iggy::NpcConstitutionEnt> constitutionEnts = {},
	iggy::NpcBehaviorStateType constitutionState = iggy::NpcBehaviorStateType::None)
{
	const iggy::NpcStrengthPoolBuildResult strengthBuild =
		iggy::NpcStrengthPoolBuilder {}.build(strengthEnts);
	const iggy::NpcDexterityPoolBuildResult dexterityBuild =
		iggy::NpcDexterityPoolBuilder {}.build(dexterityEnts);
	const iggy::NpcConstitutionPoolBuildResult constitutionBuild =
		iggy::NpcConstitutionPoolBuilder {}.build(constitutionEnts);
	Expect(strengthBuild.built, "acceptance strength pool should build");
	Expect(dexterityBuild.built, "acceptance dexterity pool should build");
	Expect(constitutionBuild.built, "acceptance constitution pool should build");

	iggy::NpcTraitSet traits;
	traits.strength = 10;
	traits.dexterity = 10;
	traits.constitution = 10;

	const iggy::NpcStrengthDrawResult strengthDraw =
		iggy::NpcStrengthDraw {}.draw(strengthBuild.pool, traits, strengthState);
	const iggy::NpcDexterityDrawResult dexterityDraw =
		iggy::NpcDexterityDraw {}.draw(dexterityBuild.pool, traits, dexterityState);
	const iggy::NpcConstitutionDrawResult constitutionDraw =
		iggy::NpcConstitutionDraw {}.draw(constitutionBuild.pool, traits, constitutionState);

	return iggy::NpcHandAssembler {}.assemble(
		strengthDraw,
		dexterityDraw,
		constitutionDraw,
		{},
		{},
		{});
}

iggy::NpcHand MapChoiceHand(
	const char *matchingEntryId = "strength:map-seek",
	const char *matchingActionTag = "action:seek-cover",
	iggy::NpcBehaviorStateType matchingState = iggy::NpcBehaviorStateType::Seeking,
	std::vector<iggy::ResourceId> matchingTags = { Id("zone:cover") },
	const char *rawEntryId = "dexterity:raw-idle",
	const char *rawActionTag = "action:raw-idle",
	iggy::NpcBehaviorStateType rawState = iggy::NpcBehaviorStateType::Idle,
	std::vector<iggy::ResourceId> rawTags = { Id("zone:loud") })
{
	return AssembleHand(
		{ StrengthEnt(matchingEntryId, matchingActionTag, matchingState, 2.0F, matchingTags) },
		matchingState,
		{ DexterityEnt(rawEntryId, rawActionTag, rawState, 5.0F, rawTags) },
		rawState);
}

iggy::NpcHand WaitingHand(const char *entryId, const char *actionTag, std::vector<iggy::ResourceId> mapTags = {})
{
	return AssembleHand(
		{},
		iggy::NpcBehaviorStateType::Seeking,
		{ DexterityEnt(entryId, actionTag, iggy::NpcBehaviorStateType::Waiting, 1.0F, mapTags) },
		iggy::NpcBehaviorStateType::Waiting);
}

iggy::NpcHand AttackingHand()
{
	return AssembleHand(
		{},
		iggy::NpcBehaviorStateType::Seeking,
		{},
		iggy::NpcBehaviorStateType::Waiting,
		{ ConstitutionEnt("constitution:attack", "action:attack", iggy::NpcBehaviorStateType::Attacking, 3.0F, { Id("zone:quiet") }) },
		iggy::NpcBehaviorStateType::Attacking);
}

iggy::NpcHand EmptyRealHand()
{
	return AssembleHand(
		{},
		iggy::NpcBehaviorStateType::Seeking,
		{},
		iggy::NpcBehaviorStateType::Waiting);
}

iggy::NpcMapPlayControlFrameStep2DConfig MapBoostConfig()
{
	iggy::NpcMapPlayControlFrameStep2DConfig config;
	config.mapRead.matchedMapTagBonus = 4.0F;
	config.mapRead.unmatchedMapTagPenalty = 0.0F;
	return config;
}

iggy::NpcMapPlayControlFrameStep2DConfig FoldAboveMapWinnerConfig()
{
	iggy::NpcMapPlayControlFrameStep2DConfig config = MapBoostConfig();
	config.fold.minimumPlayScore = 10.0F;
	return config;
}

iggy::NpcMapPlayControlFrameStep2DRequest Request(
	const char *npcId,
	const iggy::NpcHand &hand,
	const iggy::AiMapQuery2DResult &map,
	const iggy::NpcPlayControlProposalContext &context = {})
{
	return {
		Id(npcId),
		hand,
		map,
		context,
	};
}

bool SameAiMapNode(const iggy::AiMapNode2D &actual, const iggy::AiMapNode2D &expected)
{
	return actual.id == expected.id
		&& NearVec(actual.position, expected.position)
		&& actual.radius == expected.radius
		&& actual.patrolWeight == expected.patrolWeight
		&& actual.coverWeight == expected.coverWeight
		&& actual.dangerWeight == expected.dangerWeight
		&& actual.interestWeight == expected.interestWeight
		&& actual.tags == expected.tags
		&& actual.links == expected.links
		&& actual.enabled == expected.enabled;
}

bool SameAiMapNodes(
	const std::vector<iggy::AiMapNode2D> &actual,
	const std::vector<iggy::AiMapNode2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameAiMapNode(actual[index], expected[index]))
			return false;
	}
	return true;
}

bool SameHandEnt(const iggy::NpcHandEnt &actual, const iggy::NpcHandEnt &expected)
{
	return actual.source == expected.source
		&& actual.entryId == expected.entryId
		&& actual.actionTag == expected.actionTag
		&& actual.behaviorState == expected.behaviorState
		&& actual.weight == expected.weight
		&& actual.mapTags == expected.mapTags
		&& actual.drawEntryIndex == expected.drawEntryIndex;
}

bool SameHand(const iggy::NpcHand &actual, const iggy::NpcHand &expected)
{
	if (actual.ents.size() != expected.ents.size() || actual.issues.size() != expected.issues.size())
		return false;
	for (std::size_t index = 0; index < actual.ents.size(); ++index) {
		if (!SameHandEnt(actual.ents[index], expected.ents[index]))
			return false;
	}
	return true;
}

bool SameQuery(const iggy::AiMapQuery2DResult &actual, const iggy::AiMapQuery2DResult &expected)
{
	return actual.status == expected.status
		&& NearVec(actual.position, expected.position)
		&& actual.entries.size() == expected.entries.size()
		&& actual.tags == expected.tags
		&& actual.patrolWeight == expected.patrolWeight
		&& actual.coverWeight == expected.coverWeight
		&& actual.dangerWeight == expected.dangerWeight
		&& actual.interestWeight == expected.interestWeight;
}

bool SameControl(const iggy::NpcActorControlState2D &actual, const iggy::NpcActorControlState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.objective.type == expected.objective.type
		&& actual.objective.targetId == expected.objective.targetId
		&& NearVec(actual.objective.targetPosition, expected.objective.targetPosition)
		&& actual.behavior.type == expected.behavior.type
		&& actual.behavior.targetId == expected.behavior.targetId
		&& NearVec(actual.behavior.targetPosition, expected.behavior.targetPosition)
		&& actual.moveMode == expected.moveMode;
}

bool SameControls(
	const std::vector<iggy::NpcActorControlState2D> &actual,
	const std::vector<iggy::NpcActorControlState2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameControl(actual[index], expected[index]))
			return false;
	}
	return true;
}

void TestRealMapAndTraitsDriveMapSelectedControl()
{
	const BuiltMap map = BuildTacticalMap();
	const iggy::NpcHand hand = MapChoiceHand();
	const iggy::NpcActorControlState2DRegistry controls {
		{ Control("npc:guard") },
	};

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			controls,
			{ Request("npc:guard", hand, map.namespacedCover, TargetPosition({ 8.0F, 2.0F })) },
			MapBoostConfig());

	Expect(result.status == iggy::NpcMapPlayControlFrameStep2DStatus::Ran, "real map trait pipeline should run");
	Expect(result.mapChangedSelectionCount == 1, "real map trait pipeline should count changed map selection");
	Expect(result.entries.size() == 1, "real map trait pipeline should preserve request entry");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].mapPlay.rawSelectedActionTag == Id("action:raw-idle"), "raw read should prefer higher base weight");
		Expect(result.entries[0].mapPlay.mapSelectedActionTag == Id("action:seek-cover"), "map read should prefer matching map tag");
		Expect(result.entries[0].mapPlay.mapRead.rankedEnts.size() == 2, "map report should preserve ranked map entries");
		if (result.entries[0].mapPlay.mapRead.rankedEnts.size() == 2) {
			Expect(result.entries[0].mapPlay.mapRead.rankedEnts[0].baseWeight == 2.0F, "map report should preserve base weight");
			Expect(result.entries[0].mapPlay.mapRead.rankedEnts[0].score == 6.0F, "map report should preserve boosted score");
			Expect(result.entries[0].mapPlay.mapRead.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId> { Id("zone:cover") }, "map report should preserve matched tag");
		}
		Expect(result.entries[0].proposal.hasProposal(), "map-selected play should produce proposal");
		Expect(result.entries[0].proposal.actionTag == Id("action:seek-cover"), "proposal should preserve map-selected action tag");
	}
	Expect(result.appliedCount == 1 && result.failedApplyCount == 0, "real map trait pipeline should apply one control");
	Expect(result.report.appliedCount == 1 && result.report.updatedCount == 1, "frame report should count updated control");
	Expect(result.registry.entries.size() == 1, "real map trait pipeline should return one control");
	if (result.registry.entries.size() == 1) {
		Expect(result.registry.entries[0].npcId == Id("npc:guard"), "final control should preserve npc id");
		Expect(result.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Seeking, "final control should use map-selected behavior");
		Expect(result.registry.entries[0].behavior.targetPosition.x == 8.0F && result.registry.entries[0].behavior.targetPosition.y == 2.0F, "final control should preserve target position");
		Expect(result.registry.entries[0].moveMode == iggy::NpcMoveMode::Walk, "final control should use seeking move mode");
	}
}

void TestMultiNpcFrameUpdatesAppendsFailuresAndDuplicates()
{
	const BuiltMap map = BuildTacticalMap();
	const iggy::NpcHand updateHand = MapChoiceHand();
	const iggy::NpcHand appendHand = WaitingHand("dexterity:append-wait", "action:append-wait");
	const iggy::NpcHand foldedHand = EmptyRealHand();
	const iggy::NpcHand missingContextHand = AttackingHand();
	const iggy::NpcHand duplicateFirstHand = WaitingHand("dexterity:dupe-wait", "action:dupe-wait");
	const iggy::NpcHand duplicateSecondHand = MapChoiceHand(
		"strength:dupe-seek",
		"action:dupe-seek",
		iggy::NpcBehaviorStateType::Seeking,
		{ Id("zone:cover") },
		"dexterity:dupe-idle",
		"action:dupe-idle");

	const iggy::NpcActorControlState2DRegistry controls {
		{
			Control("npc:guard"),
			Control("npc:folded"),
			Control("npc:needs-target"),
			Control("npc:dupe"),
		},
	};

	std::vector<iggy::NpcMapPlayControlFrameStep2DRequest> requests {
		Request("npc:guard", updateHand, map.namespacedCover, TargetPosition({ 8.0F, 2.0F })),
		Request("npc:new", appendHand, map.quiet),
		Request("npc:folded", foldedHand, map.quiet),
		Request("npc:needs-target", missingContextHand, map.quiet),
		Request("npc:dupe", duplicateFirstHand, map.quiet),
		Request("npc:dupe", duplicateSecondHand, map.namespacedCover, TargetPosition({ 3.0F, 4.0F })),
	};

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(controls, requests, MapBoostConfig());

	Expect(result.status == iggy::NpcMapPlayControlFrameStep2DStatus::Ran, "multi npc acceptance should run with changes");
	Expect(result.requestCount == 6 && result.proposalCount == 6, "multi npc acceptance should count all requests");
	Expect(result.proposedCount == 4 && result.proposalFailedCount == 2, "multi npc acceptance should count proposal successes and failures");
	Expect(result.appliedCount == 4 && result.failedApplyCount == 2, "multi npc acceptance should count apply successes and failures");
	Expect(result.mapChangedSelectionCount == 2, "multi npc acceptance should count changed map selections");
	Expect(result.report.appliedCount == 4 && result.report.failedCount == 2, "frame report should preserve applied and failed counts");
	Expect(result.report.appendedCount == 1 && result.report.updatedCount == 3, "frame report should count appended and updated controls");
	Expect(result.report.noProposalCount == 2, "frame report should count no-proposal apply failures from folded/context-failed requests");
	Expect(result.report.duplicateNpcProposalCount == 1, "frame report should count duplicate npc proposal");
	Expect(result.report.events.size() == 12, "frame report should preserve deterministic summary events");
	if (result.report.events.size() == 12) {
		Expect(result.report.events[0] == iggy::NpcPlayControlFrameEvent2D::ProposalApplied, "frame events should start with first proposal result");
		Expect(result.report.events[2] == iggy::NpcPlayControlFrameEvent2D::ProposalFailed, "frame events should preserve folded failure order");
		Expect(result.report.events[11] == iggy::NpcPlayControlFrameEvent2D::ControlChanged, "frame events should end with changed fact");
	}

	Expect(result.entries.size() == requests.size(), "multi npc acceptance should preserve per-request entries");
	if (result.entries.size() == requests.size()) {
		Expect(result.entries[0].requestIndex == 0 && result.entries[0].request.npcId == Id("npc:guard"), "entries should preserve first request index and id");
		Expect(result.entries[0].mapPlay.mapChangedSelection, "update request should preserve map-changed report");
		Expect(result.entries[0].proposal.status == iggy::NpcPlayControlProposalStatus::Proposed, "update request should preserve proposed status");
		Expect(result.entries[0].frameProposal.proposal.actionTag == Id("action:seek-cover"), "update frame proposal should preserve map-selected action");
		Expect(result.entries[2].mapPlay.folded(), "folded request should preserve folded map report");
		Expect(result.entries[2].proposal.status == iggy::NpcPlayControlProposalStatus::NoKeptPlay, "folded request should preserve no-kept proposal status");
		Expect(result.entries[3].proposal.status == iggy::NpcPlayControlProposalStatus::MissingTargetId, "missing context request should preserve target-id failure");
		Expect(result.entries[5].mapPlay.mapChangedSelection, "duplicate later request should preserve map-changed report");
	}

	Expect(result.registry.entries.size() == 5, "multi npc acceptance should update existing controls and append one new control");
	if (result.registry.entries.size() == 5) {
		Expect(result.registry.entries[0].npcId == Id("npc:guard"), "updated existing control should keep original order");
		Expect(result.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Seeking, "updated control should use map-selected behavior");
		Expect(result.registry.entries[1].npcId == Id("npc:folded"), "folded control should remain in place");
		Expect(result.registry.entries[1].behavior.type == iggy::NpcBehaviorStateType::Idle, "folded control should not mutate behavior");
		Expect(result.registry.entries[2].npcId == Id("npc:needs-target"), "missing context control should remain in place");
		Expect(result.registry.entries[2].behavior.type == iggy::NpcBehaviorStateType::Idle, "missing context control should not mutate behavior");
		Expect(result.registry.entries[3].npcId == Id("npc:dupe"), "duplicate control should remain in original slot");
		Expect(result.registry.entries[3].behavior.type == iggy::NpcBehaviorStateType::Seeking, "later duplicate success should win");
		Expect(NearVec(result.registry.entries[3].behavior.targetPosition, { 3.0F, 4.0F }), "later duplicate success should preserve target position");
		Expect(result.registry.entries[4].npcId == Id("npc:new"), "missing npc control should append at the end");
		Expect(result.registry.entries[4].behavior.type == iggy::NpcBehaviorStateType::Waiting, "appended control should preserve waiting behavior");
	}
	Expect(SameControls(result.registry.entries, result.report.registry.entries), "result registry should match nested frame report registry");
}

void TestNamespacedAndUnqualifiedIdsRemainDistinct()
{
	const BuiltMap map = BuildTacticalMap();
	const iggy::NpcHand namespacedHand = MapChoiceHand(
		"strength:namespaced",
		"action:zone-cover",
		iggy::NpcBehaviorStateType::Seeking,
		{ Id("zone:cover") },
		"dexterity:namespaced-raw",
		"action:zone-raw");
	const iggy::NpcHand unqualifiedHand = MapChoiceHand(
		"strength:plain",
		"action:plain-cover",
		iggy::NpcBehaviorStateType::Seeking,
		{ Id("cover") },
		"dexterity:plain-raw",
		"action:plain-raw",
		iggy::NpcBehaviorStateType::Idle,
		{ Id("zone:cover") });
	const iggy::NpcActorControlState2DRegistry controls {
		{
			Control("npc:guard"),
			Control("guard"),
		},
	};

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(
			controls,
			{
				Request("npc:guard", namespacedHand, map.namespacedCover, TargetPosition({ 1.0F, 1.0F })),
				Request("guard", unqualifiedHand, map.unqualifiedCover, TargetPosition({ 2.0F, 2.0F })),
			},
			MapBoostConfig());

	Expect(result.appliedCount == 2 && result.failedApplyCount == 0, "exact ids acceptance should apply both exact npc ids");
	Expect(result.report.duplicateNpcProposalCount == 0, "namespaced and unqualified npc ids should not count as duplicates");
	Expect(result.entries.size() == 2, "exact ids acceptance should preserve two entries");
	if (result.entries.size() == 2) {
		Expect(result.entries[0].mapPlay.mapRead.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId> { Id("zone:cover") }, "namespaced map tag should match exact namespaced tag");
		Expect(result.entries[0].mapPlay.mapRead.rankedEnts[0].ent.actionTag == Id("action:zone-cover"), "namespaced action tag should be preserved exactly");
		Expect(result.entries[1].mapPlay.mapRead.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId> { Id("cover") }, "unqualified map tag should match exact unqualified tag");
		Expect(result.entries[1].mapPlay.mapRead.rankedEnts[0].ent.actionTag == Id("action:plain-cover"), "unqualified action tag should be preserved exactly");
	}
	Expect(result.registry.entries.size() == 2, "exact ids acceptance should preserve two controls");
	if (result.registry.entries.size() == 2) {
		Expect(result.registry.entries[0].npcId == Id("npc:guard"), "namespaced npc id should remain distinct");
		Expect(result.registry.entries[1].npcId == Id("guard"), "unqualified npc id should remain distinct");
		Expect(result.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Seeking, "namespaced control should update");
		Expect(result.registry.entries[1].behavior.type == iggy::NpcBehaviorStateType::Seeking, "unqualified control should update");
	}
}

void TestInputsAreNotMutated()
{
	BuiltMap map = BuildTacticalMap();
	const std::vector<iggy::AiMapNode2D> mapNodesBefore = map.build.map.nodes;
	const iggy::AiMapQuery2DResult queryBefore = map.namespacedCover;
	iggy::NpcHand hand = MapChoiceHand();
	const iggy::NpcHand handBefore = hand;
	iggy::NpcActorControlState2DRegistry controls {
		{ Control("npc:guard") },
	};
	const iggy::NpcActorControlState2DRegistry controlsBefore = controls;
	std::vector<iggy::NpcMapPlayControlFrameStep2DRequest> requests {
		Request("npc:guard", hand, map.namespacedCover, TargetPosition({ 8.0F, 2.0F })),
	};
	const std::vector<iggy::NpcMapPlayControlFrameStep2DRequest> requestsBefore = requests;

	const iggy::NpcMapPlayControlFrameStep2DResult result =
		iggy::NpcMapPlayControlFrameStepper2D {}.step(controls, requests, MapBoostConfig());

	Expect(result.changed(), "immutability setup should produce a control change");
	Expect(SameAiMapNodes(map.build.map.nodes, mapNodesBefore), "frame pipeline should not mutate ai map");
	Expect(SameQuery(map.namespacedCover, queryBefore), "frame pipeline should not mutate query result");
	Expect(SameHand(hand, handBefore), "frame pipeline should not mutate hand");
	Expect(SameControls(controls.entries, controlsBefore.entries), "frame pipeline should not mutate controls");
	Expect(requests.size() == requestsBefore.size(), "frame pipeline should not mutate request count");
	if (requests.size() == requestsBefore.size() && !requests.empty()) {
		Expect(requests[0].npcId == requestsBefore[0].npcId, "frame pipeline should not mutate request npc id");
		Expect(SameHand(requests[0].hand, requestsBefore[0].hand), "frame pipeline should not mutate request hand");
		Expect(SameQuery(requests[0].map, requestsBefore[0].map), "frame pipeline should not mutate request map");
	}
}

} // namespace

int main()
{
	TestRealMapAndTraitsDriveMapSelectedControl();
	TestMultiNpcFrameUpdatesAppendsFailuresAndDuplicates();
	TestNamespacedAndUnqualifiedIdsRemainDistinct();
	TestInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
