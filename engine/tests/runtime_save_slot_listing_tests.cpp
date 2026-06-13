#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"
#include "runtime/RuntimeSaveSlotListing.hpp"
#include "runtime/RuntimeSessionSaveLoad.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::PlayerAgent;

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_save_slot_listing_tests_tmp";
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

iggy::runtime::RuntimeSaveSlotPathPolicyConfig Config(
	std::filesystem::path base = TempRoot(),
	std::string extension = ".igsave")
{
	return { base, extension };
}

std::filesystem::path SlotDirectory(iggy::runtime::RuntimeSaveSlotKind kind)
{
	const char *name = "manual";
	if (kind == iggy::runtime::RuntimeSaveSlotKind::Auto)
		name = "auto";
	if (kind == iggy::runtime::RuntimeSaveSlotKind::Quick)
		name = "quick";
	return TempRoot() / name;
}

void CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind kind)
{
	std::filesystem::create_directories(SlotDirectory(kind));
}

void CreateAllSlotDirectories()
{
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Auto);
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Quick);
}

iggy::runtime::RuntimeSaveSlotId Slot(iggy::runtime::RuntimeSaveSlotKind kind, std::string name)
{
	return { kind, name };
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows = { "..", ".." })
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = iggy::ResourceId("level:slot-listing");
	level.map.playerStart = { 0, 0 };
	return level;
}

iggy::runtime::RuntimeSessionState Session()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = Level();
	session.tickIndex = 5;
	session.hasPlayer = true;
	session.player = PlayerAgent(iggy::ResourceId("player:slot-listing"), { 0.5F, 0.5F }, { 0, 0 });
	return session;
}

std::filesystem::path PathFor(
	iggy::runtime::RuntimeSaveSlotKind kind,
	const std::string &name,
	const std::string &extension = ".igsave")
{
	iggy::runtime::RuntimeSaveSlotPathPolicyConfig config = Config(TempRoot(), extension);
	const iggy::runtime::RuntimeSaveSlotPathResult path = iggy::runtime::RuntimeSaveSlotPathPolicy {}.pathFor(config, Slot(kind, name));
	Expect(path.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "slot listing fixture should build valid path");
	return path.path;
}

void WriteBytes(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes)
{
	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, bytes);
	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "slot listing fixture should write bytes");
}

void WriteEnvelopePayload(const std::filesystem::path &path, const std::vector<std::uint8_t> &payload)
{
	const iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult envelope = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload(payload);
	Expect(envelope.encoded, "slot listing fixture should encode envelope");
	WriteBytes(path, envelope.bytes);
}

void WriteArchive(const std::filesystem::path &path, iggy::runtime::RuntimeSaveChunkArchive archive)
{
	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult archiveBytes = iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);
	Expect(archiveBytes.encoded, "slot listing fixture should encode archive");
	WriteEnvelopePayload(path, archiveBytes.bytes);
}

iggy::runtime::RuntimeSaveChunkArchive ArchiveWithMetadata(std::string displayName, std::uint64_t createdTick)
{
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.chunks.push_back(iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode({ displayName, createdTick }));
	return archive;
}

bool HasIssue(const iggy::runtime::RuntimeSaveSlotListingResult &result, iggy::runtime::RuntimeSaveSlotListingIssueCode code)
{
	for (const iggy::runtime::RuntimeSaveSlotListingIssue &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void SaveSlot(
	iggy::runtime::RuntimeSaveSlotKind kind,
	const std::string &name,
	const iggy::runtime::RuntimeSaveMetadata *metadata = nullptr)
{
	const std::filesystem::path path = PathFor(kind, name);
	if (metadata != nullptr) {
		const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(Session(), path, *metadata);
		Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::Saved, "slot listing fixture should save with metadata");
		return;
	}

	const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(Session(), path);
	Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::Saved, "slot listing fixture should save without metadata");
}

void TestMissingBaseDirectoryReportsIssueWithoutCrash()
{
	RemoveTempRoot();

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.listed, "missing base directory should still produce listed result");
	Expect(result.entries.empty(), "missing base directory should have no entries");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveSlotListingIssueCode::BaseDirectoryMissing), "missing base directory should report issue");
	Expect(!std::filesystem::exists(TempRoot()), "listing missing base directory should not create directories");
}

void TestEmptyBaseDirectoryFailsListing()
{
	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config({}));

	Expect(!result.listed, "empty base directory should not list");
	Expect(result.entries.empty(), "empty base directory should have no entries");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveSlotListingIssueCode::BaseDirectoryMissing), "empty base directory should report base issue");
}

void TestMissingSlotDirectoriesReportAndContinue()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Manual, "manual_1");

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.listed, "missing slot directories should still list");
	Expect(result.entries.size() == 1, "listing should include entries from existing slot directories");
	Expect(result.entries.size() == 1 && result.entries[0].slot.name == "manual_1", "listing should preserve discovered manual slot");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveSlotListingIssueCode::SlotDirectoryMissing), "missing slot directory should report issue");
}

void TestManualAutoQuickDiscoveryAndOrdering()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Quick, "quick_1");
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Manual, "b_slot");
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1");
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Manual, "a_slot");

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.listed, "valid slot directories should list");
	Expect(result.issues.empty(), "valid slot listing should have no issues");
	Expect(result.entries.size() == 4, "listing should discover manual auto and quick saves");
	if (result.entries.size() == 4) {
		Expect(result.entries[0].slot.kind == iggy::runtime::RuntimeSaveSlotKind::Manual && result.entries[0].slot.name == "a_slot", "listing should sort manual filenames");
		Expect(result.entries[1].slot.kind == iggy::runtime::RuntimeSaveSlotKind::Manual && result.entries[1].slot.name == "b_slot", "listing should keep manual kind before auto");
		Expect(result.entries[2].slot.kind == iggy::runtime::RuntimeSaveSlotKind::Auto && result.entries[2].slot.name == "auto_1", "listing should keep auto kind before quick");
		Expect(result.entries[3].slot.kind == iggy::runtime::RuntimeSaveSlotKind::Quick && result.entries[3].slot.name == "quick_1", "listing should list quick last");
	}
}

void TestInvalidFilenamesAreReportedAndSkipped()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Manual, "valid");
	WriteArchive(SlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual) / "bad.name.igsave", ArchiveWithMetadata("Bad", 1));

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.entries.size() == 1, "invalid slot filename should be skipped");
	Expect(result.entries.size() == 1 && result.entries[0].slot.name == "valid", "valid slot should still be listed");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveSlotListingIssueCode::InvalidSlotFileName), "invalid slot filename should report issue");
}

void TestNonSaveExtensionsAreIgnored()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	WriteArchive(SlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual) / "notes.txt", ArchiveWithMetadata("Notes", 1));

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.listed, "listing with non-save extension should still succeed");
	Expect(result.entries.empty(), "non-save extension should be ignored");
	Expect(result.issues.empty(), "ignored non-save extension should not report issue");
}

void TestEmptyExtensionListsFilesWithoutExtensionOnly()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	WriteArchive(SlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual) / "plain", ArchiveWithMetadata("Plain", 1));
	WriteArchive(SlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual) / "ignored.igsave", ArchiveWithMetadata("Ignored", 1));

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config(TempRoot(), ""));

	Expect(result.entries.size() == 1, "empty extension should list only files without extension");
	Expect(result.entries.size() == 1 && result.entries[0].slot.name == "plain", "empty extension should derive slot from filename");
}

void TestValidMetadataIsDecodedWithoutSnapshotRestore()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	WriteArchive(PathFor(iggy::runtime::RuntimeSaveSlotKind::Manual, "meta"), ArchiveWithMetadata("Manual Save", 77));

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.entries.size() == 1, "metadata-only archive should still be listed");
	if (result.entries.size() == 1) {
		Expect(result.entries[0].hasMetadata, "valid META should be decoded");
		Expect(result.entries[0].metadata.displayName == "Manual Save", "listing should preserve metadata display name");
		Expect(result.entries[0].metadata.createdTick == 77, "listing should preserve metadata created tick");
	}
}

void TestSaveWithoutMetadataListsEntryWithoutMetadata()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Manual, "no_meta");

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.entries.size() == 1, "save without META should still be listed");
	Expect(result.entries.size() == 1 && !result.entries[0].hasMetadata, "save without META should report metadata absent");
	Expect(result.issues.empty(), "save without META should not report metadata issue");
}

void TestMalformedMetadataReportsIssueAndKeepsEntry()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.chunks.push_back({ iggy::runtime::runtimeSaveMetadataChunkId(), iggy::runtime::RuntimeSaveMetadataChunkVersion, { 0x04, 0x00, 0x00, 0x00, 'B', 'a' } });
	WriteArchive(PathFor(iggy::runtime::RuntimeSaveSlotKind::Manual, "bad_meta"), archive);
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Manual, "good");

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.entries.size() == 2, "malformed META should not prevent entry or other saves from listing");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveSlotListingIssueCode::MetadataDecodeFailed), "malformed META should report metadata decode issue");
	for (const iggy::runtime::RuntimeSaveSlotListingEntry &entry : result.entries) {
		if (entry.slot.name == "bad_meta")
			Expect(!entry.hasMetadata, "malformed META entry should be listed without metadata");
	}
}

void TestDuplicateMetadataReportsIssueAndKeepsEntry()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.chunks.push_back(iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode({ "One", 1 }));
	archive.chunks.push_back(iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode({ "Two", 2 }));
	WriteArchive(PathFor(iggy::runtime::RuntimeSaveSlotKind::Manual, "dup_meta"), archive);

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.entries.size() == 1, "duplicate META should keep entry listed");
	Expect(result.entries.size() == 1 && !result.entries[0].hasMetadata, "duplicate META should list entry without metadata");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveSlotListingIssueCode::DuplicateMetadata), "duplicate META should report issue");
}

void TestCorruptEnvelopeAndArchiveAreSkipped()
{
	ResetTempRoot();
	CreateAllSlotDirectories();
	WriteBytes(PathFor(iggy::runtime::RuntimeSaveSlotKind::Manual, "bad_envelope"), { 1, 2, 3 });
	WriteEnvelopePayload(PathFor(iggy::runtime::RuntimeSaveSlotKind::Manual, "bad_archive"), { 'I', 'G' });
	SaveSlot(iggy::runtime::RuntimeSaveSlotKind::Manual, "good");

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.entries.size() == 1, "corrupt envelope/archive saves should be skipped while valid saves list");
	Expect(result.entries.size() == 1 && result.entries[0].slot.name == "good", "valid save should remain listed after corrupt files");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveSlotListingIssueCode::EnvelopeDecodeFailed), "corrupt envelope should report issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveSlotListingIssueCode::ArchiveDecodeFailed), "corrupt archive should report issue");
}

void TestListingDoesNotCreateDirectories()
{
	RemoveTempRoot();

	const iggy::runtime::RuntimeSaveSlotListingResult result = iggy::runtime::RuntimeSaveSlotListing {}.list(Config());

	Expect(result.listed, "missing base listing should return listed result");
	Expect(!std::filesystem::exists(TempRoot()), "listing should not create missing base directory");
}

} // namespace

int main()
{
	TestMissingBaseDirectoryReportsIssueWithoutCrash();
	TestEmptyBaseDirectoryFailsListing();
	TestMissingSlotDirectoriesReportAndContinue();
	TestManualAutoQuickDiscoveryAndOrdering();
	TestInvalidFilenamesAreReportedAndSkipped();
	TestNonSaveExtensionsAreIgnored();
	TestEmptyExtensionListsFilesWithoutExtensionOnly();
	TestValidMetadataIsDecodedWithoutSnapshotRestore();
	TestSaveWithoutMetadataListsEntryWithoutMetadata();
	TestMalformedMetadataReportsIssueAndKeepsEntry();
	TestDuplicateMetadataReportsIssueAndKeepsEntry();
	TestCorruptEnvelopeAndArchiveAreSkipped();
	TestListingDoesNotCreateDirectories();

	RemoveTempRoot();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
