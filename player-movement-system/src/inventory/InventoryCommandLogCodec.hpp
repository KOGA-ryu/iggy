#pragma once

#include <optional>

#include "inventory/InventoryCommandCodec.hpp"
#include "inventory/InventoryCommandLogBytes.hpp"
#include "inventory/InventoryCommandLogFrameCodec.hpp"
#include "inventory/InventoryCommandLog.hpp"

namespace dev {

class InventoryCommandLogCodec {
public:
	[[nodiscard]] InventoryCommandLogBytes encode(const InventoryCommandLog &log) const;
	[[nodiscard]] std::optional<InventoryCommandLog> decode(const InventoryCommandLogBytes &bytes) const;

private:
	InventoryCommandCodec commandCodec_;
	InventoryCommandLogFrameCodec frameCodec_;
};

} // namespace dev
