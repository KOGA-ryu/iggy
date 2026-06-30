#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "core/math/Vec3.hpp"

namespace iggy3d {

using StableHashValue = std::uint64_t;

inline constexpr StableHashValue kStableHashOffsetBasis = 14695981039346656037ULL;
inline constexpr StableHashValue kStableHashPrime = 1099511628211ULL;

struct StableHasher {
  StableHasher();
  void addByte(std::uint8_t value);
  void addU64(std::uint64_t value);
  void addI64(std::int64_t value);
  void addBool(bool value);
  void addString(std::string_view value);
  void addFloatQuantized(float value, float scale = 1000.0F);
  StableHashValue value() const;

 private:
  StableHashValue hash_ = kStableHashOffsetBasis;
};

void addVec3Quantized(StableHasher& hasher, Vec3 value, float scale = 1000.0F);

}  // namespace iggy3d
