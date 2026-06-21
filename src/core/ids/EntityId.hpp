#pragma once

#include <cstdint>

namespace iggy3d {

struct EntityId {
  std::uint64_t value = 0;
};

inline constexpr EntityId kInvalidEntityId{};

inline bool operator==(EntityId lhs, EntityId rhs) {
  return lhs.value == rhs.value;
}

inline bool operator!=(EntityId lhs, EntityId rhs) {
  return !(lhs == rhs);
}

inline bool operator<(EntityId lhs, EntityId rhs) {
  return lhs.value < rhs.value;
}

inline bool isValid(EntityId id) {
  return id.value != 0;
}

inline EntityId nextEntityId(EntityId id) {
  return EntityId{id.value + 1U};
}

inline std::uint64_t toUint64(EntityId id) {
  return id.value;
}

inline EntityId entityIdFromUint64(std::uint64_t value) {
  return EntityId{value};
}

}  // namespace iggy3d
