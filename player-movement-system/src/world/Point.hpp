#pragma once

namespace dev {

struct Point {
	int x = 0;
	int y = 0;

	friend bool operator==(Point lhs, Point rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}
};

} // namespace dev

