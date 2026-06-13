#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <vector>

#include "runtime/RuntimeAutosaveRotationExecutor.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"
#include "runtime/RuntimeSaveSlotPathPolicy.hpp"
#include "runtime/RuntimeSaveSlotStore.hpp"
#include "scene/level/LevelRuntimeState.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_autosave_rotation_executor_tests_tmp";
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

void RemoveTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
}

iggy::runtime::RuntimeSaveSlotPathPolicyConfig PathConfig(std::filesystem::path base = TempRoot())
{
	return { base, ".igsave" };
}

iggy::runtime::RuntimeSaveSlotId AutoSlot(const std::string &name)
{
	return { iggy::runtime::RuntimeSaveSlotKind::Auto, name };
}

std::filesystem::path AutoPath(const std::string &name)
{
	const iggy::runtime::RuntimeSaveSlotPathResult path = iggy::runtime::RuntimeSaveSlotPathPolicy {}.pathFor(PathConfig(), AutoSlot(name));
	Expect(path.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "autosave executor fixture should build valid auto slot path");
	return path.path;
}

void CreateAutoDirectory()
{
	std::filesystem::create_directories(TempRoot() / "auto");
}

void WriteAutoFile(const std::string &name, std::vector<std::uint8_t> bytes)
{
	CreateAutoDirectory();
	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(AutoPath(name), bytes);
	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "autosave executor fixture should write auto file");
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path &path)
{
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "autosave executor fixture should read file bytes");
	return read.bytes;
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows, const char *id)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = iggy::ResourceId(id);
	level.map.playerStart = { 0, 0 };
	iggy::npc_ai::NpcAgentEntry npc;
	npc.id = iggy::ResourceId("npc:autosave");
	npc.state.position = { 1.25F, 1.25F };
	npc.state.homeTile = { 1, 1 };
	npc.state.awareness.playerVisible = true;
	npc.state.awareness.alerted = true;
	npc.state.awareness.alertTicksRemaining = 2;
	npc.state.awareness.lastSeenTile = { 0, 1 };
	npc.state.awareness.lastSeenPosition = { 0.5F, 1.5F };
	npc.state.followState.waypointIndex = 1;
	level.npcAgents.push_back(npc);
	return level;
}

iggy::runtime::RuntimeSessionState Session(const char *levelId = "level:autosave")
{
	iggy::runtime::RuntimeSessionState session;
	session.level = Level({ "..", ".." }, levelId);
	session.tickIndex = 17;
	session.hasPlayer = true;
	session.player = PlayerAgent(
		iggy::ResourceId("player:autosave"),
		{ 0.5F, 0.5F },
		{ 0, 0 },
		iggy::PlayerMovementStatus::Moving,
		iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::RuntimeSessionSnapshotRestoreConfig RestoreConfigNoCaches()
{
	iggy::runtime::RuntimeSessionBuildConfig config;
	config.buildRenderCache = false;
	return { config };
}

void ExpectLevelSame(const iggy::LevelRuntimeState &actual, const iggy::LevelRuntimeState &expected, const char *message)
{
	Expect(actual.map.id == expected.map.id, message);
	Expect(actual.map.width == expected.map.width, message);
	Expect(actual.map.height == expected.map.height, message);
	Expect(actual.map.playerStart.x == expected.map.playerStart.x && actual.map.playerStart.y == expected.map.playerStart.y, message);
	Expect(actual.map.tiles.size() == expected.map.tiles.size(), message);
	for (std::size_t index = 0; index < actual.map.tiles.size() && index < expected.map.tiles.size(); ++index)
		Expect(actual.map.tiles[index].walkable == expected.map.tiles[index].walkable, message);
	Expect(actual.npcAgents.size() == expected.npcAgents.size(), message);
	for (std::size_t index = 0; index < actual.npcAgents.size() && index < expected.npcAgents.size(); ++index) {
		Expect(actual.npcAgents[index].id == expected.npcAgents[index].id, message);
		Expect(NearVec(actual.npcAgents[index].state.position, expected.npcAgents[index].state.position), message);
		Expect(actual.npcAgents[index].state.homeTile == expected.npcAgents[index].state.homeTile, message);
		Expect(actual.npcAgents[index].state.awareness.playerVisible == expected.npcAgents[index].state.awareness.playerVisible, message);
		Expect(actual.npcAgents[index].state.awareness.alerted == expected.npcAgents[index].state.awareness.alerted, message);
		Expect(actual.npcAgents[index].state.awareness.alertTicksRemaining == expected.npcAgents[index].state.awareness.alertTicksRemaining, message);
		Expect(actual.npcAgents[index].state.awareness.lastSeenTile == expected.npcAgents[index].state.awareness.lastSeenTile, message);
		Expect(NearVec(actual.npcAgents[index].state.awareness.lastSeenPosition, expected.npcAgents[index].state.awareness.lastSeenPosition), message);
		Expect(actual.npcAgents[index].state.followState.waypointIndex == expected.npcAgents[index].state.followState.waypointIndex, message);
		Expect(actual.npcAgents[index].state.followState.completed == expected.npcAgents[index].state.followState.completed, message);
	}
}

void ExpectAuthoritativeSessionSame(
	const iggy::runtime::RuntimeSessionState &actual,
	const iggy::runtime::RuntimeSessionState &expected,
	const char *message)
{
	ExpectLevelSame(actual.level, expected.level, message);
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	ExpectPlayerAgent(actual.player, expected.player, message);
}

iggy::runtime::RuntimeSessionState LoadAutoSession(const std::string &name)
{
	const iggy::runtime::RuntimeSaveSlotLoadResult load = iggy::runtime::RuntimeSaveSlotStore {}.load(PathConfig(), AutoSlot(name), RestoreConfigNoCaches());
	Expect(load.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Loaded, "autosave executor fixture should load saved autosave");
	return load.load.session;
}

void TestMaxSlotsZeroReturnsInvalidPlanAndTouchesNoFiles()
{
	ResetTempRoot();
	WriteAutoFile("autosave_0", { 1 });
	const std::filesystem::path existing = AutoPath("autosave_0");

	const iggy::runtime::RuntimeAutosaveRotationExecutionResult result = iggy::runtime::RuntimeAutosaveRotationExecutor {}.saveAutosave(
		Session(),
		PathConfig(),
		0);

	Expect(result.status == iggy::runtime::RuntimeAutosaveRotationExecutionStatus::InvalidPlan, "maxSlots zero should return InvalidPlan");
	Expect(!result.plan.plan.valid, "invalid maxSlots should preserve invalid plan");
	Expect(result.deletions.empty(), "invalid plan should not delete");
	Expect(result.renames.empty(), "invalid plan should not rename");
	Expect(ReadBytes(existing) == std::vector<std::uint8_t>({ 1 }), "invalid plan should not touch existing files");
}

void TestEmptyAutosaveDirectorySavesNewAutosave()
{
	ResetTempRoot();
	CreateAutoDirectory();
	const iggy::runtime::RuntimeSessionState session = Session("level:new-autosave");

	const iggy::runtime::RuntimeAutosaveRotationExecutionResult result = iggy::runtime::RuntimeAutosaveRotationExecutor {}.saveAutosave(
		session,
		PathConfig(),
		3);

	Expect(result.status == iggy::runtime::RuntimeAutosaveRotationExecutionStatus::Saved, "empty autosave directory should save new autosave");
	Expect(result.deletions.size() == 1, "maxSlots 3 should attempt one deletion");
	Expect(result.deletions[0].status == iggy::runtime::RuntimeSaveSlotDeletionStatus::NotFound, "missing oldest autosave should not block rotation");
	Expect(result.renames.size() == 2, "maxSlots 3 should attempt two renames");
	Expect(result.renames[0].status == iggy::runtime::RuntimeSaveSlotRenameStatus::SourceNotFound, "missing autosave_1 should not block rotation");
	Expect(result.renames[1].status == iggy::runtime::RuntimeSaveSlotRenameStatus::SourceNotFound, "missing autosave_0 should not block rotation before save");
	Expect(result.save.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Saved, "empty autosave directory should preserve save diagnostics");
	ExpectAuthoritativeSessionSame(LoadAutoSession("autosave_0"), session, "empty autosave directory should save current session");
}

void TestExistingAutosavesRotateAndWriteNewest()
{
	ResetTempRoot();
	WriteAutoFile("autosave_0", { 0 });
	WriteAutoFile("autosave_1", { 1 });
	WriteAutoFile("autosave_2", { 2 });
	const iggy::runtime::RuntimeSessionState session = Session("level:rotated");

	const iggy::runtime::RuntimeAutosaveRotationExecutionResult result = iggy::runtime::RuntimeAutosaveRotationExecutor {}.saveAutosave(
		session,
		PathConfig(),
		3);

	Expect(result.status == iggy::runtime::RuntimeAutosaveRotationExecutionStatus::Saved, "existing autosaves should rotate and save");
	Expect(result.deletions.size() == 1, "rotation should record oldest deletion");
	Expect(result.deletions[0].status == iggy::runtime::RuntimeSaveSlotDeletionStatus::Deleted, "oldest autosave should be deleted before renames");
	Expect(result.renames.size() == 2, "rotation should record renames");
	Expect(result.renames[0].status == iggy::runtime::RuntimeSaveSlotRenameStatus::Renamed, "autosave_1 should rename to autosave_2");
	Expect(result.renames[1].status == iggy::runtime::RuntimeSaveSlotRenameStatus::Renamed, "autosave_0 should rename to autosave_1");
	Expect(ReadBytes(AutoPath("autosave_1")) == std::vector<std::uint8_t>({ 0 }), "autosave_0 bytes should move to autosave_1");
	Expect(ReadBytes(AutoPath("autosave_2")) == std::vector<std::uint8_t>({ 1 }), "autosave_1 bytes should move to autosave_2");
	ExpectAuthoritativeSessionSame(LoadAutoSession("autosave_0"), session, "autosave_0 should contain newly saved session");
}

void TestDeleteFailureStopsBeforeRenamesAndSave()
{
	ResetTempRoot();
	CreateAutoDirectory();
	std::filesystem::create_directories(AutoPath("autosave_1") / "child");
	WriteAutoFile("autosave_0", { 3 });

	const iggy::runtime::RuntimeAutosaveRotationExecutionResult result = iggy::runtime::RuntimeAutosaveRotationExecutor {}.saveAutosave(
		Session(),
		PathConfig(),
		2);

	Expect(result.status == iggy::runtime::RuntimeAutosaveRotationExecutionStatus::DeleteFailed, "delete failure should stop rotation");
	Expect(result.deletions.size() == 1, "delete failure should preserve deletion diagnostics");
	Expect(result.deletions[0].status == iggy::runtime::RuntimeSaveSlotDeletionStatus::DeleteFailed, "directory at delete slot should fail deletion");
	Expect(result.renames.empty(), "delete failure should stop before renames");
	Expect(result.save.status == iggy::runtime::RuntimeSaveSlotStoreStatus::InvalidSlotPath, "delete failure should stop before save");
	Expect(ReadBytes(AutoPath("autosave_0")) == std::vector<std::uint8_t>({ 3 }), "delete failure should leave source autosave in place");
}

void TestRenameFailureStopsBeforeFinalSave()
{
	ResetTempRoot();
	CreateAutoDirectory();
	std::filesystem::create_directories(AutoPath("autosave_0") / "child");

	const iggy::runtime::RuntimeAutosaveRotationExecutionResult result = iggy::runtime::RuntimeAutosaveRotationExecutor {}.saveAutosave(
		Session(),
		PathConfig(),
		2);

	Expect(result.status == iggy::runtime::RuntimeAutosaveRotationExecutionStatus::RenameFailed, "rename failure should stop rotation");
	Expect(result.deletions.size() == 1, "rename failure should preserve deletion diagnostics");
	Expect(result.deletions[0].status == iggy::runtime::RuntimeSaveSlotDeletionStatus::NotFound, "missing oldest should not block rename failure test");
	Expect(result.renames.size() == 1, "rename failure should preserve rename diagnostics");
	Expect(result.renames[0].status == iggy::runtime::RuntimeSaveSlotRenameStatus::RenameFailed, "source directory should fail rename");
	Expect(result.save.status == iggy::runtime::RuntimeSaveSlotStoreStatus::InvalidSlotPath, "rename failure should stop before save");
	Expect(std::filesystem::exists(AutoPath("autosave_0") / "child"), "rename failure should leave source directory intact");
}

void TestFinalSaveFailureReportsSaveFailed()
{
	ResetTempRoot();

	const iggy::runtime::RuntimeAutosaveRotationExecutionResult result = iggy::runtime::RuntimeAutosaveRotationExecutor {}.saveAutosave(
		Session(),
		PathConfig(),
		1);

	Expect(result.status == iggy::runtime::RuntimeAutosaveRotationExecutionStatus::SaveFailed, "missing auto directory should make final save fail");
	Expect(result.deletions.empty(), "maxSlots 1 save failure should have no deletion diagnostics");
	Expect(result.renames.empty(), "maxSlots 1 save failure should have no rename diagnostics");
	Expect(result.save.status == iggy::runtime::RuntimeSaveSlotStoreStatus::SaveFailed, "save failure should preserve slot store diagnostics");
}

void TestInputsAreNotMutated()
{
	ResetTempRoot();
	CreateAutoDirectory();
	iggy::runtime::RuntimeSessionState session = Session("level:immutable");
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	iggy::runtime::RuntimeSaveSlotPathPolicyConfig pathConfig = PathConfig();
	const iggy::runtime::RuntimeSaveSlotPathPolicyConfig pathConfigBefore = pathConfig;

	const iggy::runtime::RuntimeAutosaveRotationExecutionResult result = iggy::runtime::RuntimeAutosaveRotationExecutor {}.saveAutosave(
		session,
		pathConfig,
		1);

	Expect(result.status == iggy::runtime::RuntimeAutosaveRotationExecutionStatus::Saved, "immutability setup should save autosave");
	ExpectAuthoritativeSessionSame(session, sessionBefore, "autosave rotation executor should not mutate source session");
	Expect(pathConfig.baseDirectory == pathConfigBefore.baseDirectory, "autosave rotation executor should not mutate path base directory");
	Expect(pathConfig.extension == pathConfigBefore.extension, "autosave rotation executor should not mutate path extension");
}

} // namespace

int main()
{
	TestMaxSlotsZeroReturnsInvalidPlanAndTouchesNoFiles();
	TestEmptyAutosaveDirectorySavesNewAutosave();
	TestExistingAutosavesRotateAndWriteNewest();
	TestDeleteFailureStopsBeforeRenamesAndSave();
	TestRenameFailureStopsBeforeFinalSave();
	TestFinalSaveFailureReportsSaveFailed();
	TestInputsAreNotMutated();

	RemoveTempRoot();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
