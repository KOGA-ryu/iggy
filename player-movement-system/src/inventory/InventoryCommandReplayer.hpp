#pragma once

#include <vector>

#include "inventory/InventoryCommandDispatcher.hpp"
#include "inventory/InventoryCommandLog.hpp"

namespace dev {

class InventoryCommandReplayer {
public:
	explicit InventoryCommandReplayer(InventoryCommandDispatcher &dispatcher);

	[[nodiscard]] std::vector<InventoryCommandResult> replay(const InventoryCommandLog &log) const;

private:
	InventoryCommandDispatcher &dispatcher_;
};

} // namespace dev
