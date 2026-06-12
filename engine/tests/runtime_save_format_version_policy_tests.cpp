#include <cstdlib>

#include "runtime/RuntimeSaveFormatVersionPolicy.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::runtime::RuntimeSaveChunkId SessionChunkId = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'S');
const iggy::runtime::RuntimeSaveChunkId LevelMapChunkId = iggy::runtime::makeRuntimeSaveChunkId('L', 'M', 'A', 'P');
const iggy::runtime::RuntimeSaveChunkId PlayerChunkId = iggy::runtime::makeRuntimeSaveChunkId('P', 'L', 'Y', 'R');
const iggy::runtime::RuntimeSaveChunkId NpcsChunkId = iggy::runtime::makeRuntimeSaveChunkId('N', 'P', 'C', 'S');
const iggy::runtime::RuntimeSaveChunkId UnknownChunkId = iggy::runtime::makeRuntimeSaveChunkId('T', 'E', 'S', 'T');

iggy::runtime::RuntimeSaveChunk Chunk(iggy::runtime::RuntimeSaveChunkId id, std::uint32_t version = 1)
{
	return { id, version, {} };
}

iggy::runtime::RuntimeSaveChunkArchive Archive(std::vector<iggy::runtime::RuntimeSaveChunk> chunks = {})
{
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.chunks = chunks;
	return archive;
}

void TestEnvelopeAndArchiveVersionOneSupported()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;

	Expect(policy.supportsEnvelopeVersion(1), "version policy should support envelope version 1");
	Expect(policy.supportsArchiveVersion(1), "version policy should support archive version 1");
}

void TestEnvelopeAndArchiveVersionZeroAndFutureUnsupported()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;

	Expect(!policy.supportsEnvelopeVersion(0), "version policy should reject envelope version 0");
	Expect(!policy.supportsEnvelopeVersion(2), "version policy should reject future envelope version");
	Expect(!policy.supportsArchiveVersion(0), "version policy should reject archive version 0");
	Expect(!policy.supportsArchiveVersion(2), "version policy should reject future archive version");
}

void TestKnownSnapshotChunkVersionOneSupported()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;

	Expect(policy.supportsSnapshotChunkVersion(SessionChunkId, 1), "version policy should support SESS v1");
	Expect(policy.supportsSnapshotChunkVersion(LevelMapChunkId, 1), "version policy should support LMAP v1");
	Expect(policy.supportsSnapshotChunkVersion(PlayerChunkId, 1), "version policy should support PLYR v1");
	Expect(policy.supportsSnapshotChunkVersion(NpcsChunkId, 1), "version policy should support NPCS v1");
}

void TestKnownSnapshotChunkVersionZeroAndFutureUnsupported()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;

	Expect(!policy.supportsSnapshotChunkVersion(SessionChunkId, 0), "version policy should reject SESS v0");
	Expect(!policy.supportsSnapshotChunkVersion(SessionChunkId, 2), "version policy should reject future SESS version");
	Expect(!policy.supportsSnapshotChunkVersion(LevelMapChunkId, 0), "version policy should reject LMAP v0");
	Expect(!policy.supportsSnapshotChunkVersion(PlayerChunkId, 2), "version policy should reject future PLYR version");
	Expect(!policy.supportsSnapshotChunkVersion(NpcsChunkId, 2), "version policy should reject future NPCS version");
}

void TestUnknownChunkDoesNotSupportDirectVersionCheck()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;

	Expect(!policy.supportsSnapshotChunkVersion(UnknownChunkId, 1), "version policy direct snapshot chunk check should reject unknown chunk ids");
}

void TestUnknownChunkDoesNotFailReport()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;
	const iggy::runtime::RuntimeSaveFileEnvelope envelope;
	const iggy::runtime::RuntimeSaveChunkArchive archive = Archive({ Chunk(SessionChunkId), Chunk(UnknownChunkId, 9), Chunk(LevelMapChunkId) });

	const iggy::runtime::RuntimeSaveFormatVersionReport report = policy.inspect(envelope, archive);

	Expect(report.supported, "unknown chunk should not make version report unsupported");
	Expect(report.issues.size() == 1, "unknown chunk should be reported diagnostically");
	if (report.issues.size() == 1) {
		Expect(report.issues[0].status == iggy::runtime::RuntimeSaveFormatVersionStatus::UnknownChunk, "unknown chunk issue should use UnknownChunk status");
		Expect(report.issues[0].chunkId == UnknownChunkId, "unknown chunk issue should preserve chunk id");
		Expect(report.issues[0].version == 9, "unknown chunk issue should preserve chunk version");
		Expect(report.issues[0].chunkIndex == 1, "unknown chunk issue should preserve chunk index");
	}
}

void TestUnsupportedKnownChunkVersionFailsReport()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;
	const iggy::runtime::RuntimeSaveFileEnvelope envelope;
	const iggy::runtime::RuntimeSaveChunkArchive archive = Archive({ Chunk(SessionChunkId), Chunk(LevelMapChunkId, 2), Chunk(UnknownChunkId, 7) });

	const iggy::runtime::RuntimeSaveFormatVersionReport report = policy.inspect(envelope, archive);

	Expect(!report.supported, "unsupported known chunk version should make report unsupported");
	Expect(report.issues.size() == 2, "version report should preserve unsupported and unknown chunk diagnostics");
	if (report.issues.size() >= 1) {
		Expect(report.issues[0].status == iggy::runtime::RuntimeSaveFormatVersionStatus::UnsupportedChunkVersion, "unsupported chunk issue should be first in archive order");
		Expect(report.issues[0].chunkId == LevelMapChunkId, "unsupported chunk issue should preserve chunk id");
		Expect(report.issues[0].version == 2, "unsupported chunk issue should preserve chunk version");
		Expect(report.issues[0].chunkIndex == 1, "unsupported chunk issue should preserve chunk index");
	}
	if (report.issues.size() >= 2)
		Expect(report.issues[1].status == iggy::runtime::RuntimeSaveFormatVersionStatus::UnknownChunk, "unknown chunk diagnostic should still be reported after unsupported known chunks");
}

void TestUnsupportedEnvelopeAndArchiveVersionsFailReport()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;
	iggy::runtime::RuntimeSaveFileEnvelope envelope;
	envelope.version = 0;
	iggy::runtime::RuntimeSaveChunkArchive archive = Archive();
	archive.version = 2;

	const iggy::runtime::RuntimeSaveFormatVersionReport report = policy.inspect(envelope, archive);

	Expect(!report.supported, "unsupported envelope/archive versions should make report unsupported");
	Expect(report.issues.size() == 2, "version report should include envelope and archive issues");
	if (report.issues.size() >= 1) {
		Expect(report.issues[0].status == iggy::runtime::RuntimeSaveFormatVersionStatus::UnsupportedEnvelopeVersion, "first version issue should be envelope");
		Expect(report.issues[0].version == 0, "envelope issue should preserve version");
	}
	if (report.issues.size() >= 2) {
		Expect(report.issues[1].status == iggy::runtime::RuntimeSaveFormatVersionStatus::UnsupportedArchiveVersion, "second version issue should be archive");
		Expect(report.issues[1].version == 2, "archive issue should preserve version");
	}
}

void TestInspectDoesNotMutateInputs()
{
	const iggy::runtime::RuntimeSaveFormatVersionPolicy policy;
	iggy::runtime::RuntimeSaveFileEnvelope envelope;
	envelope.version = 1;
	envelope.payload = { 1, 2, 3 };
	const iggy::runtime::RuntimeSaveFileEnvelope envelopeBefore = envelope;
	iggy::runtime::RuntimeSaveChunkArchive archive = Archive({ Chunk(SessionChunkId), Chunk(UnknownChunkId, 2) });
	const iggy::runtime::RuntimeSaveChunkArchive archiveBefore = archive;

	const iggy::runtime::RuntimeSaveFormatVersionReport report = policy.inspect(envelope, archive);

	Expect(report.supported, "version policy immutability setup should inspect successfully");
	Expect(envelope.version == envelopeBefore.version, "version policy should not mutate envelope version");
	Expect(envelope.payload == envelopeBefore.payload, "version policy should not mutate envelope payload");
	Expect(archive.version == archiveBefore.version, "version policy should not mutate archive version");
	Expect(archive.chunks.size() == archiveBefore.chunks.size(), "version policy should not mutate archive chunks");
	for (std::size_t index = 0; index < archive.chunks.size() && index < archiveBefore.chunks.size(); ++index) {
		Expect(archive.chunks[index].id == archiveBefore.chunks[index].id, "version policy should not mutate chunk id");
		Expect(archive.chunks[index].version == archiveBefore.chunks[index].version, "version policy should not mutate chunk version");
	}
}

} // namespace

int main()
{
	TestEnvelopeAndArchiveVersionOneSupported();
	TestEnvelopeAndArchiveVersionZeroAndFutureUnsupported();
	TestKnownSnapshotChunkVersionOneSupported();
	TestKnownSnapshotChunkVersionZeroAndFutureUnsupported();
	TestUnknownChunkDoesNotSupportDirectVersionCheck();
	TestUnknownChunkDoesNotFailReport();
	TestUnsupportedKnownChunkVersionFailsReport();
	TestUnsupportedEnvelopeAndArchiveVersionsFailReport();
	TestInspectDoesNotMutateInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
