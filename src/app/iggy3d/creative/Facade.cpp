#include "app/iggy3d/creative/Facade.hpp"

namespace iggy3d::creative {

void Facade::reset() noexcept {
  state_ = State{};
  resetStats(stats_);
}

void Facade::beginFrame(const FramePacket& packet) noexcept {
  state_.frame = packet.frame;
  state_.flags = packet.flags;
}

void Facade::handle(const Packet& packet) noexcept {
  state_.frame = packet.frame;
  state_.hovered = packet.target;
  state_.flags = packet.flags;
}

const Stats& Facade::stats() const noexcept {
  return stats_;
}

}  // namespace iggy3d::creative
