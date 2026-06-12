#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace iggy::runtime {

struct RuntimeSaveChunkId {
	std::array<char, 4> value { '\0', '\0', '\0', '\0' };
};

[[nodiscard]] bool operator==(RuntimeSaveChunkId left, RuntimeSaveChunkId right);
[[nodiscard]] bool operator!=(RuntimeSaveChunkId left, RuntimeSaveChunkId right);
[[nodiscard]] RuntimeSaveChunkId makeRuntimeSaveChunkId(char a, char b, char c, char d);

struct RuntimeSaveChunk {
	RuntimeSaveChunkId id;
	std::uint32_t version = 1;
	std::vector<std::uint8_t> payload;
};

struct RuntimeSaveChunkArchive {
	std::array<char, 4> magic { 'I', 'G', 'G', 'Y' };
	std::uint32_t version = 1;
	std::vector<RuntimeSaveChunk> chunks;
};

enum class RuntimeSaveChunkArchiveIssueCode {
	InvalidMagic,
	InvalidArchiveVersion,
	EmptyChunkId,
	InvalidChunkVersion,
	DuplicateRequiredChunk,
};

struct RuntimeSaveChunkArchiveIssue {
	RuntimeSaveChunkArchiveIssueCode code = RuntimeSaveChunkArchiveIssueCode::InvalidMagic;
	std::size_t chunkIndex = 0;
	RuntimeSaveChunkId chunkId;
};

struct RuntimeSaveChunkArchiveValidationResult {
	bool valid = false;
	std::vector<RuntimeSaveChunkArchiveIssue> issues;
};

class RuntimeSaveChunkArchiveBuilder {
public:
	RuntimeSaveChunkArchiveBuilder &addChunk(RuntimeSaveChunk chunk);
	[[nodiscard]] RuntimeSaveChunkArchive build() const;

private:
	RuntimeSaveChunkArchive archive_;
};

class RuntimeSaveChunkArchiveValidator {
public:
	[[nodiscard]] RuntimeSaveChunkArchiveValidationResult validate(const RuntimeSaveChunkArchive &archive) const;
};

[[nodiscard]] const RuntimeSaveChunk *findFirstChunk(const RuntimeSaveChunkArchive &archive, RuntimeSaveChunkId id);
[[nodiscard]] std::vector<std::size_t> findChunkIndexes(const RuntimeSaveChunkArchive &archive, RuntimeSaveChunkId id);

} // namespace iggy::runtime
