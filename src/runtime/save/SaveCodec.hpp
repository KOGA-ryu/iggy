#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "runtime/replay/StateHash.hpp"
#include "runtime/save/SaveEnvelope.hpp"

namespace iggy3d {

enum class SaveCodecStatus : std::uint8_t {
  Ok,
  EncodeFailed,
  DecodeFailed,
  DuplicateKey,
  MissingField,
  UnsupportedVersion,
  InvalidEnum,
  InvalidNumber,
  InvalidId,
  InvalidSequence,
  InvalidSectionOrder,
  SaveTooLarge,
  UnknownKey,
};

struct SaveEncodeResult {
  SaveCodecStatus status = SaveCodecStatus::Ok;
  std::string encodedText;
  StateHashValue savedStateHash = 0;
  std::string diagnosticSection;
  std::string diagnosticKey;
  std::uint32_t diagnosticLine = 0;
  std::uint32_t diagnosticOffset = 0;
  std::string diagnostic;
};

struct SaveDecodeResult {
  SaveCodecStatus status = SaveCodecStatus::Ok;
  SaveEnvelope envelope;
  std::string diagnosticSection;
  std::string diagnosticKey;
  std::uint32_t diagnosticLine = 0;
  std::uint32_t diagnosticOffset = 0;
  std::size_t duplicateOrMissingIndex = 0;
  std::string diagnostic;
};

SaveEncodeResult encodeSaveEnvelope(const SaveEnvelope& envelope);
SaveDecodeResult decodeSaveEnvelope(std::string_view bytes);

}  // namespace iggy3d
