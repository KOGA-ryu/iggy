#include "CommandLogFileStore.hpp"

#include "files/ByteFileStore.hpp"

namespace dev {

CommandLogFileStore::CommandLogFileStore(CommandLogCodec codec)
    : codec_(codec)
{
}

bool CommandLogFileStore::save(const std::filesystem::path &path, const CommandLog &log) const
{
	const CommandLogBytes bytes = codec_.encode(log);
	return ByteFileStore {}.save(path, bytes);
}

std::optional<CommandLog> CommandLogFileStore::load(const std::filesystem::path &path) const
{
	std::optional<std::vector<uint8_t>> bytes = ByteFileStore {}.load(path);
	if (!bytes.has_value())
		return std::nullopt;

	return codec_.decode(*bytes);
}

} // namespace dev
