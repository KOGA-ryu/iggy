#pragma once

#include "app/iggy3d/creative/Core.hpp"
#include "app/iggy3d/creative/Metrics.hpp"
#include "app/iggy3d/creative/State.hpp"

namespace iggy3d::creative {

class Facade {
 public:
  void reset() noexcept;
  void beginFrame(const FramePacket& packet) noexcept;
  void handle(const Packet& packet) noexcept;

  [[nodiscard]] const Stats& stats() const noexcept;

 private:
  State state_;
  Stats stats_;
};

}  // namespace iggy3d::creative
