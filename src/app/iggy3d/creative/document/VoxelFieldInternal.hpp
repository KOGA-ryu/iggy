#pragma once

#include <cstddef>
#include <cstdint>

namespace iggy3d::creative::voxel_field_internal {

[[nodiscard]] std::size_t localIndex(std::int32_t x,
                                     std::int32_t y,
                                     std::int32_t z) noexcept;

}  // namespace iggy3d::creative::voxel_field_internal
