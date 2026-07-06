#pragma once

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct Ray3 {
  Vec3 origin{};
  Vec3 direction{0.0F, 0.0F, -1.0F};
};

}  // namespace iggy3d
