#pragma once

#include <filesystem>
#include <optional>

#include "inventory/InventoryCommandLogCodec.hpp"

namespace dev {

class InventoryCommandLogFileStore {
public:
	explicit InventoryCommandLogFileStore(InventoryCommandLogCodec codec = InventoryCommandLogCodec {});

	[[nodiscard]] bool save(const std::filesystem::path &path, const InventoryCommandLog &log) const;
	[[nodiscard]] std::optional<InventoryCommandLog> load(const std::filesystem::path &path) const;

private:
	InventoryCommandLogCodec codec_;
};

} // namespace dev
