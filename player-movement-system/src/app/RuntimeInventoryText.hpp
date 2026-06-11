#pragma once

#include <string>
#include <string_view>

#include "inventory/InventoryCommand.hpp"
#include "inventory/InventoryEvent.hpp"

namespace dev {

class RuntimeInventoryText {
public:
	[[nodiscard]] std::string formatResult(std::string_view label, const InventoryCommandResult &result) const;
	[[nodiscard]] std::string formatEvent(std::string_view label, const InventoryEvent &event) const;
};

} // namespace dev
