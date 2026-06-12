#include "runtime/RuntimeSaveChunkArchive.hpp"

namespace iggy::runtime {
namespace {

constexpr std::array<char, 4> ValidMagic { 'I', 'G', 'G', 'Y' };

[[nodiscard]] bool isEmpty(RuntimeSaveChunkId id)
{
	return id.value == std::array<char, 4> { '\0', '\0', '\0', '\0' };
}

} // namespace

bool operator==(RuntimeSaveChunkId left, RuntimeSaveChunkId right)
{
	return left.value == right.value;
}

bool operator!=(RuntimeSaveChunkId left, RuntimeSaveChunkId right)
{
	return !(left == right);
}

RuntimeSaveChunkId makeRuntimeSaveChunkId(char a, char b, char c, char d)
{
	return { { a, b, c, d } };
}

RuntimeSaveChunkArchiveBuilder &RuntimeSaveChunkArchiveBuilder::addChunk(RuntimeSaveChunk chunk)
{
	archive_.chunks.push_back(chunk);
	return *this;
}

RuntimeSaveChunkArchive RuntimeSaveChunkArchiveBuilder::build() const
{
	return archive_;
}

RuntimeSaveChunkArchiveValidationResult RuntimeSaveChunkArchiveValidator::validate(const RuntimeSaveChunkArchive &archive) const
{
	RuntimeSaveChunkArchiveValidationResult result;
	if (archive.magic != ValidMagic)
		result.issues.push_back({ RuntimeSaveChunkArchiveIssueCode::InvalidMagic, 0, {} });
	if (archive.version == 0)
		result.issues.push_back({ RuntimeSaveChunkArchiveIssueCode::InvalidArchiveVersion, 0, {} });

	for (std::size_t index = 0; index < archive.chunks.size(); ++index) {
		const RuntimeSaveChunk &chunk = archive.chunks[index];
		if (isEmpty(chunk.id))
			result.issues.push_back({ RuntimeSaveChunkArchiveIssueCode::EmptyChunkId, index, chunk.id });
		if (chunk.version == 0)
			result.issues.push_back({ RuntimeSaveChunkArchiveIssueCode::InvalidChunkVersion, index, chunk.id });
	}

	result.valid = result.issues.empty();
	return result;
}

const RuntimeSaveChunk *findFirstChunk(const RuntimeSaveChunkArchive &archive, RuntimeSaveChunkId id)
{
	for (const RuntimeSaveChunk &chunk : archive.chunks) {
		if (chunk.id == id)
			return &chunk;
	}
	return nullptr;
}

std::vector<std::size_t> findChunkIndexes(const RuntimeSaveChunkArchive &archive, RuntimeSaveChunkId id)
{
	std::vector<std::size_t> indexes;
	for (std::size_t index = 0; index < archive.chunks.size(); ++index) {
		if (archive.chunks[index].id == id)
			indexes.push_back(index);
	}
	return indexes;
}

} // namespace iggy::runtime
