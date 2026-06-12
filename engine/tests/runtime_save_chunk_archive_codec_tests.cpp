#include <array>
#include <cstdlib>
#include <vector>

#include "runtime/RuntimeBinaryCodec.hpp"
#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::runtime::RuntimeSaveChunkId SessionId = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'S');
const iggy::runtime::RuntimeSaveChunkId LevelMapId = iggy::runtime::makeRuntimeSaveChunkId('L', 'M', 'A', 'P');

iggy::runtime::RuntimeSaveChunk Chunk(
	iggy::runtime::RuntimeSaveChunkId id,
	std::uint32_t version = 1,
	std::vector<std::uint8_t> payload = {})
{
	return { id, version, payload };
}

bool HasIssue(
	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult &result,
	iggy::runtime::RuntimeSaveChunkArchiveCodecIssueCode code,
	std::size_t chunkIndex = 0)
{
	for (const iggy::runtime::RuntimeSaveChunkArchiveCodecIssue &issue : result.issues) {
		if (issue.code == code && issue.chunkIndex == chunkIndex)
			return true;
	}
	return false;
}

bool HasValidationIssue(
	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult &result,
	iggy::runtime::RuntimeSaveChunkArchiveIssueCode code,
	std::size_t chunkIndex = 0)
{
	for (const iggy::runtime::RuntimeSaveChunkArchiveIssue &issue : result.issues) {
		if (issue.code == code && issue.chunkIndex == chunkIndex)
			return true;
	}
	return false;
}

iggy::runtime::RuntimeSaveChunkArchive Archive(std::vector<iggy::runtime::RuntimeSaveChunk> chunks)
{
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.chunks = chunks;
	return archive;
}

std::vector<std::uint8_t> HeaderOnlyBytes(std::array<char, 4> magic = { 'I', 'G', 'G', 'Y' }, std::uint32_t version = 1, std::uint32_t chunkCount = 0)
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeBytes({
		static_cast<std::uint8_t>(magic[0]),
		static_cast<std::uint8_t>(magic[1]),
		static_cast<std::uint8_t>(magic[2]),
		static_cast<std::uint8_t>(magic[3]),
	});
	writer.writeU32LE(version);
	writer.writeU32LE(chunkCount);
	return writer.takeBytes();
}

std::vector<std::uint8_t> BytesWithChunkHeader(
	iggy::runtime::RuntimeSaveChunkId id,
	std::uint32_t chunkVersion,
	std::uint32_t payloadSize,
	std::vector<std::uint8_t> payload)
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeBytes(HeaderOnlyBytes({ 'I', 'G', 'G', 'Y' }, 1, 1));
	writer.writeBytes({
		static_cast<std::uint8_t>(id.value[0]),
		static_cast<std::uint8_t>(id.value[1]),
		static_cast<std::uint8_t>(id.value[2]),
		static_cast<std::uint8_t>(id.value[3]),
	});
	writer.writeU32LE(chunkVersion);
	writer.writeU32LE(payloadSize);
	writer.writeBytes(payload);
	return writer.takeBytes();
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

void TestEmptyArchiveEncodesAndDecodes()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive;

	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult encoded = iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);
	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(encoded.bytes);

	Expect(encoded.encoded, "empty archive should encode");
	Expect(encoded.validation.valid, "empty archive encode should preserve valid validation");
	Expect(encoded.bytes == HeaderOnlyBytes(), "empty archive should encode header with chunk count zero");
	Expect(decoded.decoded, "empty archive bytes should decode");
	ExpectSameArchive(decoded.archive, archive, "empty archive should round trip");
}

void TestOneChunkRoundTrip()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = Archive({ Chunk(SessionId, 2, { 0xAA, 0xBB }) });

	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult encoded = iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);
	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(encoded.bytes);

	Expect(encoded.encoded, "one chunk archive should encode");
	Expect(decoded.decoded, "one chunk archive should decode");
	ExpectSameArchive(decoded.archive, archive, "one chunk archive should round trip");
}

void TestMultipleChunksPreserveOrderAndDuplicateIds()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = Archive({
		Chunk(SessionId, 1, { 1 }),
		Chunk(LevelMapId, 3, { 2, 3 }),
		Chunk(SessionId, 4, { 4 }),
	});

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(
		iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive).bytes);

	Expect(decoded.decoded, "multiple chunk archive should decode");
	ExpectSameArchive(decoded.archive, archive, "multiple chunk archive should preserve order and duplicate ids");
}

void TestEmptyPayloadRoundTrips()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = Archive({ Chunk(SessionId, 1, {}) });

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(
		iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive).bytes);

	Expect(decoded.decoded, "empty payload chunk should decode");
	Expect(decoded.archive.chunks.size() == 1, "empty payload archive should contain one chunk");
	if (decoded.archive.chunks.size() == 1)
		Expect(decoded.archive.chunks[0].payload.empty(), "empty payload should round trip empty");
}

void TestInvalidArchiveFailsEncode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = Archive({ Chunk({}, 0, {}) });

	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult encoded = iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);

	Expect(!encoded.encoded, "invalid archive should not encode");
	Expect(encoded.bytes.empty(), "invalid archive encode should not publish bytes");
	Expect(!encoded.validation.valid, "invalid archive encode should preserve validation failure");
	Expect(HasValidationIssue(encoded.validation, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::EmptyChunkId, 0), "invalid archive encode should preserve empty id issue");
	Expect(HasValidationIssue(encoded.validation, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidChunkVersion, 0), "invalid archive encode should preserve invalid chunk version issue");
}

void TestTruncatedHeaderFailsDecode()
{
	const std::vector<std::uint8_t> bytes { 'I', 'G' };

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "truncated header should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveChunkArchiveCodecIssueCode::TruncatedInput), "truncated header should report truncated input");
}

void TestTruncatedChunkHeaderFailsDecode()
{
	std::vector<std::uint8_t> bytes = HeaderOnlyBytes({ 'I', 'G', 'G', 'Y' }, 1, 1);
	bytes.push_back(static_cast<std::uint8_t>('S'));
	bytes.push_back(static_cast<std::uint8_t>('E'));

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "truncated chunk header should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveChunkArchiveCodecIssueCode::TruncatedInput, 0), "truncated chunk header should report truncated input with chunk index");
}

void TestPayloadSizeLargerThanRemainingFailsDecode()
{
	const std::vector<std::uint8_t> bytes = BytesWithChunkHeader(SessionId, 1, 3, { 0xAA });

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "oversized payload should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveChunkArchiveCodecIssueCode::InvalidPayloadSize, 0), "oversized payload should report invalid payload size");
}

void TestTrailingBytesFailDecode()
{
	std::vector<std::uint8_t> bytes = HeaderOnlyBytes();
	bytes.push_back(0x99);

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "trailing bytes should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveChunkArchiveCodecIssueCode::TrailingBytes), "trailing bytes should report issue");
}

void TestDecodedArchiveValidationFailureSurfacedForInvalidMagic()
{
	const std::vector<std::uint8_t> bytes = HeaderOnlyBytes({ 'B', 'A', 'D', '!' }, 1, 0);

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "invalid decoded magic should fail validation");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveChunkArchiveCodecIssueCode::InvalidArchive), "invalid decoded magic should report invalid archive");
	Expect(HasValidationIssue(decoded.validation, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidMagic), "invalid decoded magic should preserve validation issue");
}

void TestDecodedArchiveValidationFailureSurfacedForArchiveVersion()
{
	const std::vector<std::uint8_t> bytes = HeaderOnlyBytes({ 'I', 'G', 'G', 'Y' }, 0, 0);

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "invalid decoded archive version should fail validation");
	Expect(HasValidationIssue(decoded.validation, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidArchiveVersion), "invalid decoded archive version should preserve validation issue");
}

void TestDecodedArchiveValidationFailureSurfacedForChunkVersionAndEmptyId()
{
	const std::vector<std::uint8_t> bytes = BytesWithChunkHeader({}, 0, 0, {});

	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decoded = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "invalid decoded chunk should fail validation");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveChunkArchiveCodecIssueCode::InvalidArchive), "invalid decoded chunk should report invalid archive");
	Expect(HasValidationIssue(decoded.validation, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::EmptyChunkId, 0), "empty decoded chunk id should preserve validation issue");
	Expect(HasValidationIssue(decoded.validation, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidChunkVersion, 0), "invalid decoded chunk version should preserve validation issue");
}

void TestKnownArchiveExactLittleEndianBytes()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = Archive({ Chunk(SessionId, 2, { 0xAA, 0xBB }) });

	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult encoded = iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);

	Expect(
		encoded.bytes == std::vector<std::uint8_t> {
			'I', 'G', 'G', 'Y',
			0x01, 0x00, 0x00, 0x00,
			0x01, 0x00, 0x00, 0x00,
			'S', 'E', 'S', 'S',
			0x02, 0x00, 0x00, 0x00,
			0x02, 0x00, 0x00, 0x00,
			0xAA, 0xBB,
		},
		"archive codec should write exact known little-endian bytes");
}

} // namespace

int main()
{
	TestEmptyArchiveEncodesAndDecodes();
	TestOneChunkRoundTrip();
	TestMultipleChunksPreserveOrderAndDuplicateIds();
	TestEmptyPayloadRoundTrips();
	TestInvalidArchiveFailsEncode();
	TestTruncatedHeaderFailsDecode();
	TestTruncatedChunkHeaderFailsDecode();
	TestPayloadSizeLargerThanRemainingFailsDecode();
	TestTrailingBytesFailDecode();
	TestDecodedArchiveValidationFailureSurfacedForInvalidMagic();
	TestDecodedArchiveValidationFailureSurfacedForArchiveVersion();
	TestDecodedArchiveValidationFailureSurfacedForChunkVersionAndEmptyId();
	TestKnownArchiveExactLittleEndianBytes();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
