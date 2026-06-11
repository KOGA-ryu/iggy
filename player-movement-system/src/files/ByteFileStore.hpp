#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace dev {

class ByteFileStore {
public:
	[[nodiscard]] bool save(const std::filesystem::path &path, const std::vector<uint8_t> &bytes) const;
	[[nodiscard]] std::optional<std::vector<uint8_t>> load(const std::filesystem::path &path) const;
};

} // namespace dev
