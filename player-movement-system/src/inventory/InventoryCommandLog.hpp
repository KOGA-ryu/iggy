#pragma once

#include <vector>

#include "inventory/InventoryCommand.hpp"

namespace dev {

class InventoryCommandLog {
public:
	void record(const InventoryCommand &command);
	void clear();

	[[nodiscard]] const std::vector<InventoryCommand> &commands() const;
	[[nodiscard]] bool empty() const;

private:
	std::vector<InventoryCommand> commands_;
};

} // namespace dev
