#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSessionSnapshotValidator.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::PlayerAgent;

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = iggy::ResourceId("level:snapshot-validator");
	return level;
}

iggy::npc_ai::NpcAgentEntry Npc(iggy::Vec2 position, iggy::TileCoord homeTile, iggy::ResourceId id = iggy::ResourceId("npc:one"))
{
	iggy::npc_ai::NpcAgentEntry npc;
	npc.id = id;
	npc.state.position = position;
	npc.state.homeTile = homeTile;
	return npc;
}

iggy::runtime::RuntimeSessionSnapshot Snapshot(iggy::LevelRuntimeState level)
{
	iggy::runtime::RuntimeSessionSnapshot snapshot;
	snapshot.level = level;
	snapshot.tickIndex = 11;
	return snapshot;
}

iggy::runtime::RuntimeSessionSnapshot SnapshotWithPlayer(iggy::LevelRuntimeState level, iggy::Vec2 position)
{
	iggy::runtime::RuntimeSessionSnapshot snapshot = Snapshot(level);
	snapshot.hasPlayer = true;
	snapshot.player = PlayerAgent(iggy::ResourceId("player:one"), position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return snapshot;
}

bool HasIssue(
	const iggy::runtime::RuntimeSessionSnapshotValidationResult &result,
	iggy::runtime::RuntimeSessionSnapshotIssueCode code,
	std::size_t index = 0)
{
	for (const iggy::runtime::RuntimeSessionSnapshotIssue &issue : result.issues) {
		if (issue.code == code && issue.index == index)
			return true;
	}
	return false;
}

void ExpectSnapshotSame(
	const iggy::runtime::RuntimeSessionSnapshot &actual,
	const iggy::runtime::RuntimeSessionSnapshot &expected,
	const char *message)
{
	Expect(actual.level.map.id == expected.level.map.id, message);
	Expect(actual.level.map.width == expected.level.map.width, message);
	Expect(actual.level.map.height == expected.level.map.height, message);
	Expect(actual.level.map.tiles.size() == expected.level.map.tiles.size(), message);
	Expect(actual.level.npcAgents.size() == expected.level.npcAgents.size(), message);
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	ExpectPlayerAgent(actual.player, expected.player, message);
}

void TestDefaultSnapshotIsInvalidByMapDimensions()
{
	const iggy::runtime::RuntimeSessionSnapshot snapshot;

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "default snapshot should be invalid because map dimensions are not positive");
	Expect(result.issues.size() == 1, "default snapshot should report one issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotIssueCode::InvalidMapDimensions), "default snapshot should report invalid map dimensions");
}

void TestCapturedValidSessionSnapshotValidates()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = Level({
		"..",
		"..",
	});
	session.level.npcAgents.push_back(Npc({ 1.25F, 1.25F }, { 1, 1 }));
	session.tickIndex = 9;
	session.hasPlayer = true;
	session.player = PlayerAgent(iggy::ResourceId("player:one"), { 0.25F, 0.25F }, { 0, 0 });
	const iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(session);

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "captured valid session snapshot should validate");
	Expect(result.issues.empty(), "captured valid session snapshot should have no issues");
}

void TestPlayerInsideMapValidates()
{
	const iggy::runtime::RuntimeSessionSnapshot snapshot = SnapshotWithPlayer(Level({ ".." }), { 1.5F, 0.5F });

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "player inside positive map should validate");
	Expect(result.issues.empty(), "player inside positive map should have no issues");
}

void TestAbsentPlayerIgnoresStoredOutOfBoundsPlayerData()
{
	iggy::runtime::RuntimeSessionSnapshot snapshot = Snapshot(Level({ ".." }));
	snapshot.hasPlayer = false;
	snapshot.player = PlayerAgent(iggy::ResourceId("player:ignored"), { 100.0F, 100.0F }, { 100, 100 });

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "absent player should ignore stored player fields");
	Expect(result.issues.empty(), "absent player should not report player out of bounds");
}

void TestPresentPlayerOutOfBoundsReportsIssue()
{
	const iggy::runtime::RuntimeSessionSnapshot snapshot = SnapshotWithPlayer(Level({ ".." }), { 2.0F, 0.5F });

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "present out-of-bounds player should invalidate snapshot");
	Expect(result.issues.size() == 1, "present out-of-bounds player should report one issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotIssueCode::PlayerOutOfBounds), "present out-of-bounds player should report player issue");
}

void TestSparseTileStorageUsesMapBoundsOnly()
{
	iggy::LevelRuntimeState level = Level({ ".." });
	level.map.tiles.resize(1);
	const iggy::runtime::RuntimeSessionSnapshot snapshot = SnapshotWithPlayer(level, { 1.5F, 0.5F });

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "sparse tile storage should remain valid when dimensions and entities are in bounds");
	Expect(result.issues.empty(), "sparse tile storage should not report tile count mismatch");
}

void TestNpcInsideMapValidates()
{
	iggy::LevelRuntimeState level = Level({
		"..",
		"..",
	});
	level.npcAgents.push_back(Npc({ 1.25F, 1.25F }, { 1, 1 }));
	const iggy::runtime::RuntimeSessionSnapshot snapshot = Snapshot(level);

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "in-bounds NPC should validate");
	Expect(result.issues.empty(), "in-bounds NPC should not report issues");
}

void TestNpcPositionOutOfBoundsReportsIndex()
{
	iggy::LevelRuntimeState level = Level({ ".." });
	level.npcAgents.push_back(Npc({ 0.5F, 0.5F }, { 0, 0 }, iggy::ResourceId("npc:zero")));
	level.npcAgents.push_back(Npc({ 3.0F, 0.5F }, { 1, 0 }, iggy::ResourceId("npc:one")));
	const iggy::runtime::RuntimeSessionSnapshot snapshot = Snapshot(level);

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "out-of-bounds NPC position should invalidate snapshot");
	Expect(result.issues.size() == 1, "one out-of-bounds NPC should report one issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotIssueCode::NpcOutOfBounds, 1), "NPC issue should report original NPC index");
}

void TestNpcHomeTileOutOfBoundsReportsIndex()
{
	iggy::LevelRuntimeState level = Level({ ".." });
	level.npcAgents.push_back(Npc({ 1.5F, 0.5F }, { 2, 0 }));
	const iggy::runtime::RuntimeSessionSnapshot snapshot = Snapshot(level);

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "out-of-bounds NPC home tile should invalidate snapshot");
	Expect(result.issues.size() == 1, "one out-of-bounds NPC home tile should report one issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotIssueCode::NpcOutOfBounds, 0), "NPC home tile issue should report NPC index");
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	iggy::LevelRuntimeState level = Level({ ".." });
	level.npcAgents.push_back(Npc({ 3.0F, 0.5F }, { 1, 0 }));
	level.npcAgents.push_back(Npc({ 0.5F, 0.5F }, { 3, 0 }));
	const iggy::runtime::RuntimeSessionSnapshot snapshot = SnapshotWithPlayer(level, { -1.0F, 0.5F });

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "multiple out-of-bounds agents should invalidate snapshot");
	Expect(result.issues.size() == 3, "multiple out-of-bounds agents should report all issues");
	if (result.issues.size() == 3) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeSessionSnapshotIssueCode::PlayerOutOfBounds, "player issue should come before NPC issues");
		Expect(result.issues[1].code == iggy::runtime::RuntimeSessionSnapshotIssueCode::NpcOutOfBounds && result.issues[1].index == 0, "first NPC issue should preserve NPC order");
		Expect(result.issues[2].code == iggy::runtime::RuntimeSessionSnapshotIssueCode::NpcOutOfBounds && result.issues[2].index == 1, "second NPC issue should preserve NPC order");
	}
}

void TestInvalidMapDimensionsSuppressEntityBounds()
{
	iggy::runtime::RuntimeSessionSnapshot snapshot = SnapshotWithPlayer({}, { 100.0F, 100.0F });
	snapshot.level.npcAgents.push_back(Npc({ 100.0F, 100.0F }, { 100, 100 }));

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(!result.valid, "invalid map dimensions should invalidate snapshot");
	Expect(result.issues.size() == 1, "invalid map dimensions should suppress cascading entity bounds");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotIssueCode::InvalidMapDimensions), "invalid map dimensions should report map issue");
}

void TestValidatorDoesNotMutateSnapshot()
{
	iggy::runtime::RuntimeSessionSnapshot snapshot = SnapshotWithPlayer(Level({ ".." }), { 1.5F, 0.5F });
	snapshot.level.npcAgents.push_back(Npc({ 0.5F, 0.5F }, { 0, 0 }));
	const iggy::runtime::RuntimeSessionSnapshot before = snapshot;

	const iggy::runtime::RuntimeSessionSnapshotValidationResult result = iggy::runtime::RuntimeSessionSnapshotValidator {}.validate(snapshot);

	Expect(result.valid, "snapshot immutability setup should validate");
	ExpectSnapshotSame(snapshot, before, "snapshot validator should not mutate snapshot");
}

} // namespace

int main()
{
	TestDefaultSnapshotIsInvalidByMapDimensions();
	TestCapturedValidSessionSnapshotValidates();
	TestPlayerInsideMapValidates();
	TestAbsentPlayerIgnoresStoredOutOfBoundsPlayerData();
	TestPresentPlayerOutOfBoundsReportsIssue();
	TestSparseTileStorageUsesMapBoundsOnly();
	TestNpcInsideMapValidates();
	TestNpcPositionOutOfBoundsReportsIndex();
	TestNpcHomeTileOutOfBoundsReportsIndex();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestInvalidMapDimensionsSuppressEntityBounds();
	TestValidatorDoesNotMutateSnapshot();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
