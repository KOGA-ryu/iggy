#include "core/math/Transform3.hpp"

namespace iggy {

bool operator==(Transform3 left, Transform3 right)
{
	return left.position == right.position &&
		left.yawPitchRoll == right.yawPitchRoll &&
		left.scale == right.scale;
}

} // namespace iggy
