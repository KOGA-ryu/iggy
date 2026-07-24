#include "app/iggy3d/save/SaveBridgeInternal.hpp"

#include "runtime/save/SaveFileStore.hpp"

namespace iggy3d::save_bridge_internal {

ExistingSaveIdentity readExistingSaveIdentity(const std::filesystem::path& root,
                                              std::string_view idHint) {
  ExistingSaveIdentity existing;
  if (!isValidSaveFileId(idHint)) {
    return existing;
  }
  const std::filesystem::path path = saveFilePathForId(root, idHint);
  std::error_code error;
  if (!std::filesystem::exists(path, error) || error) {
    return existing;
  }
  const SaveFileReadResult read = readSaveFile(path);
  if (!read.ok) {
    return existing;
  }
  const SaveEnvelope& envelope = read.envelope;
  existing.found = true;
  existing.worldId = envelope.metadata.worldId;
  existing.worldTitle = envelope.metadata.worldTitle;
  existing.saveTitle = envelope.metadata.saveTitle;
  existing.saveType = envelope.metadata.saveType;
  existing.createdAtUtc = envelope.metadata.createdAtUtc;
  existing.savedAtUtc = envelope.metadata.savedAtUtc;
  return existing;
}

std::string preferNonEmpty(const std::string& primary,
                           const std::string& fallback) {
  return primary.empty() ? fallback : primary;
}

}  // namespace iggy3d::save_bridge_internal
