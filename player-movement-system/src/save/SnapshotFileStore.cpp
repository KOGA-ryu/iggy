#include "SnapshotFileStore.hpp"

#include "files/ByteFileStore.hpp"

namespace dev {

SnapshotFileStore::SnapshotFileStore(SnapshotCodec codec)
    : codec_(codec)
{
}

bool SnapshotFileStore::save(const std::filesystem::path &path, const SimulationSnapshot &snapshot) const
{
	const SnapshotBytes bytes = codec_.encode(snapshot);
	return ByteFileStore {}.save(path, bytes);
}

std::optional<SimulationSnapshot> SnapshotFileStore::load(const std::filesystem::path &path) const
{
	std::optional<std::vector<uint8_t>> bytes = ByteFileStore {}.load(path);
	if (!bytes.has_value())
		return std::nullopt;

	return codec_.decode(*bytes);
}

} // namespace dev
