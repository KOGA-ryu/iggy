#include <cstdlib>
#include <filesystem>
#include <vector>

#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::runtime::RuntimeSaveChunkId SessionId = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'S');

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_save_file_io_tests_tmp";
}

std::filesystem::path TempPath(const char *name)
{
	return TempRoot() / name;
}

std::filesystem::path TempPathFor(const std::filesystem::path &path)
{
	std::filesystem::path tempPath = path;
	tempPath += ".tmp.iggy";
	return tempPath;
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

iggy::runtime::RuntimeSaveChunkArchive Archive()
{
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.chunks.push_back({ SessionId, 2, { 0xAA, 0xBB, 0xCC } });
	return archive;
}

void ExpectSameArchive(
	const iggy::runtime::RuntimeSaveChunkArchive &actual,
	const iggy::runtime::RuntimeSaveChunkArchive &expected,
	const char *message)
{
	Expect(actual.magic == expected.magic, message);
	Expect(actual.version == expected.version, message);
	Expect(actual.chunks.size() == expected.chunks.size(), message);
	for (std::size_t index = 0; index < actual.chunks.size() && index < expected.chunks.size(); ++index) {
		Expect(actual.chunks[index].id == expected.chunks[index].id, message);
		Expect(actual.chunks[index].version == expected.chunks[index].version, message);
		Expect(actual.chunks[index].payload == expected.chunks[index].payload, message);
	}
}

void TestWriteAndReadNonEmptyBytes()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("non_empty.iggy");
	const std::vector<std::uint8_t> bytes { 1, 2, 3, 4 };

	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, bytes);
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);

	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "non-empty write should succeed");
	Expect(write.path == path, "write result should preserve target path");
	Expect(write.tempPath == TempPathFor(path), "write result should report temp path");
	Expect(write.bytesWritten == bytes.size(), "write result should report byte count");
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "non-empty read should succeed");
	Expect(read.path == path, "read result should preserve target path");
	Expect(read.bytes == bytes, "read bytes should match written bytes");
}

void TestWriteAndReadEmptyBytes()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("empty.iggy");

	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, {});
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);

	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "empty write should succeed");
	Expect(write.bytesWritten == 0, "empty write should report zero bytes");
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "empty read should succeed");
	Expect(read.bytes.empty(), "empty file should read as empty bytes");
}

void TestWriteReplacesExistingFile()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("replace.iggy");
	const std::vector<std::uint8_t> oldBytes { 9, 9, 9 };
	const std::vector<std::uint8_t> newBytes { 1, 2 };
	Expect(iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, oldBytes).status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "replace setup should write old bytes");

	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, newBytes);
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);

	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "replacement write should succeed");
	Expect(read.bytes == newBytes, "replacement write should replace old file contents");
}

void TestMissingParentDirectoryWriteFails()
{
	ResetTempRoot();
	const std::filesystem::path parent = TempPath("missing_parent");
	const std::filesystem::path path = parent / "save.iggy";

	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, { 1 });

	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::OpenFailed, "write with missing parent should fail open");
	Expect(!std::filesystem::exists(parent), "write with missing parent should not create parent directory");
	Expect(!std::filesystem::exists(TempPathFor(path)), "write with missing parent should not leave temp file");
}

void TestEmptyPathWriteAndReadInvalid()
{
	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes({}, { 1 });
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes({});

	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::InvalidPath, "empty write path should be invalid");
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::InvalidPath, "empty read path should be invalid");
}

void TestReadMissingFileOpenFailed()
{
	ResetTempRoot();

	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(TempPath("missing.iggy"));

	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::OpenFailed, "missing file read should fail open");
	Expect(read.bytes.empty(), "missing file read should not publish bytes");
}

void TestTempFileRemovedAfterSuccessfulWrite()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("temp_cleanup_success.iggy");

	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, { 1, 2, 3 });

	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "temp cleanup success setup should write");
	Expect(!std::filesystem::exists(write.tempPath), "successful write should not leave temp file");
}

void TestTempFileRemovedAfterReplaceFailure()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("blocked_final.iggy");
	std::filesystem::create_directories(path / "child");

	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, { 1, 2, 3 });

	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::ReplaceFailed, "non-empty directory target should fail replacement");
	Expect(!std::filesystem::exists(write.tempPath), "replace failure should best-effort remove temp file");
	Expect(std::filesystem::exists(path / "child"), "replace failure should leave existing directory target intact");
}

void TestInputBytesAreNotMutated()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("immutability.iggy");
	std::vector<std::uint8_t> bytes { 4, 5, 6 };
	const std::vector<std::uint8_t> before = bytes;

	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, bytes);

	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "immutability write should succeed");
	Expect(bytes == before, "writer should not mutate input bytes");
}

void TestArchiveEnvelopeIoIntegration()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("integration.iggy");
	const iggy::runtime::RuntimeSaveChunkArchive archive = Archive();
	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult archiveBytes = iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);
	const iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult envelopeBytes = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload(archiveBytes.bytes);

	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, envelopeBytes.bytes);
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);
	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult envelope = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(read.bytes);
	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decodedArchive = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(envelope.envelope.payload);

	Expect(archiveBytes.encoded, "IO integration setup should encode archive bytes");
	Expect(envelopeBytes.encoded, "IO integration setup should encode envelope bytes");
	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "IO integration should write envelope bytes");
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "IO integration should read envelope bytes");
	Expect(read.bytes == envelopeBytes.bytes, "IO integration should be byte-transparent");
	Expect(envelope.decoded, "IO integration should decode envelope after read");
	Expect(decodedArchive.decoded, "IO integration should decode archive after read");
	ExpectSameArchive(decodedArchive.archive, archive, "IO integration should preserve archive through file bytes");
}

} // namespace

int main()
{
	TestWriteAndReadNonEmptyBytes();
	TestWriteAndReadEmptyBytes();
	TestWriteReplacesExistingFile();
	TestMissingParentDirectoryWriteFails();
	TestEmptyPathWriteAndReadInvalid();
	TestReadMissingFileOpenFailed();
	TestTempFileRemovedAfterSuccessfulWrite();
	TestTempFileRemovedAfterReplaceFailure();
	TestInputBytesAreNotMutated();
	TestArchiveEnvelopeIoIntegration();

	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
