#include "SessionCommandLogFileStore.hpp"

#include <fstream>

namespace dev {

SessionCommandLogFileStore::SessionCommandLogFileStore(SessionCommandLogCodec codec)
    : codec_(codec)
{
}

bool SessionCommandLogFileStore::save(const std::filesystem::path &path, const SessionCommandLog &log) const
{
	const SessionCommandLogBytes bytes = codec_.encode(log);
	const std::filesystem::path tempPath = path.string() + ".tmp";

	{
		std::ofstream output { tempPath, std::ios::binary | std::ios::trunc };
		if (!output)
			return false;
		output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		if (!output)
			return false;
	}

	std::error_code error;
	std::filesystem::remove(path, error);
	error.clear();
	std::filesystem::rename(tempPath, path, error);
	if (error) {
		std::filesystem::remove(tempPath);
		return false;
	}

	return true;
}

std::optional<SessionCommandLog> SessionCommandLogFileStore::load(const std::filesystem::path &path) const
{
	std::ifstream input { path, std::ios::binary };
	if (!input)
		return std::nullopt;

	SessionCommandLogBytes bytes {
		std::istreambuf_iterator<char> { input },
		std::istreambuf_iterator<char> {},
	};
	if (!input.eof() && input.fail())
		return std::nullopt;

	return codec_.decode(bytes);
}

} // namespace dev
