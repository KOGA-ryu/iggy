#include "InventoryCommandLogFileStore.hpp"

#include "files/ByteFileStore.hpp"

namespace dev {

InventoryCommandLogFileStore::InventoryCommandLogFileStore(InventoryCommandLogCodec codec)
    : codec_(codec)
{
}

bool InventoryCommandLogFileStore::save(const std::filesystem::path &path, const InventoryCommandLog &log) const
{
	const InventoryCommandLogBytes bytes = codec_.encode(log);
	return ByteFileStore {}.save(path, bytes);
}

std::optional<InventoryCommandLog> InventoryCommandLogFileStore::load(const std::filesystem::path &path) const
{
	std::optional<std::vector<uint8_t>> bytes = ByteFileStore {}.load(path);
	if (!bytes.has_value())
		return std::nullopt;

	return codec_.decode(*bytes);
}

} // namespace dev
