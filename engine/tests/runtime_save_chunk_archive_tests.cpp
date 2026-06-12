#include <array>
#include <cstdlib>
#include <vector>

#include "runtime/RuntimeSaveChunkArchive.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::runtime::RuntimeSaveChunkId SessionId = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'S');
const iggy::runtime::RuntimeSaveChunkId SessionAltId = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'a');
const iggy::runtime::RuntimeSaveChunkId LevelMapId = iggy::runtime::makeRuntimeSaveChunkId('L', 'M', 'A', 'P');
const iggy::runtime::RuntimeSaveChunkId PlayerId = iggy::runtime::makeRuntimeSaveChunkId('P', 'L', 'Y', 'R');

iggy::runtime::RuntimeSaveChunk Chunk(
	iggy::runtime::RuntimeSaveChunkId id,
	std::uint32_t version = 1,
	std::vector<std::uint8_t> payload = {})
{
	return { id, version, payload };
}

bool SamePayload(const std::vector<std::uint8_t> &actual, const std::vector<std::uint8_t> &expected)
{
	return actual == expected;
}

bool HasIssue(
	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult &result,
	iggy::runtime::RuntimeSaveChunkArchiveIssueCode code,
	std::size_t chunkIndex = 0,
	iggy::runtime::RuntimeSaveChunkId chunkId = {})
{
	for (const iggy::runtime::RuntimeSaveChunkArchiveIssue &issue : result.issues) {
		if (issue.code == code && issue.chunkIndex == chunkIndex && issue.chunkId == chunkId)
			return true;
	}
	return false;
}

void TestDefaultArchiveValidates()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive;

	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult result = iggy::runtime::RuntimeSaveChunkArchiveValidator {}.validate(archive);

	Expect(archive.magic == std::array<char, 4> { 'I', 'G', 'G', 'Y' }, "default archive should use IGGY magic");
	Expect(archive.version == 1, "default archive should use version 1");
	Expect(result.valid, "default empty archive should validate");
	Expect(result.issues.empty(), "default empty archive should have no issues");
}

void TestBuilderAppendsAndPreservesChunks()
{
	const std::vector<std::uint8_t> sessionPayload { 1, 2, 3 };
	const std::vector<std::uint8_t> levelPayload { 9, 8 };

	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSaveChunkArchiveBuilder {}
		.addChunk(Chunk(SessionId, 1, sessionPayload))
		.addChunk(Chunk(LevelMapId, 3, levelPayload))
		.build();

	Expect(archive.chunks.size() == 2, "builder should append two chunks");
	if (archive.chunks.size() == 2) {
		Expect(archive.chunks[0].id == SessionId, "builder should preserve first chunk id");
		Expect(archive.chunks[0].version == 1, "builder should preserve first chunk version");
		Expect(SamePayload(archive.chunks[0].payload, sessionPayload), "builder should preserve first payload");
		Expect(archive.chunks[1].id == LevelMapId, "builder should preserve second chunk id");
		Expect(archive.chunks[1].version == 3, "builder should preserve second chunk version");
		Expect(SamePayload(archive.chunks[1].payload, levelPayload), "builder should preserve second payload");
	}
}

void TestEmptyPayloadIsValid()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSaveChunkArchiveBuilder {}
		.addChunk(Chunk(SessionId))
		.build();

	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult result = iggy::runtime::RuntimeSaveChunkArchiveValidator {}.validate(archive);

	Expect(result.valid, "chunk with empty payload should validate");
	Expect(result.issues.empty(), "chunk with empty payload should have no issues");
}

void TestEmptyChunkIdInvalid()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSaveChunkArchiveBuilder {}
		.addChunk(Chunk({}))
		.build();

	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult result = iggy::runtime::RuntimeSaveChunkArchiveValidator {}.validate(archive);

	Expect(!result.valid, "empty chunk id should invalidate archive");
	Expect(result.issues.size() == 1, "empty chunk id should report one issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::EmptyChunkId, 0, {}), "empty chunk id should report issue details");
}

void TestChunkVersionZeroInvalid()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSaveChunkArchiveBuilder {}
		.addChunk(Chunk(SessionId, 0))
		.build();

	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult result = iggy::runtime::RuntimeSaveChunkArchiveValidator {}.validate(archive);

	Expect(!result.valid, "chunk version zero should invalidate archive");
	Expect(result.issues.size() == 1, "chunk version zero should report one issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidChunkVersion, 0, SessionId), "chunk version zero should report issue details");
}

void TestInvalidMagicReported()
{
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.magic = { 'B', 'A', 'D', '!' };

	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult result = iggy::runtime::RuntimeSaveChunkArchiveValidator {}.validate(archive);

	Expect(!result.valid, "invalid magic should invalidate archive");
	Expect(result.issues.size() == 1, "invalid magic should report one issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidMagic), "invalid magic should report issue details");
}

void TestArchiveVersionZeroInvalid()
{
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.version = 0;

	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult result = iggy::runtime::RuntimeSaveChunkArchiveValidator {}.validate(archive);

	Expect(!result.valid, "archive version zero should invalidate archive");
	Expect(result.issues.size() == 1, "archive version zero should report one issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidArchiveVersion), "archive version zero should report issue details");
}

void TestMultipleIssuesPreserveDeterministicOrder()
{
	iggy::runtime::RuntimeSaveChunkArchive archive;
	archive.magic = { 'B', 'A', 'D', '!' };
	archive.version = 0;
	archive.chunks.push_back(Chunk({}, 0));
	archive.chunks.push_back(Chunk(SessionId, 0));

	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult result = iggy::runtime::RuntimeSaveChunkArchiveValidator {}.validate(archive);

	Expect(!result.valid, "multiple archive issues should invalidate archive");
	Expect(result.issues.size() == 5, "validator should report all archive and chunk issues");
	if (result.issues.size() == 5) {
		Expect(result.issues[0].code == iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidMagic, "invalid magic should report first");
		Expect(result.issues[1].code == iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidArchiveVersion, "invalid archive version should report second");
		Expect(result.issues[2].code == iggy::runtime::RuntimeSaveChunkArchiveIssueCode::EmptyChunkId && result.issues[2].chunkIndex == 0, "empty chunk id should report in chunk order");
		Expect(result.issues[3].code == iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidChunkVersion && result.issues[3].chunkIndex == 0, "invalid chunk version should report after empty id for same chunk");
		Expect(result.issues[4].code == iggy::runtime::RuntimeSaveChunkArchiveIssueCode::InvalidChunkVersion && result.issues[4].chunkIndex == 1, "second invalid chunk version should preserve chunk order");
	}
}

void TestDuplicateChunkIdsAllowedAndFindIndexesPreserveOrder()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSaveChunkArchiveBuilder {}
		.addChunk(Chunk(SessionId, 1, { 1 }))
		.addChunk(Chunk(LevelMapId, 1, { 2 }))
		.addChunk(Chunk(SessionId, 2, { 3 }))
		.build();

	const iggy::runtime::RuntimeSaveChunkArchiveValidationResult result = iggy::runtime::RuntimeSaveChunkArchiveValidator {}.validate(archive);
	const std::vector<std::size_t> indexes = iggy::runtime::findChunkIndexes(archive, SessionId);

	Expect(result.valid, "duplicate chunk ids should validate");
	Expect(indexes == std::vector<std::size_t> { 0, 2 }, "findChunkIndexes should return duplicate ids in archive order");
}

void TestExactIdMatching()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSaveChunkArchiveBuilder {}
		.addChunk(Chunk(SessionId, 1, { 1 }))
		.addChunk(Chunk(SessionAltId, 1, { 2 }))
		.build();

	const std::vector<std::size_t> sessionIndexes = iggy::runtime::findChunkIndexes(archive, SessionId);
	const std::vector<std::size_t> altIndexes = iggy::runtime::findChunkIndexes(archive, SessionAltId);

	Expect(sessionIndexes == std::vector<std::size_t> { 0 }, "findChunkIndexes should distinguish SESS exactly");
	Expect(altIndexes == std::vector<std::size_t> { 1 }, "findChunkIndexes should distinguish SESa exactly");
}

void TestFindFirstChunk()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSaveChunkArchiveBuilder {}
		.addChunk(Chunk(LevelMapId, 1, { 1 }))
		.addChunk(Chunk(PlayerId, 2, { 2 }))
		.build();

	const iggy::runtime::RuntimeSaveChunk *chunk = iggy::runtime::findFirstChunk(archive, PlayerId);
	const iggy::runtime::RuntimeSaveChunk *missing = iggy::runtime::findFirstChunk(archive, SessionId);

	Expect(chunk != nullptr, "findFirstChunk should find matching chunk");
	if (chunk != nullptr) {
		Expect(chunk->id == PlayerId, "findFirstChunk should return matching chunk id");
		Expect(chunk->version == 2, "findFirstChunk should return matching chunk version");
		Expect(SamePayload(chunk->payload, { 2 }), "findFirstChunk should return matching chunk payload");
	}
	Expect(missing == nullptr, "findFirstChunk should return null for missing chunk");
}

void TestBuilderCopiesInputChunks()
{
	iggy::runtime::RuntimeSaveChunk chunk = Chunk(SessionId, 1, { 1, 2, 3 });
	iggy::runtime::RuntimeSaveChunkArchiveBuilder builder;
	builder.addChunk(chunk);
	chunk.id = LevelMapId;
	chunk.version = 9;
	chunk.payload[0] = 99;

	const iggy::runtime::RuntimeSaveChunkArchive archive = builder.build();

	Expect(archive.chunks.size() == 1, "builder copy test should produce one chunk");
	if (archive.chunks.size() == 1) {
		Expect(archive.chunks[0].id == SessionId, "builder should copy chunk id on add");
		Expect(archive.chunks[0].version == 1, "builder should copy chunk version on add");
		Expect(SamePayload(archive.chunks[0].payload, { 1, 2, 3 }), "builder should copy payload on add");
	}
}

} // namespace

int main()
{
	TestDefaultArchiveValidates();
	TestBuilderAppendsAndPreservesChunks();
	TestEmptyPayloadIsValid();
	TestEmptyChunkIdInvalid();
	TestChunkVersionZeroInvalid();
	TestInvalidMagicReported();
	TestArchiveVersionZeroInvalid();
	TestMultipleIssuesPreserveDeterministicOrder();
	TestDuplicateChunkIdsAllowedAndFindIndexesPreserveOrder();
	TestExactIdMatching();
	TestFindFirstChunk();
	TestBuilderCopiesInputChunks();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
