#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

// Intrinsic X-then-Y-then-Z Euler rotation (R = Rz * Ry * Rx), Y-up.
[[nodiscard]] Vec3 rotateEulerXyz(Vec3 point, Vec3 eulerRadians);

}  // namespace iggy3d
