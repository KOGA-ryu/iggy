#include "RuntimeFrameTraceFileStore.hpp"

#include "files/TextFileStore.hpp"

namespace dev {

bool RuntimeFrameTraceFileStore::save(const std::filesystem::path &path, const std::vector<std::string> &lines) const
{
	return TextFileStore {}.saveLines(path, lines);
}

std::optional<std::vector<std::string>> RuntimeFrameTraceFileStore::load(const std::filesystem::path &path) const
{
	return TextFileStore {}.loadLines(path);
}

} // namespace dev
