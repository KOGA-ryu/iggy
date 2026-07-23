#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "core/hash/StableHash.hpp"

namespace iggy3d::creative {

std::string_view toString(CreativeWorldLayoutCodecStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutCodecStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutCodecStatus::Ready:
      return "Ready";
    case CreativeWorldLayoutCodecStatus::EmptyInput:
      return "EmptyInput";
    case CreativeWorldLayoutCodecStatus::EncodedSizeExceeded:
      return "EncodedSizeExceeded";
    case CreativeWorldLayoutCodecStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeWorldLayoutCodecStatus::InvalidHeader:
      return "InvalidHeader";
    case CreativeWorldLayoutCodecStatus::InvalidRecord:
      return "InvalidRecord";
    case CreativeWorldLayoutCodecStatus::InvalidNumber:
      return "InvalidNumber";
    case CreativeWorldLayoutCodecStatus::InvalidEnum:
      return "InvalidEnum";
    case CreativeWorldLayoutCodecStatus::InvalidString:
      return "InvalidString";
    case CreativeWorldLayoutCodecStatus::NonFiniteValue:
      return "NonFiniteValue";
    case CreativeWorldLayoutCodecStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeWorldLayoutCodecStatus::TrailingData:
      return "TrailingData";
  }
  return "Unknown";
}

std::uint64_t fingerprintCreativeWorldLayout(
    const CreativeWorldLayout& layout) {
  const CreativeWorldLayoutEncodeResult encoded =
      encodeCreativeWorldLayout(layout);
  if (!encoded.accepted) {
    return 0U;
  }
  StableHasher hasher;
  hasher.addString("creative_world_layout_source_v1");
  hasher.addString(encoded.encodedText);
  const std::uint64_t fingerprint = hasher.value();
  return fingerprint != 0U ? fingerprint : 0U;
}

}  // namespace iggy3d::creative
