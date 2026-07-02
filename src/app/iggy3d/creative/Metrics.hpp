#pragma once

#include <cstdint>

namespace iggy3d::creative {

struct Stats {
  std::uint64_t frames = 0;
  std::uint64_t packets = 0;
  std::uint64_t handled = 0;
  std::uint64_t ignored = 0;
};

void resetStats(Stats& stats) noexcept;

}  // namespace iggy3d::creative
