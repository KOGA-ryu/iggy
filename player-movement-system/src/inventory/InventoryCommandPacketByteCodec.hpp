#pragma once

#include <optional>

#include "inventory/InventoryCommandPacket.hpp"

namespace dev {

class InventoryCommandPacketByteCodec {
public:
	[[nodiscard]] InventoryCommandBytes encode(const InventoryCommandPacket &packet) const;
	[[nodiscard]] std::optional<InventoryCommandPacket> decode(const InventoryCommandBytes &bytes) const;
};

} // namespace dev
