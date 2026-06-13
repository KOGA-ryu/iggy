#include <cstdlib>
#include <filesystem>
#include <vector>

#include "runtime/RuntimeSaveFileIO.hpp"
#include "runtime/RuntimeSaveSlotPathPolicy.hpp"
#include "runtime/RuntimeSaveSlotRename.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_save_slot_rename_tests_tmp";
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
	Expect(path.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "slot rename fixture should build valid slot path");
	return path.path;
}

void WriteSlotFile(
	iggy::runtime::RuntimeSaveSlotKind kind,
	const std::string &name,
	std::vector<std::uint8_t> bytes = { 1, 2, 3 })
{
	CreateSlotDirectory(kind);
	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(SlotPath(kind, name), bytes);
	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "slot rename fixture should write slot file");
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path &path)
{
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "slot rename fixture should read slot file");
	return read.bytes;
}

void TestExistingSourceRenamesToMissingDestination()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "source", { 4, 5, 6 });
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const std::filesystem::path sourcePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "source");
	const std::filesystem::path destinationPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::Renamed, "existing source should rename to missing destination");
	Expect(result.sourceExisted, "renamed source should report existed");
	Expect(!result.destinationExisted, "missing destination should report existed false");
	Expect(!std::filesystem::exists(sourcePath), "renamed source path should disappear");
	Expect(ReadBytes(destinationPath) == std::vector<std::uint8_t>({ 4, 5, 6 }), "renamed destination should contain exact source bytes");
}

void TestExistingDestinationRegularFileIsReplaced()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "source", { 7, 8 });
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination", { 9, 9, 9 });
	const std::filesystem::path sourcePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "source");
	const std::filesystem::path destinationPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::Renamed, "existing destination regular file should be replaced");
	Expect(result.destinationExisted, "replacement destination should report existed");
	Expect(!std::filesystem::exists(sourcePath), "replaced source path should disappear");
	Expect(ReadBytes(destinationPath) == std::vector<std::uint8_t>({ 7, 8 }), "replacement destination should contain source bytes");
}

void TestMissingSourceReturnsSourceNotFound()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination", { 1 });
	const std::filesystem::path destinationPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "missing"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::SourceNotFound, "missing source should return SourceNotFound");
	Expect(!result.sourceExisted, "missing source should report existed false");
	Expect(result.destinationPath.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "missing source should still resolve destination");
	Expect(ReadBytes(destinationPath) == std::vector<std::uint8_t>({ 1 }), "missing source should leave destination unchanged");
}

void TestInvalidSourceSlotTouchesNothing()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "safe", { 2 });
	const std::filesystem::path safePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "safe");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "../safe"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "target"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::InvalidSourcePath, "invalid source slot should return InvalidSourcePath");
	Expect(result.sourcePath.status == iggy::runtime::RuntimeSaveSlotPathStatus::InvalidSlotName, "invalid source should preserve path diagnostics");
	Expect(std::filesystem::exists(safePath), "invalid source rename should not touch neighboring file");
}

void TestInvalidDestinationSlotTouchesNothing()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "source", { 3 });
	const std::filesystem::path sourcePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "source");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "../target"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::InvalidDestinationPath, "invalid destination slot should return InvalidDestinationPath");
	Expect(result.destinationPath.status == iggy::runtime::RuntimeSaveSlotPathStatus::InvalidSlotName, "invalid destination should preserve path diagnostics");
	Expect(ReadBytes(sourcePath) == std::vector<std::uint8_t>({ 3 }), "invalid destination rename should leave source unchanged");
}

void TestDestinationDirectoryReturnsDestinationNotRegularFile()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "source", { 4 });
	const std::filesystem::path sourcePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "source");
	const std::filesystem::path destinationPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination_dir");
	std::filesystem::create_directories(destinationPath / "child");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination_dir"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::DestinationNotRegularFile, "destination directory should return DestinationNotRegularFile");
	Expect(result.destinationExisted, "destination directory should report existed");
	Expect(ReadBytes(sourcePath) == std::vector<std::uint8_t>({ 4 }), "destination directory failure should leave source unchanged");
	Expect(std::filesystem::exists(destinationPath / "child"), "destination directory should remain intact");
}

void TestSourceDirectoryReturnsRenameFailed()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const std::filesystem::path sourcePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "source_dir");
	std::filesystem::create_directories(sourcePath / "child");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source_dir"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::RenameFailed, "source directory should return RenameFailed");
	Expect(result.sourceExisted, "source directory should report existed");
	Expect(std::filesystem::exists(sourcePath / "child"), "source directory should remain intact");
}

void TestMissingDestinationParentReturnsRenameFailed()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "source", { 5 });
	const std::filesystem::path sourcePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "source");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Auto, "destination"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::RenameFailed, "missing destination parent should return RenameFailed");
	Expect(result.sourceExisted, "missing destination parent should report source existed");
	Expect(ReadBytes(sourcePath) == std::vector<std::uint8_t>({ 5 }), "missing destination parent should leave source in place");
}

void TestSourceEqualsDestinationIsNoOpRename()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "source", { 6 });
	const std::filesystem::path sourcePath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "source");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::Renamed, "source equals destination should be a no-op rename");
	Expect(result.sourceExisted, "source equals destination should report source existed");
	Expect(result.destinationExisted, "source equals destination should report destination existed");
	Expect(ReadBytes(sourcePath) == std::vector<std::uint8_t>({ 6 }), "source equals destination should leave file bytes unchanged");
}

void TestSourceEqualsDestinationMissingReturnsSourceNotFound()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "missing"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "missing"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::SourceNotFound, "missing same source and destination should return SourceNotFound");
	Expect(!result.sourceExisted, "missing same source and destination should report source not existed");
	Expect(!result.destinationExisted, "missing same source and destination should report destination not existed");
}

void TestAutoAndQuickKindsWorkThroughPathPolicy()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1", { 10 });
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Quick);
	const std::filesystem::path autoPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1");
	const std::filesystem::path quickPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Quick, "quick_1");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Quick, "quick_1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::Renamed, "auto source should rename to quick destination through path policy");
	Expect(result.sourcePath.path == autoPath, "auto source path should use auto directory");
	Expect(result.destinationPath.path == quickPath, "quick destination path should use quick directory");
	Expect(!std::filesystem::exists(autoPath), "auto source should disappear after rename");
	Expect(ReadBytes(quickPath) == std::vector<std::uint8_t>({ 10 }), "quick destination should contain auto source bytes");
}

void TestNeighboringFilesUnaffected()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "source", { 11 });
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "neighbor", { 12 });
	const std::filesystem::path neighborPath = SlotPath(iggy::runtime::RuntimeSaveSlotKind::Manual, "neighbor");

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source"),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::Renamed, "neighbor fixture rename should succeed");
	Expect(ReadBytes(neighborPath) == std::vector<std::uint8_t>({ 12 }), "slot rename should not affect neighboring files");
}

void TestInputsAreNotMutated()
{
	ResetTempRoot();
	WriteSlotFile(iggy::runtime::RuntimeSaveSlotKind::Manual, "source", { 13 });
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	iggy::runtime::RuntimeSaveSlotPathPolicyConfig config = PathConfig();
	const iggy::runtime::RuntimeSaveSlotPathPolicyConfig configBefore = config;
	iggy::runtime::RuntimeSaveSlotId source = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "source");
	const iggy::runtime::RuntimeSaveSlotId sourceBefore = source;
	iggy::runtime::RuntimeSaveSlotId destination = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "destination");
	const iggy::runtime::RuntimeSaveSlotId destinationBefore = destination;

	const iggy::runtime::RuntimeSaveSlotRenameResult result = iggy::runtime::RuntimeSaveSlotRename {}.rename(config, source, destination);

	Expect(result.status == iggy::runtime::RuntimeSaveSlotRenameStatus::Renamed, "slot rename immutability setup should succeed");
	Expect(config.baseDirectory == configBefore.baseDirectory, "slot rename should not mutate base directory");
	Expect(config.extension == configBefore.extension, "slot rename should not mutate extension");
	Expect(source.kind == sourceBefore.kind, "slot rename should not mutate source kind");
	Expect(source.name == sourceBefore.name, "slot rename should not mutate source name");
	Expect(destination.kind == destinationBefore.kind, "slot rename should not mutate destination kind");
	Expect(destination.name == destinationBefore.name, "slot rename should not mutate destination name");
}

} // namespace

int main()
{
	TestExistingSourceRenamesToMissingDestination();
	TestExistingDestinationRegularFileIsReplaced();
	TestMissingSourceReturnsSourceNotFound();
	TestInvalidSourceSlotTouchesNothing();
	TestInvalidDestinationSlotTouchesNothing();
	TestDestinationDirectoryReturnsDestinationNotRegularFile();
	TestSourceDirectoryReturnsRenameFailed();
	TestMissingDestinationParentReturnsRenameFailed();
	TestSourceEqualsDestinationIsNoOpRename();
	TestSourceEqualsDestinationMissingReturnsSourceNotFound();
	TestAutoAndQuickKindsWorkThroughPathPolicy();
	TestNeighboringFilesUnaffected();
	TestInputsAreNotMutated();

	RemoveTempRoot();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
