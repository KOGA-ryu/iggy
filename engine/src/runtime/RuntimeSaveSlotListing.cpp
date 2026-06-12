#include "runtime/RuntimeSaveSlotListing.hpp"

#include <algorithm>

#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"

namespace iggy::runtime {
namespace {

struct SlotDirectory {
	RuntimeSaveSlotKind kind = RuntimeSaveSlotKind::Manual;
	const char *name = "manual";
};

constexpr SlotDirectory SlotDirectories[] {
	{ RuntimeSaveSlotKind::Manual, "manual" },
	{ RuntimeSaveSlotKind::Auto, "auto" },
	{ RuntimeSaveSlotKind::Quick, "quick" },
};

void addIssue(
	RuntimeSaveSlotListingResult &result,
	RuntimeSaveSlotListingIssueCode code,
	RuntimeSaveSlotId slot,
	std::filesystem::path path)
{
	result.issues.push_back({ code, slot, std::move(path) });
}

[[nodiscard]] bool hasListedExtension(const std::filesystem::path &path, const std::string &extension)
{
	if (extension.empty())
		return path.extension().empty();
	return path.extension() == extension;
}

[[nodiscard]] std::vector<std::filesystem::path> sortedRegularFiles(const std::filesystem::path &directory)
{
	std::vector<std::filesystem::path> files;
	std::error_code ignored;
	for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(directory, ignored)) {
		if (entry.is_regular_file(ignored))
			files.push_back(entry.path());
	}

	std::sort(files.begin(), files.end(), [](const std::filesystem::path &left, const std::filesystem::path &right) {
		return left.filename().string() < right.filename().string();
	});
	return files;
}

[[nodiscard]] bool inspectMetadata(RuntimeSaveSlotListingResult &result, RuntimeSaveSlotListingEntry &entry)
{
	const RuntimeSaveFileReadResult read = RuntimeSaveFileReader {}.readBytes(entry.path);
	if (read.status != RuntimeSaveFileIOStatus::Ok) {
		addIssue(result, RuntimeSaveSlotListingIssueCode::FileReadFailed, entry.slot, entry.path);
		return false;
	}

	const RuntimeSaveFileEnvelopeDecodeResult envelope = RuntimeSaveFileEnvelopeDecoder {}.decode(read.bytes);
	if (!envelope.decoded) {
		addIssue(result, RuntimeSaveSlotListingIssueCode::EnvelopeDecodeFailed, entry.slot, entry.path);
		return false;
	}

	const RuntimeSaveChunkArchiveDecodeResult archive = RuntimeSaveChunkArchiveDecoder {}.decode(envelope.envelope.payload);
	if (!archive.decoded) {
		addIssue(result, RuntimeSaveSlotListingIssueCode::ArchiveDecodeFailed, entry.slot, entry.path);
		return false;
	}

	const std::vector<std::size_t> metadataIndexes = findChunkIndexes(archive.archive, runtimeSaveMetadataChunkId());
	if (metadataIndexes.empty())
		return true;

	if (metadataIndexes.size() > 1) {
		addIssue(result, RuntimeSaveSlotListingIssueCode::DuplicateMetadata, entry.slot, entry.path);
		return true;
	}

	const RuntimeSaveMetadataDecodeResult metadata = RuntimeSaveMetadataChunkDecoder {}.decode(archive.archive.chunks[metadataIndexes.front()]);
	if (!metadata.decoded) {
		addIssue(result, RuntimeSaveSlotListingIssueCode::MetadataDecodeFailed, entry.slot, entry.path);
		return true;
	}

	entry.hasMetadata = true;
	entry.metadata = metadata.metadata;
	return true;
}

} // namespace

RuntimeSaveSlotListingResult RuntimeSaveSlotListing::list(const RuntimeSaveSlotPathPolicyConfig &config) const
{
	RuntimeSaveSlotListingResult result;
	if (config.baseDirectory.empty()) {
		addIssue(result, RuntimeSaveSlotListingIssueCode::BaseDirectoryMissing, {}, {});
		return result;
	}

	result.listed = true;
	if (!std::filesystem::exists(config.baseDirectory)) {
		addIssue(result, RuntimeSaveSlotListingIssueCode::BaseDirectoryMissing, {}, config.baseDirectory);
		return result;
	}

	for (const SlotDirectory &slotDirectory : SlotDirectories) {
		const std::filesystem::path directory = config.baseDirectory / slotDirectory.name;
		if (!std::filesystem::exists(directory)) {
			addIssue(result, RuntimeSaveSlotListingIssueCode::SlotDirectoryMissing, { slotDirectory.kind, "" }, directory);
			continue;
		}

		for (const std::filesystem::path &path : sortedRegularFiles(directory)) {
			if (!hasListedExtension(path, config.extension))
				continue;

			RuntimeSaveSlotId slot { slotDirectory.kind, path.stem().string() };
			const RuntimeSaveSlotPathResult slotPath = RuntimeSaveSlotPathPolicy {}.pathFor(config, slot);
			if (slotPath.status != RuntimeSaveSlotPathStatus::Valid || slotPath.path != path) {
				addIssue(result, RuntimeSaveSlotListingIssueCode::InvalidSlotFileName, slot, path);
				continue;
			}

			RuntimeSaveSlotListingEntry entry;
			entry.slot = slot;
			entry.path = path;
			if (inspectMetadata(result, entry))
				result.entries.push_back(entry);
		}
	}

	return result;
}

} // namespace iggy::runtime
