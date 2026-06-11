#include "ByteFileStore.hpp"

#include <fstream>

namespace dev {

bool ByteFileStore::save(const std::filesystem::path &path, const std::vector<uint8_t> &bytes) const
{
	const std::filesystem::path tempPath = path.string() + ".tmp";

	{
		std::ofstream output { tempPath, std::ios::binary | std::ios::trunc };
		if (!output)
			return false;
		output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		if (!output) {
			std::filesystem::remove(tempPath);
			return false;
		}
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

std::optional<std::vector<uint8_t>> ByteFileStore::load(const std::filesystem::path &path) const
{
	std::ifstream input { path, std::ios::binary };
	if (!input)
		return std::nullopt;

	std::vector<uint8_t> bytes {
		std::istreambuf_iterator<char> { input },
		std::istreambuf_iterator<char> {},
	};
	if (!input.eof() && input.fail())
		return std::nullopt;

	return bytes;
}

} // namespace dev
