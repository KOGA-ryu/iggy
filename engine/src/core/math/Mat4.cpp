#include "core/math/Mat4.hpp"

namespace iggy {

Mat4 Mat4::Identity()
{
	Mat4 matrix;
	matrix.values[0] = 1.0F;
	matrix.values[5] = 1.0F;
	matrix.values[10] = 1.0F;
	matrix.values[15] = 1.0F;
	return matrix;
}

bool operator==(const Mat4 &left, const Mat4 &right)
{
	return left.values == right.values;
}

} // namespace iggy
