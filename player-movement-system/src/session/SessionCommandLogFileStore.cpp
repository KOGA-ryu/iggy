#include "SessionCommandLogFileStore.hpp"

#include "files/ByteFileStore.hpp"

namespace dev {

SessionCommandLogFileStore::SessionCommandLogFileStore(SessionCommandLogCodec codec)
    : codec_(codec)
{
}

bool SessionCommandLogFileStore::save(const std::filesystem::path &path, const SessionCommandLog &log) const
{
	const SessionCommandLogBytes bytes = codec_.encode(log);
	return ByteFileStore {}.save(path, bytes);
}

std::optional<SessionCommandLog> SessionCommandLogFileStore::load(const std::filesystem::path &path) const
{
	std::optional<std::vector<uint8_t>> bytes = ByteFileStore {}.load(path);
	if (!bytes.has_value())
		return std::nullopt;

	return codec_.decode(*bytes);
}

} // namespace dev
