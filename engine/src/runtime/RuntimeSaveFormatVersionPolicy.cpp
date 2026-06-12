#include "runtime/RuntimeSaveFormatVersionPolicy.hpp"

namespace iggy::runtime {
namespace {

constexpr std::uint32_t SupportedEnvelopeVersion = 1;
constexpr std::uint32_t SupportedArchiveVersion = 1;
constexpr std::uint32_t SupportedSnapshotChunkVersion = 1;

const RuntimeSaveChunkId SessionChunkId = makeRuntimeSaveChunkId('S', 'E', 'S', 'S');
const RuntimeSaveChunkId LevelMapChunkId = makeRuntimeSaveChunkId('L', 'M', 'A', 'P');
const RuntimeSaveChunkId PlayerChunkId = makeRuntimeSaveChunkId('P', 'L', 'Y', 'R');
const RuntimeSaveChunkId NpcsChunkId = makeRuntimeSaveChunkId('N', 'P', 'C', 'S');

[[nodiscard]] bool isKnownSnapshotChunkId(RuntimeSaveChunkId chunkId)
{
	return chunkId == SessionChunkId || chunkId == LevelMapChunkId || chunkId == PlayerChunkId || chunkId == NpcsChunkId;
}

void addIssue(
	RuntimeSaveFormatVersionReport &report,
	RuntimeSaveFormatVersionStatus status,
	RuntimeSaveChunkId chunkId = {},
	std::uint32_t version = 0,
	std::size_t chunkIndex = 0)
{
	report.issues.push_back({ status, chunkId, version, chunkIndex });
}

} // namespace

bool RuntimeSaveFormatVersionPolicy::supportsEnvelopeVersion(std::uint32_t version) const
{
	return version == SupportedEnvelopeVersion;
}

bool RuntimeSaveFormatVersionPolicy::supportsArchiveVersion(std::uint32_t version) const
{
	return version == SupportedArchiveVersion;
}

bool RuntimeSaveFormatVersionPolicy::supportsSnapshotChunkVersion(RuntimeSaveChunkId chunkId, std::uint32_t version) const
{
	return isKnownSnapshotChunkId(chunkId) && version == SupportedSnapshotChunkVersion;
}

RuntimeSaveFormatVersionReport RuntimeSaveFormatVersionPolicy::inspect(
	const RuntimeSaveFileEnvelope &envelope,
	const RuntimeSaveChunkArchive &archive) const
{
	RuntimeSaveFormatVersionReport report;
	report.supported = true;

	if (!supportsEnvelopeVersion(envelope.version)) {
		addIssue(report, RuntimeSaveFormatVersionStatus::UnsupportedEnvelopeVersion, {}, envelope.version);
		report.supported = false;
	}

	if (!supportsArchiveVersion(archive.version)) {
		addIssue(report, RuntimeSaveFormatVersionStatus::UnsupportedArchiveVersion, {}, archive.version);
		report.supported = false;
	}

	for (std::size_t index = 0; index < archive.chunks.size(); ++index) {
		const RuntimeSaveChunk &chunk = archive.chunks[index];
		if (!isKnownSnapshotChunkId(chunk.id)) {
			addIssue(report, RuntimeSaveFormatVersionStatus::UnknownChunk, chunk.id, chunk.version, index);
			continue;
		}

		if (!supportsSnapshotChunkVersion(chunk.id, chunk.version)) {
			addIssue(report, RuntimeSaveFormatVersionStatus::UnsupportedChunkVersion, chunk.id, chunk.version, index);
			report.supported = false;
		}
	}

	return report;
}

} // namespace iggy::runtime
