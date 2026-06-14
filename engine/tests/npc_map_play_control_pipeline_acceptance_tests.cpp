#include <cstdlib>
#include <vector>

#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/AiMapQuery2D.hpp"
#include "scene/ai/NpcDexterityDraw.hpp"
#include "scene/ai/NpcDexterityPool.hpp"
#include "scene/ai/NpcFold.hpp"
#include "scene/ai/NpcHand.hpp"
#include "scene/ai/NpcMapRead.hpp"
#include "scene/ai/NpcMapReadProjection.hpp"
#include "scene/ai/NpcPlayControlProposal.hpp"
#include "scene/ai/NpcRead.hpp"
#include "scene/ai/NpcStrengthDraw.hpp"
#include "scene/ai/NpcStrengthPool.hpp"
#include "scene/ai/NpcTell.hpp"
#include "scene/npc/NpcPlayControlFrameApply2D.hpp"
#include "scene/npc/NpcPlayControlFrameReport2D.hpp"
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
	std::vector<iggy::ResourceId> mapTags)
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

iggy::NpcPlayControlFrameReport2D ApplyAndReport(
	const iggy::NpcActorControlState2DRegistry &registry,
	const std::vector<iggy::NpcPlayControlFrameProposal2D> &requests)
{
	const iggy::NpcPlayControlFrameApply2DResult apply =
		iggy::NpcPlayControlFrameApplier2D {}.apply(registry, requests);
	return iggy::NpcPlayControlFrameReporter2D {}.report(apply);
}

bool SameNode(const iggy::AiMapNode2D &actual, const iggy::AiMapNode2D &expected)
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

bool SameNodes(
	const std::vector<iggy::AiMapNode2D> &actual,
	const std::vector<iggy::AiMapNode2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameNode(actual[index], expected[index]))
			return false;
	}
	return true;
}

bool SameEnt(const iggy::NpcHandEnt &actual, const iggy::NpcHandEnt &expected)
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
	if (actual.ents.size() != expected.ents.size())
		return false;
	if (actual.issues.size() != expected.issues.size())
		return false;
	for (std::size_t index = 0; index < actual.ents.size(); ++index) {
		if (!SameEnt(actual.ents[index], expected.ents[index]))
			return false;
	}
	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		if (actual.issues[index].code != expected.issues[index].code
			|| actual.issues[index].source != expected.issues[index].source)
			return false;
	}
	return true;
}

bool SameProposalCore(const iggy::NpcPlayControlProposal &actual, const iggy::NpcPlayControlProposal &expected)
{
	return actual.status == expected.status
		&& actual.actionTag == expected.actionTag
		&& actual.requestedBehaviorState == expected.requestedBehaviorState
		&& actual.objective.type == expected.objective.type
		&& actual.objective.targetId == expected.objective.targetId
		&& NearVec(actual.objective.targetPosition, expected.objective.targetPosition)
		&& actual.behavior.type == expected.behavior.type
		&& actual.behavior.targetId == expected.behavior.targetId
		&& NearVec(actual.behavior.targetPosition, expected.behavior.targetPosition)
		&& actual.moveMode == expected.moveMode;
}

struct MapSelectionFixture {
	iggy::AiMap2DBuildResult mapBuild;
	iggy::AiMapQuery2DResult mapQuery;
	iggy::NpcStrengthPoolBuildResult strengthBuild;
	iggy::NpcDexterityPoolBuildResult dexterityBuild;
	iggy::NpcStrengthDrawResult strengthDraw;
	iggy::NpcDexterityDrawResult dexterityDraw;
	iggy::NpcHand hand;
	iggy::NpcRead rawRead;
	iggy::NpcMapRead mapRead;
	iggy::NpcRead adaptedMapRead;
	iggy::NpcPlay rawPlay;
	iggy::NpcPlay mapPlay;
	iggy::NpcTell tell;
	iggy::NpcFold fold;
	iggy::NpcPlayControlProposal proposal;
};

MapSelectionFixture BuildMapSelectionFixture()
{
	MapSelectionFixture fixture;
	fixture.mapBuild = iggy::AiMap2DBuilder {}.build({
		Node("ai:cover-zone", { 0.0F, 0.0F }, 3.0F, { Id("zone:cover"), Id("zone:quiet") }),
	});
	Expect(fixture.mapBuild.built, "map selection fixture should build ai map");
	fixture.mapQuery = iggy::AiMapQuery2D {}.query(fixture.mapBuild.map, { 1.0F, 0.0F });

	fixture.strengthBuild = iggy::NpcStrengthPoolBuilder {}.build({
		StrengthEnt("strength:map-seek", "action:seek-cover", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }),
	});
	fixture.dexterityBuild = iggy::NpcDexterityPoolBuilder {}.build({
		DexterityEnt("dexterity:raw-idle", "action:idle-raw", iggy::NpcBehaviorStateType::Idle, 5.0F, { Id("zone:loud") }),
	});
	Expect(fixture.strengthBuild.built, "map selection fixture should build strength pool");
	Expect(fixture.dexterityBuild.built, "map selection fixture should build dexterity pool");

	iggy::NpcTraitSet traits;
	traits.strength = 10;
	traits.dexterity = 10;
	fixture.strengthDraw = iggy::NpcStrengthDraw {}.draw(fixture.strengthBuild.pool, traits, iggy::NpcBehaviorStateType::Seeking);
	fixture.dexterityDraw = iggy::NpcDexterityDraw {}.draw(fixture.dexterityBuild.pool, traits, iggy::NpcBehaviorStateType::Idle);
	fixture.hand = iggy::NpcHandAssembler {}.assemble(fixture.strengthDraw, fixture.dexterityDraw, {}, {}, {}, {});

	fixture.rawRead = iggy::NpcReader {}.read(fixture.hand);
	fixture.rawPlay = iggy::NpcPlaySelector {}.play(fixture.rawRead);
	fixture.mapRead = iggy::NpcMapReader {}.read(fixture.hand, fixture.mapQuery, { 0.0F, 4.0F, 0.0F, false });
	fixture.adaptedMapRead = iggy::NpcMapReadProjection {}.toRead(fixture.mapRead);
	fixture.mapPlay = iggy::NpcPlaySelector {}.play(fixture.adaptedMapRead);
	fixture.tell = iggy::NpcTeller {}.tell(fixture.mapPlay);
	fixture.fold = iggy::NpcFolder {}.fold(fixture.tell);
	fixture.proposal = iggy::NpcPlayControlProjector {}.project(
		fixture.fold,
		{ true, { 8.0F, 2.0F }, {} });
	return fixture;
}

void TestMapAwareReadChangesSelectionAndAppliesControl()
{
	const MapSelectionFixture fixture = BuildMapSelectionFixture();

	Expect(fixture.mapQuery.tags == std::vector<iggy::ResourceId>({ Id("zone:cover"), Id("zone:quiet") }), "map selection should use real ai map query tags");
	Expect(fixture.hand.ents.size() == 2, "map selection fixture should assemble two hand ents");
	Expect(fixture.rawPlay.hasPlay(), "raw read setup should produce a play");
	Expect(fixture.rawPlay.selected.ent.entryId == Id("dexterity:raw-idle"), "raw read should select higher base weight non-matching ent");
	Expect(fixture.mapPlay.hasPlay(), "map read setup should produce a play");
	Expect(fixture.mapPlay.selected.ent.entryId == Id("strength:map-seek"), "map read should select lower base weight matching ent");
	Expect(fixture.mapPlay.selected.ent.actionTag == Id("action:seek-cover"), "map-selected play should preserve action tag");
	Expect(fixture.fold.kept(), "map-selected play should be kept");
	Expect(fixture.proposal.hasProposal(), "map-selected play should project control proposal");
	Expect(fixture.proposal.actionTag == Id("action:seek-cover"), "map-selected proposal should preserve selected action");
	Expect(fixture.proposal.behavior.type == iggy::NpcBehaviorStateType::Seeking, "map-selected proposal should preserve selected behavior");
	Expect(NearVec(fixture.proposal.behavior.targetPosition, { 8.0F, 2.0F }), "map-selected proposal should preserve target position");

	Expect(fixture.mapRead.rankedEnts.size() == 2, "map read should rank two ents");
	if (fixture.mapRead.rankedEnts.size() == 2) {
		Expect(fixture.mapRead.rankedEnts[0].ent.entryId == Id("strength:map-seek"), "map explanation should rank matching ent first");
		Expect(fixture.mapRead.rankedEnts[0].baseWeight == 2.0F, "map explanation should preserve base weight");
		Expect(fixture.mapRead.rankedEnts[0].score == 6.0F, "map explanation should preserve final score");
		Expect(fixture.mapRead.rankedEnts[0].handIndex == 0, "map explanation should preserve original hand index");
		Expect(fixture.mapRead.rankedEnts[0].matchedMapTags == std::vector<iggy::ResourceId> { Id("zone:cover") }, "map explanation should preserve matched tags");
		Expect(fixture.mapRead.rankedEnts[1].unmatchedMapTags == std::vector<iggy::ResourceId> { Id("zone:loud") }, "map explanation should preserve unmatched tags");
	}

	const iggy::NpcActorControlState2D existing = Control("npc:guard");
	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(
		{ { existing } },
		{ { Id("npc:guard"), fixture.proposal } });

	Expect(report.appliedCount == 1 && report.updatedCount == 1 && report.changed, "map-selected proposal should update existing control");
	Expect(report.registry.entries.size() == 1, "map-selected update should preserve one control");
	if (report.registry.entries.size() == 1) {
		Expect(report.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Seeking, "final control should use map-selected behavior");
		Expect(NearVec(report.registry.entries[0].behavior.targetPosition, { 8.0F, 2.0F }), "final control should preserve map-selected target");
		Expect(report.registry.entries[0].moveMode == iggy::NpcMoveMode::Walk, "final control should use seeking move mode");
	}
	Expect(report.apply.entries.size() == 1, "map-selected report should preserve apply entry");
	if (report.apply.entries.size() == 1)
		Expect(report.apply.entries[0].request.proposal.actionTag == Id("action:seek-cover"), "apply report should preserve map-selected action");
}

void TestRequireMapTagMatchKeepsNeutralFallbackThroughControlPipeline()
{
	iggy::AiMap2DBuildResult mapBuild = iggy::AiMap2DBuilder {}.build({
		Node("ai:quiet-zone", { 0.0F, 0.0F }, 3.0F, { Id("zone:quiet") }),
	});
	Expect(mapBuild.built, "fallback fixture should build ai map");
	const iggy::AiMapQuery2DResult mapQuery = iggy::AiMapQuery2D {}.query(mapBuild.map, { 0.0F, 0.0F });

	const iggy::NpcStrengthPoolBuildResult strengthBuild = iggy::NpcStrengthPoolBuilder {}.build({
		StrengthEnt("strength:miss", "action:miss", iggy::NpcBehaviorStateType::Seeking, 8.0F, { Id("zone:loud") }),
	});
	const iggy::NpcDexterityPoolBuildResult dexterityBuild = iggy::NpcDexterityPoolBuilder {}.build({
		DexterityEnt("dexterity:neutral", "action:neutral-wait", iggy::NpcBehaviorStateType::Waiting, 1.0F),
	});
	Expect(strengthBuild.built, "fallback fixture should build strength pool");
	Expect(dexterityBuild.built, "fallback fixture should build dexterity pool");

	iggy::NpcTraitSet traits;
	traits.strength = 10;
	traits.dexterity = 10;
	const iggy::NpcStrengthDrawResult strengthDraw =
		iggy::NpcStrengthDraw {}.draw(strengthBuild.pool, traits, iggy::NpcBehaviorStateType::Seeking);
	const iggy::NpcDexterityDrawResult dexterityDraw =
		iggy::NpcDexterityDraw {}.draw(dexterityBuild.pool, traits, iggy::NpcBehaviorStateType::Waiting);
	const iggy::NpcHand hand =
		iggy::NpcHandAssembler {}.assemble(strengthDraw, dexterityDraw, {}, {}, {}, {});
	const iggy::NpcMapRead mapRead =
		iggy::NpcMapReader {}.read(hand, mapQuery, { 0.0F, 1.0F, 0.0F, true });
	const iggy::NpcRead adaptedRead = iggy::NpcMapReadProjection {}.toRead(mapRead);
	const iggy::NpcPlay play = iggy::NpcPlaySelector {}.play(adaptedRead);
	const iggy::NpcTell tell = iggy::NpcTeller {}.tell(play);
	const iggy::NpcFold fold = iggy::NpcFolder {}.fold(tell);
	const iggy::NpcPlayControlProposal proposal = iggy::NpcPlayControlProjector {}.project(fold);
	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(
		{},
		{ { Id("npc:fallback"), proposal } });

	Expect(mapRead.rankedEnts.size() == 1, "required map match should filter tagged miss and keep neutral fallback");
	if (mapRead.rankedEnts.size() == 1) {
		Expect(mapRead.rankedEnts[0].ent.entryId == Id("dexterity:neutral"), "neutral fallback should be selected after filtering tagged miss");
		Expect(!mapRead.rankedEnts[0].mapMatched, "neutral fallback should remain map-neutral");
		Expect(mapRead.rankedEnts[0].matchedMapTags.empty() && mapRead.rankedEnts[0].unmatchedMapTags.empty(), "neutral fallback should have no map tag explanations");
	}
	Expect(play.hasPlay() && play.selected.ent.actionTag == Id("action:neutral-wait"), "neutral fallback should play through adapted read");
	Expect(fold.kept(), "neutral fallback should fold as kept");
	Expect(proposal.hasProposal(), "neutral fallback should project proposal");
	Expect(report.appliedCount == 1 && report.appendedCount == 1, "neutral fallback should append control");
	Expect(report.registry.entries.size() == 1, "neutral fallback should create one control");
	if (report.registry.entries.size() == 1) {
		Expect(report.registry.entries[0].npcId == Id("npc:fallback"), "neutral fallback should preserve npc id");
		Expect(report.registry.entries[0].behavior.type == iggy::NpcBehaviorStateType::Waiting, "neutral fallback should preserve selected waiting behavior");
		Expect(report.registry.entries[0].moveMode == iggy::NpcMoveMode::Still, "neutral fallback should use waiting move mode");
	}
}

void TestMapAwarePipelineInputsAreNotMutated()
{
	MapSelectionFixture fixture = BuildMapSelectionFixture();
	const std::vector<iggy::AiMapNode2D> mapNodesBefore = fixture.mapBuild.map.nodes;
	const iggy::AiMapQuery2DResult mapQueryBefore = fixture.mapQuery;
	const iggy::NpcHand handBefore = fixture.hand;
	const iggy::NpcRead adaptedReadBefore = fixture.adaptedMapRead;
	const iggy::NpcPlay playBefore = fixture.mapPlay;
	const iggy::NpcFold foldBefore = fixture.fold;
	const iggy::NpcPlayControlProposal proposalBefore = fixture.proposal;
	iggy::NpcActorControlState2DRegistry registry { { Control("npc:guard") } };
	const iggy::NpcActorControlState2DRegistry registryBefore = registry;
	std::vector<iggy::NpcPlayControlFrameProposal2D> requests {
		{ Id("npc:guard"), fixture.proposal },
	};
	const std::vector<iggy::NpcPlayControlFrameProposal2D> requestsBefore = requests;

	const iggy::NpcPlayControlFrameReport2D report = ApplyAndReport(registry, requests);

	Expect(report.changed, "immutability setup should apply a map-selected control");
	Expect(SameNodes(fixture.mapBuild.map.nodes, mapNodesBefore), "map-aware pipeline should not mutate ai map");
	Expect(fixture.mapQuery.tags == mapQueryBefore.tags && fixture.mapQuery.status == mapQueryBefore.status, "map-aware pipeline should not mutate query result");
	Expect(SameHand(fixture.hand, handBefore), "map-aware pipeline should not mutate hand");
	Expect(fixture.adaptedMapRead.rankedEnts.size() == adaptedReadBefore.rankedEnts.size(), "map-aware pipeline should not mutate adapted read");
	Expect(fixture.mapPlay.status == playBefore.status && fixture.mapPlay.selected.ent.entryId == playBefore.selected.ent.entryId, "map-aware pipeline should not mutate play");
	Expect(fixture.fold.status == foldBefore.status && fixture.fold.reason == foldBefore.reason, "map-aware pipeline should not mutate fold");
	Expect(SameProposalCore(fixture.proposal, proposalBefore), "map-aware pipeline should not mutate proposal");
	Expect(registry.entries.size() == registryBefore.entries.size() && registry.entries[0].npcId == registryBefore.entries[0].npcId, "map-aware pipeline should not mutate input registry");
	Expect(requests.size() == requestsBefore.size() && SameProposalCore(requests[0].proposal, requestsBefore[0].proposal), "map-aware pipeline should not mutate proposal requests");
}

} // namespace

int main()
{
	TestMapAwareReadChangesSelectionAndAppliesControl();
	TestRequireMapTagMatchKeepsNeutralFallbackThroughControlPipeline();
	TestMapAwarePipelineInputsAreNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
