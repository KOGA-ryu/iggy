#include <cstdlib>
#include <filesystem>
#include <vector>

#include "runtime/RuntimeSaveFileIO.hpp"
#include "runtime/RuntimeSaveSlotDeletion.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_save_slot_deletion_tests_tmp";
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

const char *DirectoryName(iggy::runtime::RuntimeSaveSlotKind kind)
{
	if (kind == iggy::runtime::RuntimeSaveSlotKind::Auto)
		return "auto";
	if (kind == iggy::runtime::RuntimeSaveSlotKind::Quick)
		return "quick";
	return "manual";
}

void CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind kind)
{
	std::filesystem::create_directories(TempRoot() / DirectoryName(kind));
}

iggy::runtime::RuntimeSaveSlotPathPolicyConfig PathConfig(std::filesystem::path base = TempRoot())
{
	return { base, ".igsave" };
}

iggy::runtime::RuntimeSaveSlotId Slot(
	iggy::runtime::RuntimeSaveSlotKind kind,
	std::string name)
{
	return { kind, name };
}

std::filesystem::path SlotPath(iggy::runtime::RuntimeSaveSlotKind kind, const std::string &name)
{
	const iggy::runtime::RuntimeSaveSlotPathResult path = iggy::runtime::RuntimeSaveSlotPathPolicy {}.pathFor(PathConfig(), Slot(kind, name));
	Expect(path.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "slot deletion fixture should build valid slot path");
	return path.path;
}

void WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind kind, const std::string &name, std::vector<std::uint8_t> bytes = { 1, 2, 3 })
{
	CreateSlotDirectory(kind);
	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(SlotPath(kind, name), bytes);
	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "slot deletion fixture should write slot file");
}

void TestExistingManualSlotFileDeleted()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	const std::filesystem::path path = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");

	const iggy::runtime::RuntimeSaveSlotDeletionResult result = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::Deleted, "existing manual slot should be deleted");
	Expect(result.existed, "deleted manual slot should report existed");
	Expect(result.path.path == path, "deleted manual slot should preserve resolved path");
	Expect(!std::filesystem::exists(path), "deleted manual slot file should be removed");
}

void TestMissingValidSlotReturnsNotFound()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);

	const iggy::runtime::RuntimeSaveSlotDeletionResult result = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "missing"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::NotFound, "missing valid slot should return NotFound");
	Expect(!result.existed, "missing valid slot should report existed false");
	Expect(result.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "missing valid slot should preserve valid path");
}

void TestInvalidSlotNameDoesNotDelete()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "safe");
	const std::filesystem::path safePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "safe");

	const iggy::runtime::RuntimeSaveSlotDeletionResult result = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "../safe"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::InvalidSlotPath, "invalid slot name should return InvalidSlotPath");
	Expect(result.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::InvalidSlotName, "invalid slot name should preserve path diagnostics");
	Expect(!result.existed, "invalid slot path should not check file existence");
	Expect(std::filesystem::exists(safePath), "invalid slot delete should not remove neighboring file");
}

void TestEmptyBaseDirectoryInvalid()
{
	const iggy::runtime::RuntimeSaveSlotDeletionResult result = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig({}), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::InvalidSlotPath, "empty base directory should return InvalidSlotPath");
	Expect(result.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::EmptyBaseDirectory, "empty base directory should preserve path diagnostics");
	Expect(!result.existed, "empty base directory should not check file existence");
}

void TestDirectoryAtSlotPathReturnsDeleteFailed()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const std::filesystem::path path = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot_dir");
	std::filesystem::create_directories(path / "child");

	const iggy::runtime::RuntimeSaveSlotDeletionResult result = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot_dir"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::DeleteFailed, "directory at slot path should fail deletion");
	Expect(result.existed, "directory at slot path should report existed");
	Expect(std::filesystem::exists(path / "child"), "directory at slot path should remain after failed deletion");
}

void TestAutoAndQuickSlotFilesDelete()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1");
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Quick, "quick_1");
	const std::filesystem::path autoPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1");
	const std::filesystem::path quickPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Quick, "quick_1");

	const iggy::runtime::RuntimeSaveSlotDeletionResult autoResult = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig(), Slot(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1"));
	const iggy::runtime::RuntimeSaveSlotDeletionResult quickResult = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig(), Slot(iggy::runtime::RuntimeSaveSlotKind::Quick, "quick_1"));

	Expect(autoResult.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::Deleted, "auto slot file should delete");
	Expect(quickResult.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::Deleted, "quick slot file should delete");
	Expect(!std::filesystem::exists(autoPath), "auto slot file should be removed");
	Expect(!std::filesystem::exists(quickPath), "quick slot file should be removed");
}

void TestDeletingOneSlotDoesNotAffectNeighbor()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot2");
	const std::filesystem::path slot1Path = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	const std::filesystem::path slot2Path = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot2");

	const iggy::runtime::RuntimeSaveSlotDeletionResult result = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::Deleted, "one slot deletion should succeed");
	Expect(!std::filesystem::exists(slot1Path), "deleted slot should be removed");
	Expect(std::filesystem::exists(slot2Path), "neighboring slot should remain");
}

void TestParentDirectoriesAreNotRemoved()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	const std::filesystem::path manualDirectory = TempRoot() / "manual";

	const iggy::runtime::RuntimeSaveSlotDeletionResult result = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(PathConfig(), Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::Deleted, "parent directory preservation setup should delete file");
	Expect(std::filesystem::exists(manualDirectory), "slot deletion should not remove parent directory");
}

void TestInputsAreNotMutated()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	iggy::runtime::RuntimeSaveSlotPathPolicyConfig config = PathConfig();
	const iggy::runtime::RuntimeSaveSlotPathPolicyConfig configBefore = config;
	iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	const iggy::runtime::RuntimeSaveSlotId slotBefore = slot;

	const iggy::runtime::RuntimeSaveSlotDeletionResult result = iggy::runtime::RuntimeSaveSlotDeletion {}.remove(config, slot);

	Expect(result.status == iggy::runtime::RuntimeSaveSlotDeletionStatus::Deleted, "slot deletion immutability setup should delete");
	Expect(config.baseDirectory == configBefore.baseDirectory, "slot deletion should not mutate base directory");
	Expect(config.extension == configBefore.extension, "slot deletion should not mutate extension");
	Expect(slot.kind == slotBefore.kind, "slot deletion should not mutate slot kind");
	Expect(slot.name == slotBefore.name, "slot deletion should not mutate slot name");
}

} // namespace

int main()
{
	TestExistingManualSlotFileDeleted();
	TestMissingValidSlotReturnsNotFound();
	TestInvalidSlotNameDoesNotDelete();
	TestEmptyBaseDirectoryInvalid();
	TestDirectoryAtSlotPathReturnsDeleteFailed();
	TestAutoAndQuickSlotFilesDelete();
	TestDeletingOneSlotDoesNotAffectNeighbor();
	TestParentDirectoriesAreNotRemoved();
	TestInputsAreNotMutated();

	RemoveTempRoot();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
