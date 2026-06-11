#pragma once

#include <cstddef>
#include <vector>

#include "inventory/Equipment.hpp"
#include "items/Item.hpp"

namespace dev {

struct Inventory {
	std::vector<Item> items;
	Equipment equipment;
	std::size_t capacity = 40;

	[[nodiscard]] bool full() const
	{
		return items.size() >= capacity;
	}
};

} // namespace dev
