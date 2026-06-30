#include "core/hash/StableHash.hpp"

#include <cmath>

namespace iggy3d {

StableHasher::StableHasher() = default;

void StableHasher::addByte(std::uint8_t value) {
  hash_ ^= static_cast<StableHashValue>(value);
  hash_ *= kStableHashPrime;
}

void StableHasher::addU64(std::uint64_t value) {
  for (std::uint32_t index = 0; index < 8U; ++index) {
    addByte(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
  }
}

void StableHasher::addI64(std::int64_t value) {
  addU64(static_cast<std::uint64_t>(value));
}

void StableHasher::addBool(bool value) {
  addByte(value ? 1U : 0U);
}

void StableHasher::addString(std::string_view value) {
  addU64(value.size());
  for (char byte : value) {
    addByte(static_cast<std::uint8_t>(byte));
  }
}

void StableHasher::addFloatQuantized(float value, float scale) {
  addI64(static_cast<std::int64_t>(std::llround(value * scale)));
}

StableHashValue StableHasher::value() const {
  return hash_;
}

void addVec3Quantized(StableHasher& hasher, Vec3 value, float scale) {
  hasher.addFloatQuantized(value.x, scale);
  hasher.addFloatQuantized(value.y, scale);
  hasher.addFloatQuantized(value.z, scale);
}

}  // namespace iggy3d
