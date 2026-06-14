#include <cstdlib>
#include <vector>

#include "scene/npc/NpcActorMovementFrameIntent2D.hpp"
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
	const char *npcId = "npc:guard",
	iggy::Vec2 position = { 1.0F, 2.0F },
	bool present = true)
{
	return {
		Id(npcId),
		Id("profile:guard"),
		Id("faction:town"),
		position,
		{},
		present,
	};
}

iggy::NpcActorControlState2D Control(
	iggy::NpcBehaviorState behavior = iggy::idleNpcBehaviorState(),
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still,
	const char *npcId = "npc:guard")
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		behavior,
		moveMode,
	};
}

iggy::NpcActorFrameState2D Frame(
	const iggy::NpcActorState2D &actor,
	const iggy::NpcActorControlState2D &control,
	bool hasControl = true)
{
	return {
		actor,
		control,
		hasControl,
	};
}

iggy::NpcActorFrameState2DProjectionResult Projection(std::vector<iggy::NpcActorFrameState2D> entries)
{
	iggy::NpcActorFrameState2DProjectionResult result;
	result.entries = entries;
	return result;
}

bool SameActor(const iggy::NpcActorState2D &actual, const iggy::NpcActorState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.aiProfileId == expected.aiProfileId
		&& actual.factionId == expected.factionId
		&& NearVec(actual.position, expected.position)
		&& actual.currentGoalId == expected.currentGoalId
		&& actual.present == expected.present;
}

bool SameBehavior(const iggy::NpcBehaviorState &actual, const iggy::NpcBehaviorState &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& NearVec(actual.targetPosition, expected.targetPosition);
}

bool SameControl(const iggy::NpcActorControlState2D &actual, const iggy::NpcActorControlState2D &expected)
{
	return actual.npcId == expected.npcId
		&& actual.objective.type == expected.objective.type
		&& actual.objective.targetId == expected.objective.targetId
		&& NearVec(actual.objective.targetPosition, expected.objective.targetPosition)
		&& SameBehavior(actual.behavior, expected.behavior)
		&& actual.moveMode == expected.moveMode;
}

bool SameFrame(const iggy::NpcActorFrameState2D &actual, const iggy::NpcActorFrameState2D &expected)
{
	return SameActor(actual.actor, expected.actor)
		&& SameControl(actual.control, expected.control)
		&& actual.hasControl == expected.hasControl;
}

bool SameIssue(const iggy::NpcActorFrameState2DIssue &actual, const iggy::NpcActorFrameState2DIssue &expected)
{
	return actual.code == expected.code
		&& actual.actorIndex == expected.actorIndex
		&& actual.controlIndex == expected.controlIndex
		&& SameActor(actual.actor, expected.actor)
		&& SameControl(actual.control, expected.control);
}

bool SameProjection(
	const iggy::NpcActorFrameState2DProjectionResult &actual,
	const iggy::NpcActorFrameState2DProjectionResult &expected)
{
	if (actual.entries.size() != expected.entries.size()
		|| actual.issues.size() != expected.issues.size()) {
		return false;
	}

	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (!SameFrame(actual.entries[index], expected.entries[index])) {
			return false;
		}
	}

	for (std::size_t index = 0; index < actual.issues.size(); ++index) {
		if (!SameIssue(actual.issues[index], expected.issues[index])) {
			return false;
		}
	}

	return true;
}

void ExpectZeroCounts(const iggy::NpcActorMovementFrameIntent2DResult &result)
{
	Expect(result.entryCount == 0, "empty frame intent should have zero entry count");
	Expect(result.readyCount == 0, "empty frame intent should have zero ready count");
	Expect(result.noMovementCount == 0, "empty frame intent should have zero no movement count");
	Expect(result.missingControlCount == 0, "empty frame intent should have zero missing control count");
	Expect(result.actorNotPresentCount == 0, "empty frame intent should have zero actor-not-present count");
	Expect(result.unsupportedBehaviorCount == 0, "empty frame intent should have zero unsupported behavior count");
	Expect(result.invalidMoveModeCount == 0, "empty frame intent should have zero invalid move mode count");
}

void TestEmptyFrameStateReturnsNoFrameEntries()
{
	const iggy::NpcActorFrameState2DProjectionResult frameState;

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState);

	Expect(result.status == iggy::NpcActorMovementFrameIntent2DStatus::NoFrameEntries, "empty frame-state projection should return NoFrameEntries");
	Expect(result.entries.empty(), "empty frame-state projection should produce no movement entries");
	Expect(!result.hasReadyMovement(), "empty frame-state projection should have no ready movement");
	ExpectZeroCounts(result);
	Expect(SameProjection(result.frameState, frameState), "empty frame-state projection should be copied");
}

void TestEmptyEntriesWithProjectionIssuesPreservesIssues()
{
	iggy::NpcActorFrameState2DProjectionResult frameState;
	frameState.issues.push_back({
		iggy::NpcActorFrameState2DIssueCode::OrphanControlState,
		0,
		2,
		{},
		Control(iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still, "npc:orphan"),
	});

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState);

	Expect(result.status == iggy::NpcActorMovementFrameIntent2DStatus::NoFrameEntries, "issue-only projection should still have no frame entries");
	Expect(result.entries.empty(), "issue-only projection should not create movement entries");
	Expect(result.issues.size() == 1, "issue-only projection should copy issues");
	Expect(SameIssue(result.issues[0], frameState.issues[0]), "issue-only projection should preserve issue payload");
	Expect(SameProjection(result.frameState, frameState), "issue-only projection should preserve copied frame-state result");
	ExpectZeroCounts(result);
}

void TestSingleSeekingMovingFrameReturnsReadyMoveTo()
{
	const iggy::NpcActorFrameState2D frame = Frame(
		Actor("npc:seeker", { 2.0F, 3.0F }),
		Control(iggy::seekingNpcBehaviorState({ 8.0F, 9.0F }), iggy::NpcMoveMode::Walk, "npc:seeker"));
	const iggy::NpcActorFrameState2DProjectionResult frameState = Projection({ frame });

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState);

	Expect(result.status == iggy::NpcActorMovementFrameIntent2DStatus::Projected, "ready movement should project successfully");
	Expect(result.hasReadyMovement(), "ready movement should be reported");
	Expect(result.entries.size() == 1, "ready movement should produce one entry");
	Expect(result.entryCount == 1, "ready movement should count one entry");
	Expect(result.readyCount == 1, "ready movement should count one ready intent");
	Expect(result.entries[0].frameIndex == 0, "ready movement should preserve frame index");
	Expect(SameFrame(result.entries[0].frame, frame), "ready movement should preserve copied frame");
	Expect(result.entries[0].intent.status == iggy::NpcActorMovementIntent2DStatus::Ready, "ready movement should preserve delegated intent status");
	Expect(result.entries[0].intent.type == iggy::NpcActorMovementIntent2DType::MoveTo, "ready movement should be MoveTo");
	Expect(result.entries[0].intent.npcId == Id("npc:seeker"), "ready movement should preserve namespaced npc id");
	Expect(NearVec(result.entries[0].intent.targetPosition, { 8.0F, 9.0F }), "ready movement should preserve target position");
}

void TestMultipleEntriesPreserveOrderAndFrameIndexes()
{
	const std::vector<iggy::NpcActorFrameState2D> frames {
		Frame(Actor("npc:first"), Control(iggy::seekingNpcBehaviorState({ 1.0F, 1.0F }), iggy::NpcMoveMode::Walk, "npc:first")),
		Frame(Actor("npc:second"), Control(iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still, "npc:second")),
		Frame(Actor("third"), Control(iggy::fleeingNpcBehaviorState({ 9.0F, 9.0F }), iggy::NpcMoveMode::Run, "third")),
	};
	const iggy::NpcActorFrameState2DProjectionResult frameState = Projection(frames);

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState);

	Expect(result.entries.size() == frames.size(), "all frame entries should be projected");
	for (std::size_t index = 0; index < frames.size(); ++index) {
		Expect(result.entries[index].frameIndex == index, "frame indexes should follow input order");
		Expect(SameFrame(result.entries[index].frame, frames[index]), "projected frame entries should preserve frame payloads");
	}
	Expect(result.entries[0].intent.npcId == Id("npc:first"), "first projected intent should preserve first npc id");
	Expect(result.entries[1].intent.npcId == Id("npc:second"), "second projected intent should preserve second npc id");
	Expect(result.entries[2].intent.npcId == Id("third"), "third projected intent should preserve unqualified npc id");
}

void TestMixedStatusesCountCorrectly()
{
	const iggy::NpcMoveMode invalidMode = static_cast<iggy::NpcMoveMode>(999);
	const std::vector<iggy::NpcActorFrameState2D> frames {
		Frame(Actor("npc:ready"), Control(iggy::seekingNpcBehaviorState({ 8.0F, 8.0F }), iggy::NpcMoveMode::Walk, "npc:ready")),
		Frame(Actor("npc:no-move"), Control(iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Run, "npc:no-move")),
		Frame(Actor("npc:missing"), Control(iggy::seekingNpcBehaviorState({ 8.0F, 8.0F }), iggy::NpcMoveMode::Walk, "npc:missing"), false),
		Frame(Actor("npc:absent", { 1.0F, 1.0F }, false), Control(iggy::seekingNpcBehaviorState({ 8.0F, 8.0F }), iggy::NpcMoveMode::Walk, "npc:absent")),
		Frame(Actor("npc:unsupported"), Control(iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run, "npc:unsupported")),
		Frame(Actor("npc:invalid"), Control(iggy::seekingNpcBehaviorState({ 8.0F, 8.0F }), invalidMode, "npc:invalid")),
	};
	const iggy::NpcActorFrameState2DProjectionResult frameState = Projection(frames);

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState);

	Expect(result.status == iggy::NpcActorMovementFrameIntent2DStatus::Projected, "mixed entries with one ready intent should return Projected");
	Expect(result.entryCount == frames.size(), "mixed statuses should count all entries");
	Expect(result.readyCount == 1, "mixed statuses should count ready intents");
	Expect(result.noMovementCount == 1, "mixed statuses should count no movement intents");
	Expect(result.missingControlCount == 1, "mixed statuses should count missing control intents");
	Expect(result.actorNotPresentCount == 1, "mixed statuses should count actor-not-present intents");
	Expect(result.unsupportedBehaviorCount == 1, "mixed statuses should count unsupported behavior intents");
	Expect(result.invalidMoveModeCount == 1, "mixed statuses should count invalid move mode intents");
	Expect(result.entries[2].intent.status == iggy::NpcActorMovementIntent2DStatus::MissingControl, "missing-control entries should delegate to the one-frame projector");
}

void TestNoReadyMovementStatusWhenEntriesAllDeclineMovement()
{
	const iggy::NpcActorFrameState2DProjectionResult frameState = Projection({
		Frame(Actor("npc:idle"), Control(iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still, "npc:idle")),
		Frame(Actor("npc:attack"), Control(iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run, "npc:attack")),
	});

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState);

	Expect(result.status == iggy::NpcActorMovementFrameIntent2DStatus::NoReadyMovement, "entries without ready movement should return NoReadyMovement");
	Expect(!result.hasReadyMovement(), "entries without ready movement should not report ready movement");
	Expect(result.noMovementCount == 1, "entries without ready movement should count no movement");
	Expect(result.unsupportedBehaviorCount == 1, "entries without ready movement should count unsupported behavior");
}

void TestFrameStateProjectionIssuesDoNotBecomeMovementEntries()
{
	iggy::NpcActorFrameState2DProjectionResult frameState = Projection({
		Frame(Actor("npc:ready"), Control(iggy::seekingNpcBehaviorState({ 2.0F, 2.0F }), iggy::NpcMoveMode::Walk, "npc:ready")),
	});
	frameState.issues.push_back({
		iggy::NpcActorFrameState2DIssueCode::MissingControlState,
		4,
		0,
		Actor("npc:missing-control"),
		{},
	});

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState);

	Expect(result.entries.size() == 1, "projection issues should not create extra movement entries");
	Expect(result.entryCount == 1, "projection issues should not change entry count");
	Expect(result.issues.size() == 1, "projection issues should be preserved separately");
	Expect(SameIssue(result.issues[0], frameState.issues[0]), "projection issue payload should be copied");
}

void TestConfigPassesThroughWithoutInventingApproachSemantics()
{
	iggy::NpcActorMovementIntent2DConfig config;
	config.allowAttackingApproach = true;
	config.allowInteractingApproach = true;
	const iggy::NpcActorFrameState2DProjectionResult frameState = Projection({
		Frame(Actor("npc:attack"), Control(iggy::attackingNpcBehaviorState(Id("target:enemy")), iggy::NpcMoveMode::Run, "npc:attack")),
	});

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState, config);

	Expect(result.status == iggy::NpcActorMovementFrameIntent2DStatus::NoReadyMovement, "approach config should not create ready frame movement yet");
	Expect(result.unsupportedBehaviorCount == 1, "approach config should still delegate unsupported behavior status");
	Expect(result.entries[0].intent.status == iggy::NpcActorMovementIntent2DStatus::UnsupportedBehavior, "approach config should not change delegated attacking status");
}

void TestInputProjectionIsNotMutated()
{
	iggy::NpcActorFrameState2DProjectionResult frameState = Projection({
		Frame(Actor("npc:stable"), Control(iggy::seekingNpcBehaviorState({ 6.0F, 7.0F }), iggy::NpcMoveMode::Walk, "npc:stable")),
		Frame(Actor("npc:idle"), Control(iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still, "npc:idle")),
	});
	frameState.issues.push_back({
		iggy::NpcActorFrameState2DIssueCode::OrphanControlState,
		0,
		1,
		{},
		Control(iggy::waitingNpcBehaviorState(), iggy::NpcMoveMode::Still, "npc:orphan"),
	});
	const iggy::NpcActorFrameState2DProjectionResult before = frameState;

	const iggy::NpcActorMovementFrameIntent2DResult result =
		iggy::NpcActorMovementFrameIntentProjector2D {}.project(frameState);

	Expect(result.entryCount == 2, "immutability setup should project two entries");
	Expect(SameProjection(frameState, before), "frame movement intent projection should not mutate input projection");
}

} // namespace

int main()
{
	TestEmptyFrameStateReturnsNoFrameEntries();
	TestEmptyEntriesWithProjectionIssuesPreservesIssues();
	TestSingleSeekingMovingFrameReturnsReadyMoveTo();
	TestMultipleEntriesPreserveOrderAndFrameIndexes();
	TestMixedStatusesCountCorrectly();
	TestNoReadyMovementStatusWhenEntriesAllDeclineMovement();
	TestFrameStateProjectionIssuesDoNotBecomeMovementEntries();
	TestConfigPassesThroughWithoutInventingApproachSemantics();
	TestInputProjectionIsNotMutated();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
