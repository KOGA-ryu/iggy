#include <cstdlib>
#include <vector>

#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::runtime::RuntimeSaveChunkId SessionId = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'S');

bool HasIssue(
	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult &result,
	iggy::runtime::RuntimeSaveFileEnvelopeIssueCode code)
{
	for (const iggy::runtime::RuntimeSaveFileEnvelopeIssue &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
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

void TestCrc32StandardVector()
{
	const std::vector<std::uint8_t> bytes { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	Expect(iggy::runtime::computeRuntimeSaveCrc32(bytes) == 0xCBF43926U, "CRC32 standard vector should match expected value");
}

void TestEmptyPayloadEncodesAndDecodes()
{
	const iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult encoded = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload({});
	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(encoded.bytes);

	Expect(encoded.encoded, "empty payload should encode");
	Expect(encoded.envelope.payload.empty(), "empty payload encode should preserve empty payload");
	Expect(encoded.envelope.payloadChecksum == 0U, "empty payload CRC32 should be zero");
	Expect(decoded.decoded, "empty payload envelope should decode");
	Expect(decoded.envelope.payload.empty(), "empty payload decode should preserve empty payload");
	Expect(decoded.envelope.payloadChecksum == 0U, "empty payload decode should preserve checksum");
}

void TestNonEmptyPayloadRoundTrips()
{
	const std::vector<std::uint8_t> payload { 1, 2, 3, 4, 5 };

	const iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult encoded = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload(payload);
	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(encoded.bytes);

	Expect(encoded.encoded, "non-empty payload should encode");
	Expect(encoded.envelope.payload == payload, "encode should copy payload exactly");
	Expect(decoded.decoded, "non-empty payload should decode");
	Expect(decoded.envelope.payload == payload, "decode should preserve payload exactly");
	Expect(decoded.envelope.payloadChecksum == iggy::runtime::computeRuntimeSaveCrc32(payload), "decode should preserve payload checksum");
}

void TestExactEnvelopeLayout()
{
	const std::vector<std::uint8_t> payload { 1, 2, 3 };
	const std::uint32_t checksum = iggy::runtime::computeRuntimeSaveCrc32(payload);

	const iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult encoded = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload(payload);

	Expect(
		encoded.bytes == std::vector<std::uint8_t> {
			'I', 'G', 'S', 'F',
			0x01, 0x00, 0x00, 0x00,
			0x01, 0x00, 0x00, 0x00,
			0x03, 0x00, 0x00, 0x00,
			static_cast<std::uint8_t>(checksum & 0xFFU),
			static_cast<std::uint8_t>((checksum >> 8U) & 0xFFU),
			static_cast<std::uint8_t>((checksum >> 16U) & 0xFFU),
			static_cast<std::uint8_t>((checksum >> 24U) & 0xFFU),
			0x01, 0x02, 0x03,
		},
		"envelope should encode exact magic/version/algorithm/size/checksum/payload layout");
}

void TestInvalidMagicFailsDecode()
{
	std::vector<std::uint8_t> bytes = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload({ 1 }).bytes;
	bytes[0] = 'B';

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "invalid magic should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveFileEnvelopeIssueCode::InvalidMagic), "invalid magic should report issue");
}

void TestUnsupportedEnvelopeVersionFailsDecode()
{
	std::vector<std::uint8_t> bytes = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload({ 1 }).bytes;
	bytes[4] = 2;

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "unsupported envelope version should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveFileEnvelopeIssueCode::UnsupportedEnvelopeVersion), "unsupported envelope version should report issue");
}

void TestUnsupportedChecksumAlgorithmFailsDecode()
{
	std::vector<std::uint8_t> bytes = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload({ 1 }).bytes;
	bytes[8] = 2;

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "unsupported checksum algorithm should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveFileEnvelopeIssueCode::UnsupportedChecksumAlgorithm), "unsupported checksum algorithm should report issue");
}

void TestTruncatedHeaderFailsDecode()
{
	const std::vector<std::uint8_t> bytes { 'I', 'G', 'S' };

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "truncated envelope header should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveFileEnvelopeIssueCode::TruncatedInput), "truncated envelope header should report issue");
}

void TestPayloadSizeLargerThanAvailableFailsDecode()
{
	std::vector<std::uint8_t> bytes = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload({ 1 }).bytes;
	bytes[12] = 4;

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "payload size larger than available bytes should fail decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveFileEnvelopeIssueCode::InvalidPayloadSize), "payload size larger than available bytes should report issue");
}

void TestTrailingBytesFailDecode()
{
	std::vector<std::uint8_t> bytes = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload({ 1 }).bytes;
	bytes.push_back(0x99);

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "trailing bytes should fail envelope decode");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveFileEnvelopeIssueCode::TrailingBytes), "trailing bytes should report issue");
}

void TestChecksumMismatchFailsDecode()
{
	std::vector<std::uint8_t> bytes = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload({ 1, 2, 3 }).bytes;
	bytes.back() ^= 0xFFU;

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(bytes);

	Expect(!decoded.decoded, "modified payload should fail checksum");
	Expect(HasIssue(decoded, iggy::runtime::RuntimeSaveFileEnvelopeIssueCode::ChecksumMismatch), "modified payload should report checksum mismatch");
	Expect(decoded.envelope.payload == std::vector<std::uint8_t>({ 1, 2, static_cast<std::uint8_t>(3 ^ 0xFFU) }), "checksum mismatch should preserve decoded payload for diagnostics");
	if (!decoded.issues.empty()) {
		Expect(decoded.issues[0].expected != decoded.issues[0].actual, "checksum mismatch should report expected and actual checksums");
	}
}

void TestArbitraryPayloadBytesAreAccepted()
{
	const std::vector<std::uint8_t> payload { 0, 255, 127, 64, 0 };

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult decoded = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(
		iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload(payload).bytes);

	Expect(decoded.decoded, "arbitrary payload bytes should decode when checksum matches");
	Expect(decoded.envelope.payload == payload, "arbitrary payload bytes should be preserved exactly");
}

void TestArchiveCodecIntegrationRoundTrip()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = Archive();
	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult archiveBytes = iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);
	const iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult envelopeBytes = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload(archiveBytes.bytes);

	const iggy::runtime::RuntimeSaveFileEnvelopeDecodeResult envelope = iggy::runtime::RuntimeSaveFileEnvelopeDecoder {}.decode(envelopeBytes.bytes);
	const iggy::runtime::RuntimeSaveChunkArchiveDecodeResult decodedArchive = iggy::runtime::RuntimeSaveChunkArchiveDecoder {}.decode(envelope.envelope.payload);

	Expect(archiveBytes.encoded, "archive integration setup should encode archive bytes");
	Expect(envelopeBytes.encoded, "archive integration setup should encode envelope bytes");
	Expect(envelope.decoded, "archive integration should decode envelope");
	Expect(decodedArchive.decoded, "archive integration should decode archive payload");
	ExpectSameArchive(decodedArchive.archive, archive, "archive integration should preserve archive through envelope payload");
}

} // namespace

int main()
{
	TestCrc32StandardVector();
	TestEmptyPayloadEncodesAndDecodes();
	TestNonEmptyPayloadRoundTrips();
	TestExactEnvelopeLayout();
	TestInvalidMagicFailsDecode();
	TestUnsupportedEnvelopeVersionFailsDecode();
	TestUnsupportedChecksumAlgorithmFailsDecode();
	TestTruncatedHeaderFailsDecode();
	TestPayloadSizeLargerThanAvailableFailsDecode();
	TestTrailingBytesFailDecode();
	TestChecksumMismatchFailsDecode();
	TestArbitraryPayloadBytesAreAccepted();
	TestArchiveCodecIntegrationRoundTrip();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
