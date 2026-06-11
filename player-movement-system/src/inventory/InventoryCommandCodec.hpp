#pragma once

#include <optional>

#include "inventory/InventoryCommand.hpp"
#include "inventory/InventoryCommandPacket.hpp"

namespace dev {

class InventoryCommandCodec {
public:
	[[nodiscard]] InventoryCommandPacket toPacket(const InventoryCommand &command) const;
	[[nodiscard]] std::optional<InventoryCommand> fromPacket(const InventoryCommandPacket &packet) const;

	[[nodiscard]] InventoryCommandBytes encode(const InventoryCommandPacket &packet) const;
	[[nodiscard]] std::optional<InventoryCommandPacket> decode(const InventoryCommandBytes &bytes) const;
};

} // namespace dev
