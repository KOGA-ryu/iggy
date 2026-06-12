#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace iggy::runtime {

enum class RuntimeSaveFileChecksumAlgorithm : std::uint32_t {
	Crc32 = 1,
};

struct RuntimeSaveFileEnvelope {
	std::uint32_t version = 1;
	RuntimeSaveFileChecksumAlgorithm checksumAlgorithm = RuntimeSaveFileChecksumAlgorithm::Crc32;
	std::uint32_t payloadChecksum = 0;
	std::vector<std::uint8_t> payload;
};

enum class RuntimeSaveFileEnvelopeIssueCode {
	InvalidMagic,
	UnsupportedEnvelopeVersion,
	UnsupportedChecksumAlgorithm,
	TruncatedInput,
	InvalidPayloadSize,
	ChecksumMismatch,
	TrailingBytes,
};

struct RuntimeSaveFileEnvelopeIssue {
	RuntimeSaveFileEnvelopeIssueCode code = RuntimeSaveFileEnvelopeIssueCode::InvalidMagic;
	std::size_t offset = 0;
	std::uint32_t expected = 0;
	std::uint32_t actual = 0;
};

struct RuntimeSaveFileEnvelopeEncodeResult {
	bool encoded = false;
	std::vector<std::uint8_t> bytes;
	RuntimeSaveFileEnvelope envelope;
};

struct RuntimeSaveFileEnvelopeDecodeResult {
	bool decoded = false;
	RuntimeSaveFileEnvelope envelope;
	std::vector<RuntimeSaveFileEnvelopeIssue> issues;
};

[[nodiscard]] std::uint32_t computeRuntimeSaveCrc32(const std::vector<std::uint8_t> &bytes);

class RuntimeSaveFileEnvelopeEncoder {
public:
	[[nodiscard]] RuntimeSaveFileEnvelopeEncodeResult encodePayload(const std::vector<std::uint8_t> &payload) const;
};

class RuntimeSaveFileEnvelopeDecoder {
public:
	[[nodiscard]] RuntimeSaveFileEnvelopeDecodeResult decode(const std::vector<std::uint8_t> &bytes) const;
};

} // namespace iggy::runtime
