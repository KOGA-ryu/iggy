#pragma once

#include <cstdint>

namespace iggy3d::creative {

using Id = std::uint32_t;

inline constexpr Id kInvalidId = 0;

enum class Tool : std::uint8_t {
  Select,
  Inspect,
  Measure,
};

enum class PacketKind : std::uint8_t {
  None,
};

struct FrameRef {
  std::uint64_t value = 0;
};

struct TargetRef {
  Id value = kInvalidId;
};

struct Flags {
  bool enabled = false;
  bool active = false;
  bool dirty = false;
};

struct FramePacket {
  FrameRef frame;
  Flags flags;
};

struct Packet {
  PacketKind kind = PacketKind::None;
  FrameRef frame;
  TargetRef target;
  Flags flags;
};

}  // namespace iggy3d::creative
