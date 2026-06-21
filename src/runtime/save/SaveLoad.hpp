#pragma once

#include <string>
#include <string_view>

#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveCompatibility.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

enum class SaveLoadStatus : std::uint8_t {
  Ok,
  InvalidSourceState,
  EncodeFailed,
  DecodeFailed,
  CompatibilityFailed,
  InvalidEnvelope,
  InvalidReference,
  InvalidCommandLog,
  HashMismatch,
  ReplacementFailed,
};

struct SaveStateResult {
  SaveLoadStatus status = SaveLoadStatus::InvalidSourceState;
  SaveEnvelope envelope;
  std::string encodedSaveText;
  StateHashValue savedStateHash = 0;
  SaveCodecStatus codecStatus = SaveCodecStatus::Ok;
  std::string diagnostic;
  std::string diagnosticSection;
  std::string diagnosticKey;
};

struct LoadStateResult {
  SaveLoadStatus status = SaveLoadStatus::InvalidEnvelope;
  StateHashValue previousHash = 0;
  StateHashValue loadedHash = 0;
  SaveCompatibilityStatus compatibilityStatus = SaveCompatibilityStatus::Compatible;
  SaveCodecStatus codecStatus = SaveCodecStatus::Ok;
  SessionLoadStatus sessionLoadStatus = SessionLoadStatus::Ok;
  std::string diagnostic;
};

SaveStateResult saveSessionState(const SessionState& state);
SaveStateResult saveSessionStateEncoded(const SessionState& state);

LoadStateResult loadEnvelopeIntoSession(
    Session& session,
    const SaveEnvelope& envelope,
    const SaveCompatibilityRequest& request);

LoadStateResult loadEncodedSaveIntoSession(
    Session& session,
    std::string_view encodedSave,
    const SaveCompatibilityRequest& request);

}  // namespace iggy3d
