#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeGameplaySnapshotValidator.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = Id("level:gameplay-snapshot-validator");
	return level;
}

iggy::NpcActorState2D Actor(
	const char *npcId,
	iggy::Vec2 position,
	bool present = true)
{
	return {
		Id(npcId),
		Id("ai-profile:guard"),
		Id("faction:town"),
		position,
		Id("goal:patrol"),
		present,
	};
}

iggy::NpcActorControlState2D Control(const char *npcId)
{
	iggy::NpcActorControlState2D control;
	control.npcId = Id(npcId);
	control.objective = iggy::waitNpcObjective();
	control.behavior = iggy::idleNpcBehaviorState();
	control.moveMode = iggy::NpcMoveMode::Still;
	return control;
}

iggy::runtime::RuntimeGameplaySnapshot Snapshot(
	iggy::LevelRuntimeState level,
	std::vector<iggy::NpcActorState2D> actors = {},
	std::vector<iggy::NpcActorControlState2D> controls = {})
{
	iggy::runtime::RuntimeGameplaySnapshot snapshot;
	snapshot.session.level = level;
	snapshot.session.tickIndex = 7;
	snapshot.npcActors.actors = actors;
	snapshot.npcControls.entries = controls;
	return snapshot;
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplaySnapshotValidationResult &result,
	iggy::runtime::RuntimeGameplaySnapshotIssueCode code,
	std::size_t index = 0)
{
	for (const iggy::runtime::RuntimeGameplaySnapshotIssue &issue : result.issues) {
		if (issue.code == code && issue.index == index) {
			return true;
		}
	}
	return false;
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

bool SameSnapshot(
	const iggy::runtime::RuntimeGameplaySnapshot &actual,
	const iggy::runtime::RuntimeGameplaySnapshot &expected)
{
	if (actual.session.level.map.id != expected.session.level.map.id
		|| actual.session.level.map.width != expected.session.level.map.width
		|| actual.session.level.map.height != expected.session.level.map.height
		|| actual.session.level.map.tiles.size() != expected.session.level.map.tiles.size()
		|| actual.session.tickIndex != expected.session.tickIndex
		|| actual.npcActors.actors.size() != expected.npcActors.actors.size()
		|| actual.npcControls.entries.size() != expected.npcControls.entries.size()) {
		return false;
	}
	for (std::size_t index = 0; index < actual.npcActors.actors.size(); ++index) {
		if (!SameActor(actual.npcActors.actors[index], expected.npcActors.actors[index])) {
			return false;
		}
	}
	for (std::size_t index = 0; index < actual.npcControls.entries.size(); ++index) {
		if (actual.npcControls.entries[index].npcId != expected.npcControls.entries[index].npcId) {
			return false;
		}
	}
	return true;
}

void TestValidGameplaySnapshotWithMatchingActorControlValidates()
{
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = Snapshot(
		Level({ "..", ".." }),
		{ Actor("npc:guard", { 0.5F, 0.5F }) },
		{ Control("npc:guard") });

	const iggy::runtime::RuntimeGameplaySnapshotValidationResult result =
		iggy::runtime::RuntimeGameplaySnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "valid gameplay snapshot should validate");
	Expect(result.session.valid, "valid gameplay snapshot should preserve valid nested session result");
	Expect(result.issues.empty(), "valid gameplay snapshot should not report issues");
	Expect(result.npcFrameState.entries.size() == 1, "valid gameplay snapshot should preserve frame-state projection");
	Expect(result.npcFrameState.issues.empty(), "valid gameplay snapshot should have no frame-state issues");
}

void TestInvalidSessionSnapshotProducesSessionInvalidIssue()
{
	iggy::runtime::RuntimeGameplaySnapshot snapshot = Snapshot(Level({ ".." }));
	snapshot.session.hasPlayer = true;
	snapshot.session.player = PlayerAgent(Id("player:bad"), { 8.0F, 0.5F }, { 0, 0 });

	const iggy::runtime::RuntimeGameplaySnapshotValidationResult result =
		iggy::runtime::RuntimeGameplaySnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "invalid session should invalidate gameplay snapshot");
	Expect(!result.session.valid, "invalid session should preserve nested session invalidity");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::SessionInvalid, 0), "invalid session should emit gameplay session issue");
	Expect(result.issues[0].code == iggy::runtime::RuntimeGameplaySnapshotIssueCode::SessionInvalid, "session issue should come first");
	Expect(result.issues[0].sessionIssue.code == iggy::runtime::RuntimeSessionSnapshotIssueCode::PlayerOutOfBounds, "session issue should preserve nested session issue");
}

void TestPresentNpcActorOutOfBoundsReportsActorIndex()
{
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = Snapshot(
		Level({ ".." }),
		{
			Actor("npc:inside", { 0.5F, 0.5F }),
			Actor("npc:outside", { 3.0F, 0.5F }),
		},
		{
			Control("npc:inside"),
			Control("npc:outside"),
		});

	const iggy::runtime::RuntimeGameplaySnapshotValidationResult result =
		iggy::runtime::RuntimeGameplaySnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "out-of-bounds present NPC should invalidate gameplay snapshot");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorOutOfBounds, 1), "out-of-bounds NPC issue should preserve actor index");
}

void TestAbsentOutOfBoundsNpcActorIsIgnored()
{
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = Snapshot(
		Level({ ".." }),
		{ Actor("npc:absent", { 3.0F, 0.5F }, false) },
		{ Control("npc:absent") });

	const iggy::runtime::RuntimeGameplaySnapshotValidationResult result =
		iggy::runtime::RuntimeGameplaySnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "absent out-of-bounds NPC should be ignored by gameplay snapshot validator");
	Expect(!HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorOutOfBounds), "absent out-of-bounds NPC should not report bounds issue");
}

void TestMissingControlAndOrphanControlProduceDeterministicIssues()
{
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = Snapshot(
		Level({ "..", ".." }),
		{
			Actor("npc:missing-control", { 0.5F, 0.5F }),
			Actor("npc:matched", { 1.5F, 0.5F }),
		},
		{
			Control("npc:matched"),
			Control("npc:orphan"),
		});

	const iggy::runtime::RuntimeGameplaySnapshotValidationResult result =
		iggy::runtime::RuntimeGameplaySnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "missing/orphan controls should invalidate gameplay snapshot");
	Expect(result.npcFrameState.issues.size() == 2, "frame-state projection should preserve missing and orphan issues");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorMissingControl, 0), "missing control should report actor index");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcControlMissingActor, 1), "orphan control should report control index");
	Expect(result.issues.size() == 2, "missing/orphan setup should produce two gameplay issues");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorMissingControl, "missing control issue should come before orphan issue");
		Expect(result.issues[1].code == iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcControlMissingActor, "orphan issue should follow missing control issue");
	}
}

void TestMultipleIssuesPreserveStableOrder()
{
	iggy::runtime::RuntimeGameplaySnapshot snapshot = Snapshot(
		Level({ ".." }),
		{
			Actor("npc:outside", { 4.0F, 0.5F }),
			Actor("npc:missing-control", { 0.5F, 0.5F }),
		},
		{
			Control("npc:orphan"),
		});
	snapshot.session.hasPlayer = true;
	snapshot.session.player = PlayerAgent(Id("player:bad"), { -1.0F, 0.5F }, { 0, 0 });

	const iggy::runtime::RuntimeGameplaySnapshotValidationResult result =
		iggy::runtime::RuntimeGameplaySnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "multiple issue snapshot should be invalid");
	Expect(result.issues.size() == 5, "multiple issue snapshot should report all expected issues");
	if (result.issues.size() == 5) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeGameplaySnapshotIssueCode::SessionInvalid, "session issue should be first");
		Expect(result.issues[1].code == iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorOutOfBounds && result.issues[1].index == 0, "actor bounds issue should follow in actor order");
		Expect(result.issues[2].code == iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorMissingControl && result.issues[2].index == 0, "first frame missing issue should follow bounds");
		Expect(result.issues[3].code == iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorMissingControl && result.issues[3].index == 1, "second frame missing issue should preserve projector order");
		Expect(result.issues[4].code == iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcControlMissingActor && result.issues[4].index == 0, "orphan control issue should follow missing controls");
	}
}

void TestInvalidMapDimensionsSuppressActorBoundsButStillProjectControls()
{
	const iggy::runtime::RuntimeGameplaySnapshot snapshot = Snapshot(
		{},
		{ Actor("npc:actor", { 99.0F, 99.0F }) },
		{ Control("npc:orphan") });

	const iggy::runtime::RuntimeGameplaySnapshotValidationResult result =
		iggy::runtime::RuntimeGameplaySnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "invalid map dimensions should invalidate gameplay snapshot");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::SessionInvalid), "invalid map should emit session issue");
	Expect(!HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorOutOfBounds), "invalid map dimensions should suppress actor bounds checks");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorMissingControl, 0), "invalid map should still preserve frame-state missing control diagnostics");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcControlMissingActor, 0), "invalid map should still preserve frame-state orphan diagnostics");
}

void TestValidatorDoesNotMutateSnapshot()
{
	iggy::runtime::RuntimeGameplaySnapshot snapshot = Snapshot(
		Level({ "..", ".." }),
		{ Actor("npc:guard", { 0.5F, 0.5F }) },
		{ Control("npc:guard") });
	const iggy::runtime::RuntimeGameplaySnapshot before = snapshot;

	const iggy::runtime::RuntimeGameplaySnapshotValidationResult result =
		iggy::runtime::RuntimeGameplaySnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "snapshot immutability setup should validate");
	Expect(SameSnapshot(snapshot, before), "gameplay snapshot validator should not mutate snapshot");
}

} // namespace

int main()
{
	TestValidGameplaySnapshotWithMatchingActorControlValidates();
	TestInvalidSessionSnapshotProducesSessionInvalidIssue();
	TestPresentNpcActorOutOfBoundsReportsActorIndex();
	TestAbsentOutOfBoundsNpcActorIsIgnored();
	TestMissingControlAndOrphanControlProduceDeterministicIssues();
	TestMultipleIssuesPreserveStableOrder();
	TestInvalidMapDimensionsSuppressActorBoundsButStillProjectControls();
	TestValidatorDoesNotMutateSnapshot();

	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
