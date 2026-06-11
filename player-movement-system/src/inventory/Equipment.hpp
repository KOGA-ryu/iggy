#pragma once

#include <optional>

#include "items/Item.hpp"

namespace dev {

struct Equipment {
	std::optional<Item> weapon;
	std::optional<Item> armor;
	std::optional<Item> accessory;
};

} // namespace dev
