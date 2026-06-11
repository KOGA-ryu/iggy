#pragma once

#include <optional>

#include "inventory/InventoryCommandCodec.hpp"
#include "inventory/InventoryCommandLog.hpp"

namespace dev {

using InventoryCommandLogBytes = std::vector<uint8_t>;

class InventoryCommandLogCodec {
public:
	[[nodiscard]] InventoryCommandLogBytes encode(const InventoryCommandLog &log) const;
	[[nodiscard]] std::optional<InventoryCommandLog> decode(const InventoryCommandLogBytes &bytes) const;

private:
	InventoryCommandCodec commandCodec_;
};

} // namespace dev
