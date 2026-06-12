#include <cstdlib>
#include <string>
#include <vector>

#include "runtime/RuntimeSaveMetadata.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::runtime::RuntimeSaveMetadata Metadata(std::string displayName = "Slot 1", std::uint64_t createdTick = 42)
{
	return { displayName, createdTick };
}

iggy::runtime::RuntimeSaveChunk Chunk(std::vector<std::uint8_t> payload)
{
	return { iggy::runtime::runtimeSaveMetadataChunkId(), iggy::runtime::RuntimeSaveMetadataChunkVersion, payload };
}

bool HasValidationIssue(
	const iggy::runtime::RuntimeSaveMetadataValidationResult &validation,
	iggy::runtime::RuntimeSaveMetadataIssueCode code)
{
	for (const iggy::runtime::RuntimeSaveMetadataIssue &issue : validation.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

bool HasChunkIssue(
	const iggy::runtime::RuntimeSaveMetadataDecodeResult &decode,
	iggy::runtime::RuntimeSaveMetadataChunkIssueCode code)
{
	for (const iggy::runtime::RuntimeSaveMetadataChunkIssue &issue : decode.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void ExpectBytes(const std::vector<std::uint8_t> &actual, const std::vector<std::uint8_t> &expected, const char *message)
{
	Expect(actual == expected, message);
}

void TestValidMetadataValidates()
{
	const iggy::runtime::RuntimeSaveMetadataValidationResult validation = iggy::runtime::RuntimeSaveMetadataValidator {}.validate(Metadata());

	Expect(validation.valid, "valid metadata should validate");
	Expect(validation.issues.empty(), "valid metadata should have no validation issues");
}

void TestEmptyDisplayNameInvalid()
{
	const iggy::runtime::RuntimeSaveMetadataValidationResult validation = iggy::runtime::RuntimeSaveMetadataValidator {}.validate(Metadata("", 0));

	Expect(!validation.valid, "empty display name should be invalid");
	Expect(HasValidationIssue(validation, iggy::runtime::RuntimeSaveMetadataIssueCode::EmptyDisplayName), "empty display name should report issue");
}

void TestTooLongDisplayNameInvalid()
{
	const std::string longName(iggy::runtime::RuntimeSaveMetadataMaxDisplayNameBytes + 1, 'a');

	const iggy::runtime::RuntimeSaveMetadataValidationResult validation = iggy::runtime::RuntimeSaveMetadataValidator {}.validate(Metadata(longName, 0));

	Expect(!validation.valid, "too-long display name should be invalid");
	Expect(HasValidationIssue(validation, iggy::runtime::RuntimeSaveMetadataIssueCode::DisplayNameTooLong), "too-long display name should report issue");
}

void TestCreatedTickZeroAccepted()
{
	const iggy::runtime::RuntimeSaveMetadataValidationResult validation = iggy::runtime::RuntimeSaveMetadataValidator {}.validate(Metadata("Start", 0));

	Expect(validation.valid, "created tick zero should be accepted");
}

void TestEncodingProducesMetaV1WithDeterministicPayload()
{
	const iggy::runtime::RuntimeSaveChunk chunk = iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode(Metadata("Save", 0x0102030405060708ULL));

	Expect(chunk.id == iggy::runtime::runtimeSaveMetadataChunkId(), "metadata encoder should use META chunk id");
	Expect(chunk.version == iggy::runtime::RuntimeSaveMetadataChunkVersion, "metadata encoder should use META v1");
	ExpectBytes(
		chunk.payload,
		{
			0x04, 0x00, 0x00, 0x00,
			'S', 'a', 'v', 'e',
			0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
		},
		"metadata encoder should write deterministic little-endian payload");
}

void TestDecodeRoundTrip()
{
	const iggy::runtime::RuntimeSaveMetadata metadata = Metadata("Manual_A-1", 99);
	const iggy::runtime::RuntimeSaveChunk chunk = iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode(metadata);

	const iggy::runtime::RuntimeSaveMetadataDecodeResult decode = iggy::runtime::RuntimeSaveMetadataChunkDecoder {}.decode(chunk);

	Expect(decode.decoded, "valid metadata chunk should decode");
	Expect(decode.metadata.displayName == metadata.displayName, "metadata decode should preserve display name");
	Expect(decode.metadata.createdTick == metadata.createdTick, "metadata decode should preserve created tick");
	Expect(decode.validation.valid, "metadata decode should preserve validation result");
}

void TestUnsupportedChunkVersionFailsDecode()
{
	iggy::runtime::RuntimeSaveChunk chunk = iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode(Metadata());
	chunk.version = 2;

	const iggy::runtime::RuntimeSaveMetadataDecodeResult decode = iggy::runtime::RuntimeSaveMetadataChunkDecoder {}.decode(chunk);

	Expect(!decode.decoded, "unsupported metadata chunk version should fail decode");
	Expect(HasChunkIssue(decode, iggy::runtime::RuntimeSaveMetadataChunkIssueCode::UnsupportedChunkVersion), "unsupported metadata chunk version should report issue");
}

void TestWrongChunkIdFailsDecode()
{
	iggy::runtime::RuntimeSaveChunk chunk = iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode(Metadata());
	chunk.id = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'S');

	const iggy::runtime::RuntimeSaveMetadataDecodeResult decode = iggy::runtime::RuntimeSaveMetadataChunkDecoder {}.decode(chunk);

	Expect(!decode.decoded, "wrong metadata chunk id should fail decode");
	Expect(HasChunkIssue(decode, iggy::runtime::RuntimeSaveMetadataChunkIssueCode::UnexpectedChunkId), "wrong metadata chunk id should report issue");
}

void TestTruncatedPayloadsFailDecode()
{
	const std::vector<std::vector<std::uint8_t>> payloads {
		{},
		{ 0x04, 0x00, 0x00 },
		{ 0x04, 0x00, 0x00, 0x00, 'S', 'a' },
		{ 0x04, 0x00, 0x00, 0x00, 'S', 'a', 'v', 'e', 0x01 },
	};

	for (const std::vector<std::uint8_t> &payload : payloads) {
		const iggy::runtime::RuntimeSaveMetadataDecodeResult decode = iggy::runtime::RuntimeSaveMetadataChunkDecoder {}.decode(Chunk(payload));
		Expect(!decode.decoded, "truncated metadata payload should fail decode");
		Expect(HasChunkIssue(decode, iggy::runtime::RuntimeSaveMetadataChunkIssueCode::MalformedPayload), "truncated metadata payload should report malformed issue");
	}
}

void TestTrailingPayloadBytesFailDecode()
{
	iggy::runtime::RuntimeSaveChunk chunk = iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode(Metadata());
	chunk.payload.push_back(0xFF);

	const iggy::runtime::RuntimeSaveMetadataDecodeResult decode = iggy::runtime::RuntimeSaveMetadataChunkDecoder {}.decode(chunk);

	Expect(!decode.decoded, "metadata chunk with trailing bytes should fail decode");
	Expect(HasChunkIssue(decode, iggy::runtime::RuntimeSaveMetadataChunkIssueCode::MalformedPayload), "metadata chunk with trailing bytes should report malformed issue");
}

void TestDecodedInvalidMetadataFailsWithValidationDiagnostics()
{
	const iggy::runtime::RuntimeSaveChunk chunk = iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode(Metadata("", 10));

	const iggy::runtime::RuntimeSaveMetadataDecodeResult decode = iggy::runtime::RuntimeSaveMetadataChunkDecoder {}.decode(chunk);

	Expect(!decode.decoded, "decoded invalid metadata should fail decode");
	Expect(HasChunkIssue(decode, iggy::runtime::RuntimeSaveMetadataChunkIssueCode::MetadataInvalid), "decoded invalid metadata should report metadata invalid issue");
	Expect(!decode.validation.valid, "decoded invalid metadata should preserve validation result");
	Expect(HasValidationIssue(decode.validation, iggy::runtime::RuntimeSaveMetadataIssueCode::EmptyDisplayName), "decoded invalid metadata should preserve validation diagnostics");
}

void TestEncoderEncodesInvalidMetadataExactly()
{
	const iggy::runtime::RuntimeSaveChunk chunk = iggy::runtime::RuntimeSaveMetadataChunkEncoder {}.encode(Metadata("", 5));

	Expect(chunk.id == iggy::runtime::runtimeSaveMetadataChunkId(), "metadata encoder should still use META id for invalid metadata");
	Expect(chunk.version == iggy::runtime::RuntimeSaveMetadataChunkVersion, "metadata encoder should still use v1 for invalid metadata");
	ExpectBytes(
		chunk.payload,
		{
			0x00, 0x00, 0x00, 0x00,
			0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		"metadata encoder should encode exact invalid metadata bytes");
}

} // namespace

int main()
{
	TestValidMetadataValidates();
	TestEmptyDisplayNameInvalid();
	TestTooLongDisplayNameInvalid();
	TestCreatedTickZeroAccepted();
	TestEncodingProducesMetaV1WithDeterministicPayload();
	TestDecodeRoundTrip();
	TestUnsupportedChunkVersionFailsDecode();
	TestWrongChunkIdFailsDecode();
	TestTruncatedPayloadsFailDecode();
	TestTrailingPayloadBytesFailDecode();
	TestDecodedInvalidMetadataFailsWithValidationDiagnostics();
	TestEncoderEncodesInvalidMetadataExactly();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
