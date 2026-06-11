#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "inventory/InventoryScriptRunner.hpp"

namespace dev {

class RuntimeInventoryScriptText {
public:
	[[nodiscard]] std::string formatResult(std::string_view label, const InventoryScriptRunResult &result) const;
	[[nodiscard]] std::string formatAggregate(std::string_view label, const std::vector<InventoryScriptRunResult> &results) const;
};

} // namespace dev
