#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"

#include "runtime/RuntimeBinaryCodec.hpp"

namespace iggy::runtime {
namespace {

std::vector<std::uint8_t> bytesFor(const std::array<char, 4> &value)
{
	return {
		static_cast<std::uint8_t>(value[0]),
		static_cast<std::uint8_t>(value[1]),
		static_cast<std::uint8_t>(value[2]),
		static_cast<std::uint8_t>(value[3]),
	};
}

std::vector<std::uint8_t> bytesFor(RuntimeSaveChunkId id)
{
	return bytesFor(id.value);
}

std::array<char, 4> magicFromBytes(const std::vector<std::uint8_t> &bytes)
{
	return {
		static_cast<char>(bytes[0]),
		static_cast<char>(bytes[1]),
		static_cast<char>(bytes[2]),
		static_cast<char>(bytes[3]),
	};
}

RuntimeSaveChunkId chunkIdFromBytes(const std::vector<std::uint8_t> &bytes)
{
	return { {
		static_cast<char>(bytes[0]),
		static_cast<char>(bytes[1]),
		static_cast<char>(bytes[2]),
		static_cast<char>(bytes[3]),
	} };
}

void addIssue(
	RuntimeSaveChunkArchiveDecodeResult &result,
	RuntimeSaveChunkArchiveCodecIssueCode code,
	std::size_t offset,
	std::size_t chunkIndex = 0)
{
	result.issues.push_back({ code, offset, chunkIndex });
}

} // namespace

RuntimeSaveChunkArchiveEncodeResult RuntimeSaveChunkArchiveEncoder::encode(const RuntimeSaveChunkArchive &archive) const
{
	RuntimeSaveChunkArchiveEncodeResult result;
	result.validation = RuntimeSaveChunkArchiveValidator {}.validate(archive);
	if (!result.validation.valid)
		return result;

	RuntimeBinaryWriter writer;
	writer.writeBytes(bytesFor(archive.magic));
	writer.writeU32LE(archive.version);
	writer.writeU32LE(static_cast<std::uint32_t>(archive.chunks.size()));
	for (const RuntimeSaveChunk &chunk : archive.chunks) {
		writer.writeBytes(bytesFor(chunk.id));
		writer.writeU32LE(chunk.version);
		writer.writeU32LE(static_cast<std::uint32_t>(chunk.payload.size()));
		writer.writeBytes(chunk.payload);
	}

	result.encoded = true;
	result.bytes = writer.takeBytes();
	return result;
}

RuntimeSaveChunkArchiveDecodeResult RuntimeSaveChunkArchiveDecoder::decode(const std::vector<std::uint8_t> &bytes) const
{
	RuntimeSaveChunkArchiveDecodeResult result;
	RuntimeBinaryReader reader(bytes);

	std::vector<std::uint8_t> rawMagic;
	if (reader.readBytes(4, rawMagic) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::TruncatedInput, reader.offset());
		return result;
	}
	result.archive.magic = magicFromBytes(rawMagic);

	if (reader.readU32LE(result.archive.version) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::TruncatedInput, reader.offset());
		return result;
	}

	std::uint32_t chunkCount = 0;
	if (reader.readU32LE(chunkCount) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::TruncatedInput, reader.offset());
		return result;
	}

	for (std::uint32_t index = 0; index < chunkCount; ++index) {
		const std::size_t chunkHeaderOffset = reader.offset();
		std::vector<std::uint8_t> rawChunkId;
		if (reader.readBytes(4, rawChunkId) != RuntimeBinaryReadStatus::Ok) {
			addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::TruncatedInput, chunkHeaderOffset, index);
			return result;
		}

		RuntimeSaveChunk chunk;
		chunk.id = chunkIdFromBytes(rawChunkId);
		if (reader.readU32LE(chunk.version) != RuntimeBinaryReadStatus::Ok) {
			addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::TruncatedInput, chunkHeaderOffset, index);
			return result;
		}

		std::uint32_t payloadSize = 0;
		if (reader.readU32LE(payloadSize) != RuntimeBinaryReadStatus::Ok) {
			addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::TruncatedInput, chunkHeaderOffset, index);
			return result;
		}

		if (reader.readBytes(payloadSize, chunk.payload) != RuntimeBinaryReadStatus::Ok) {
			addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::InvalidPayloadSize, reader.offset(), index);
			return result;
		}

		result.archive.chunks.push_back(chunk);
	}

	if (reader.remaining() != 0) {
		addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::TrailingBytes, reader.offset());
		return result;
	}

	result.validation = RuntimeSaveChunkArchiveValidator {}.validate(result.archive);
	if (!result.validation.valid) {
		addIssue(result, RuntimeSaveChunkArchiveCodecIssueCode::InvalidArchive, 0);
		return result;
	}

	result.decoded = true;
	return result;
}

} // namespace iggy::runtime
