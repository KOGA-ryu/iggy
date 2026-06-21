#pragma once

#include <array>

namespace iggy {

struct Mat4 {
	std::array<float, 16> values {};

	[[nodiscard]] static Mat4 Identity();
};

[[nodiscard]] bool operator==(const Mat4 &left, const Mat4 &right);

} // namespace iggy
