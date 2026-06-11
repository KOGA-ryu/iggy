#pragma once

#include <cstddef>
#include <vector>

#include "inventory/InventoryCommand.hpp"

namespace dev {

class InventoryCommandSource {
public:
	virtual ~InventoryCommandSource() = default;

	[[nodiscard]] virtual std::vector<InventoryCommand> drain() = 0;
};

class QueuedInventoryCommandSource : public InventoryCommandSource {
public:
	void enqueue(const InventoryCommand &command);
	void clear();

	[[nodiscard]] std::vector<InventoryCommand> drain() override;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t size() const;

private:
	std::vector<InventoryCommand> commands_;
};

} // namespace dev
