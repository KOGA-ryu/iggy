#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dev {

class RuntimeFrameTraceFileStore {
public:
	[[nodiscard]] bool save(const std::filesystem::path &path, const std::vector<std::string> &lines) const;
	[[nodiscard]] std::optional<std::vector<std::string>> load(const std::filesystem::path &path) const;
};

} // namespace dev
