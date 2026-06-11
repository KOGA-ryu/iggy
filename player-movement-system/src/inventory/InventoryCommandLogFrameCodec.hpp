#pragma once

#include <optional>
#include <vector>

#include "inventory/InventoryCommandLogBytes.hpp"
#include "inventory/InventoryCommandPacket.hpp"

namespace dev {

class InventoryCommandLogFrameCodec {
public:
	[[nodiscard]] InventoryCommandLogBytes encode(const std::vector<InventoryCommandBytes> &packets) const;
	[[nodiscard]] std::optional<std::vector<InventoryCommandBytes>> decode(const InventoryCommandLogBytes &bytes) const;
};

} // namespace dev
