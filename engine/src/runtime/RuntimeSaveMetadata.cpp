#include "runtime/RuntimeSaveMetadata.hpp"

#include "runtime/RuntimeBinaryCodec.hpp"

namespace iggy::runtime {
namespace {

void writeString(RuntimeBinaryWriter &writer, const std::string &value)
{
	writer.writeU32LE(static_cast<std::uint32_t>(value.size()));
	writer.writeBytes(std::vector<std::uint8_t>(value.begin(), value.end()));
}

bool readString(RuntimeBinaryReader &reader, std::string &value)
{
	std::uint32_t length = 0;
	if (reader.readU32LE(length) != RuntimeBinaryReadStatus::Ok)
		return false;

	std::vector<std::uint8_t> bytes;
	if (reader.readBytes(length, bytes) != RuntimeBinaryReadStatus::Ok)
		return false;

	value.assign(bytes.begin(), bytes.end());
	return true;
}

void addIssue(RuntimeSaveMetadataDecodeResult &result, RuntimeSaveMetadataChunkIssueCode code)
{
	result.issues.push_back({ code });
}

} // namespace

RuntimeSaveMetadataValidationResult RuntimeSaveMetadataValidator::validate(const RuntimeSaveMetadata &metadata) const
{
	RuntimeSaveMetadataValidationResult result;
	if (metadata.displayName.empty())
		result.issues.push_back({ RuntimeSaveMetadataIssueCode::EmptyDisplayName });
	if (metadata.displayName.size() > RuntimeSaveMetadataMaxDisplayNameBytes)
		result.issues.push_back({ RuntimeSaveMetadataIssueCode::DisplayNameTooLong });

	result.valid = result.issues.empty();
	return result;
}

RuntimeSaveChunkId runtimeSaveMetadataChunkId()
{
	return makeRuntimeSaveChunkId('M', 'E', 'T', 'A');
}

RuntimeSaveChunk RuntimeSaveMetadataChunkEncoder::encode(const RuntimeSaveMetadata &metadata) const
{
	RuntimeBinaryWriter writer;
	writeString(writer, metadata.displayName);
	writer.writeU64LE(metadata.createdTick);
	return { runtimeSaveMetadataChunkId(), RuntimeSaveMetadataChunkVersion, writer.takeBytes() };
}

RuntimeSaveMetadataDecodeResult RuntimeSaveMetadataChunkDecoder::decode(const RuntimeSaveChunk &chunk) const
{
	RuntimeSaveMetadataDecodeResult result;
	if (chunk.id != runtimeSaveMetadataChunkId()) {
		addIssue(result, RuntimeSaveMetadataChunkIssueCode::UnexpectedChunkId);
		return result;
	}

	if (chunk.version != RuntimeSaveMetadataChunkVersion) {
		addIssue(result, RuntimeSaveMetadataChunkIssueCode::UnsupportedChunkVersion);
		return result;
	}

	RuntimeBinaryReader reader(chunk.payload);
	RuntimeSaveMetadata decoded;
	if (!readString(reader, decoded.displayName)) {
		addIssue(result, RuntimeSaveMetadataChunkIssueCode::MalformedPayload);
		return result;
	}
	if (reader.readU64LE(decoded.createdTick) != RuntimeBinaryReadStatus::Ok) {
		addIssue(result, RuntimeSaveMetadataChunkIssueCode::MalformedPayload);
		return result;
	}
	if (reader.remaining() != 0) {
		addIssue(result, RuntimeSaveMetadataChunkIssueCode::MalformedPayload);
		return result;
	}

	result.metadata = decoded;
	result.validation = RuntimeSaveMetadataValidator {}.validate(decoded);
	if (!result.validation.valid) {
		addIssue(result, RuntimeSaveMetadataChunkIssueCode::MetadataInvalid);
		return result;
	}

	result.decoded = true;
	return result;
}

} // namespace iggy::runtime
