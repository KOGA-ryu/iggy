#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/world/WorldLayout.hpp"

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeWorldLayoutCodecVersion = 28U;
inline constexpr std::size_t kCreativeWorldLayoutCodecMaxEncodedBytes =
    8U * 1024U * 1024U;
inline constexpr std::size_t kCreativeWorldLayoutCodecMaxRecords = 65535U;
inline constexpr std::size_t kCreativeWorldLayoutCodecMaxStringBytes = 4096U;

enum class CreativeWorldLayoutCodecStatus : std::uint8_t {
  NotRequested,
  Ready,
  EmptyInput,
  EncodedSizeExceeded,
  UnsupportedVersion,
  InvalidHeader,
  InvalidRecord,
  InvalidNumber,
  InvalidEnum,
  InvalidString,
  NonFiniteValue,
  CapacityExceeded,
  TrailingData,
};

struct CreativeWorldLayoutEncodeResult {
  bool accepted = false;
  CreativeWorldLayoutCodecStatus status =
      CreativeWorldLayoutCodecStatus::NotRequested;
  std::string encodedText;
  std::string reasonCode = "creative_world_layout_encode_not_requested";
};

struct CreativeWorldLayoutDecodeResult {
  bool accepted = false;
  CreativeWorldLayoutCodecStatus status =
      CreativeWorldLayoutCodecStatus::NotRequested;
  CreativeWorldLayout layout;
  std::size_t failedLine = 0U;
  std::string reasonCode = "creative_world_layout_decode_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutCodecStatus status) noexcept;

// Deterministic, versioned source codec for the semantic 2D authoring model.
// Strings are hexadecimal byte tokens, so names and tags can contain spaces,
// newlines, percent signs, and '=' without becoming record syntax.
[[nodiscard]] CreativeWorldLayoutEncodeResult encodeCreativeWorldLayout(
    const CreativeWorldLayout& layout);
[[nodiscard]] CreativeWorldLayoutDecodeResult decodeCreativeWorldLayout(
    std::string_view encodedText);
[[nodiscard]] std::uint64_t fingerprintCreativeWorldLayout(
    const CreativeWorldLayout& layout);

}  // namespace iggy3d::creative
