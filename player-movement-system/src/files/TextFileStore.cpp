#include "TextFileStore.hpp"

#include <fstream>

namespace dev {

bool TextFileStore::saveLines(const std::filesystem::path &path, const std::vector<std::string> &lines) const
{
	const std::filesystem::path tempPath = path.string() + ".tmp";

	{
		std::ofstream output { tempPath, std::ios::trunc };
		if (!output)
			return false;
		for (const std::string &line : lines)
			output << line << '\n';
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

std::optional<std::vector<std::string>> TextFileStore::loadLines(const std::filesystem::path &path) const
{
	std::ifstream input { path };
	if (!input)
		return std::nullopt;

	std::vector<std::string> lines;
	std::string line;
	while (std::getline(input, line))
		lines.push_back(line);
	if (!input.eof() && input.fail())
		return std::nullopt;

	return lines;
}

} // namespace dev
