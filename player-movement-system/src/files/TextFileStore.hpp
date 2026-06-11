#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dev {

class TextFileStore {
public:
	[[nodiscard]] bool saveLines(const std::filesystem::path &path, const std::vector<std::string> &lines) const;
	[[nodiscard]] std::optional<std::vector<std::string>> loadLines(const std::filesystem::path &path) const;
};

} // namespace dev
