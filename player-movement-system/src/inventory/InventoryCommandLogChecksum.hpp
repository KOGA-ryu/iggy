#pragma once

#include <cstddef>
#include <cstdint>

#include "inventory/InventoryCommandLogBytes.hpp"

namespace dev {

class InventoryCommandLogChecksum {
public:
	void appendTo(InventoryCommandLogBytes &bytes) const;
	[[nodiscard]] bool hasValidTrailingChecksum(const InventoryCommandLogBytes &bytes, std::size_t payloadSize) const;
	[[nodiscard]] uint32_t compute(const InventoryCommandLogBytes &bytes, std::size_t length) const;

private:
	[[nodiscard]] uint32_t readTrailingU32(const InventoryCommandLogBytes &bytes) const;
};

} // namespace dev
