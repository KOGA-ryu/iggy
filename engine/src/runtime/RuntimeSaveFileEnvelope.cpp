#include "runtime/RuntimeSaveFileEnvelope.hpp"

#include <array>

#include "runtime/RuntimeBinaryCodec.hpp"

namespace iggy::runtime {
namespace {

constexpr std::array<std::uint8_t, 4> SaveFileMagic {
	static_cast<std::uint8_t>('I'),
	static_cast<std::uint8_t>('G'),
	static_cast<std::uint8_t>('S'),
	static_cast<std::uint8_t>('F'),
};
constexpr std::uint32_t CurrentEnvelopeVersion = 1;
constexpr std::uint32_t Crc32AlgorithmId = static_cast<std::uint32_t>(RuntimeSaveFileChecksumAlgorithm::Crc32);

void addIssue(
	RuntimeSaveFileEnvelopeDecodeResult &result,
	RuntimeSaveFileEnvelopeIssueCode code,
	std::size_t offset,
	std::uint32_t expected = 0,
	std::uint32_t actual = 0)
{
	result.issues.push_back({ code, offset, expected, actual });
}

bool sameMagic(const std::vector<std::uint8_t> &rawMagic)
{
	return rawMagic.size() == SaveFileMagic.size()
		&& rawMagic[0] == SaveFileMagic[0]
		&& rawMagic[1] == SaveFileMagic[1]
		&& rawMagic[2] == SaveFileMagic[2]
		&& rawMagic[3] == SaveFileMagic[3];
}

} // namespace

std::uint32_t computeRuntimeSaveCrc32(const std::vector<std::uint8_t> &bytes)
{
	std::uint32_t crc = 0xFFFFFFFFU;
	for (std::uint8_t byte : bytes) {
		crc ^= byte;
		for (int bit = 0; bit < 8; ++bit) {
			const std::uint32_t mask = 0U - (crc & 1U);
			crc = (crc >> 1U) ^ (0xEDB88320U & mask);
		}
	}
	return ~crc;
}

RuntimeSaveFileEnvelopeEncodeResult RuntimeSaveFileEnvelopeEncoder::encodePayload(const std::vector<std::uint8_t> &payload) const
{
	RuntimeSaveFileEnvelopeEncodeResult result;
	result.envelope.version = CurrentEnvelopeVersion;
	result.envelope.checksumAlgorithm = RuntimeSaveFileChecksumAlgorithm::Crc32;
	result.envelope.payload = payload;
	result.envelope.payloadChecksum = computeRuntimeSaveCrc32(payload);

	RuntimeBinaryWriter writer;
	writer.writeBytes({ SaveFileMagic[0], SaveFileMagic[1], SaveFileMagic[2], SaveFileMagic[3] });
	writer.writeU32LE(result.envelope.version);
	writer.writeU32LE(static_cast<std::uint32_t>(result.envelope.checksumAlgorithm));
	writer.writeU32LE(static_cast<std::uint32_t>(payload.size()));
	writer.writeU32LE(result.envelope.payloadChecksum);
	writer.writeBytes(payload);

	result.encoded = true;
	result.bytes = writer.takeBytes();
	return result;
}

RuntimeSaveFileEnvelopeDecodeResult RuntimeSaveFileEnvelopeDecoder::decode(const std::vector<std::uint8_t> &bytes) const
{
	RuntimeSaveFileEnvelopeDecodeResult result;
	RuntimeBinaryReader reader(bytes);

	std::vector<std::uint8_t> rawMagic;
	if (reader.readBytes(4, rawMagic) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::TruncatedInput, reader.offset());
		return result;
	}
	if (!sameMagic(rawMagic)) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::InvalidMagic, 0);
		return result;
	}

	if (reader.readU32LE(result.envelope.version) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::TruncatedInput, reader.offset());
		return result;
	}
	if (result.envelope.version != CurrentEnvelopeVersion) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::UnsupportedEnvelopeVersion, reader.offset() - 4, CurrentEnvelopeVersion, result.envelope.version);
		return result;
	}

	std::uint32_t checksumAlgorithm = 0;
	if (reader.readU32LE(checksumAlgorithm) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::TruncatedInput, reader.offset());
		return result;
	}
	if (checksumAlgorithm != Crc32AlgorithmId) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::UnsupportedChecksumAlgorithm, reader.offset() - 4, Crc32AlgorithmId, checksumAlgorithm);
		return result;
	}
	result.envelope.checksumAlgorithm = RuntimeSaveFileChecksumAlgorithm::Crc32;

	std::uint32_t payloadSize = 0;
	if (reader.readU32LE(payloadSize) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::TruncatedInput, reader.offset());
		return result;
	}
	if (reader.readU32LE(result.envelope.payloadChecksum) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::TruncatedInput, reader.offset());
		return result;
	}

	if (reader.readBytes(payloadSize, result.envelope.payload) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::InvalidPayloadSize, reader.offset());
		return result;
	}

	if (reader.remaining() != 0) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::TrailingBytes, reader.offset());
		return result;
	}

	const std::uint32_t actualChecksum = computeRuntimeSaveCrc32(result.envelope.payload);
	if (actualChecksum != result.envelope.payloadChecksum) {
		addIssue(result, RuntimeSaveFileEnvelopeIssueCode::ChecksumMismatch, 16, result.envelope.payloadChecksum, actualChecksum);
		return result;
	}

	result.decoded = true;
	return result;
}

} // namespace iggy::runtime
