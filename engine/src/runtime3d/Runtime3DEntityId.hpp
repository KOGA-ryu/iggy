#pragma once

#include <cstdint>

namespace iggy::runtime3d {

struct Runtime3DEntityId {
	std::uint64_t value = 0;

	[[nodiscard]] static constexpr Runtime3DEntityId Invalid()
	{
		return {};
	}

	[[nodiscard]] constexpr bool valid() const
	{
		return value != 0;
	}
};

[[nodiscard]] constexpr bool operator==(Runtime3DEntityId left, Runtime3DEntityId right)
{
	return left.value == right.value;
}

[[nodiscard]] constexpr bool operator!=(Runtime3DEntityId left, Runtime3DEntityId right)
{
	return !(left == right);
}

} // namespace iggy::runtime3d
