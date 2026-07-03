#include "runtime/save/SaveCompatibility.hpp"

#include "runtime/replay/StateHash.hpp"

namespace iggy3d {

namespace {

SaveCompatibilityResult failure(SaveCompatibilityStatus status,
                                std::string field,
                                std::string expected,
                                std::string actual) {
  return {status, std::move(field), std::move(expected), std::move(actual)};
}

bool isLowerHex16(const std::string& value) {
  if (value.size() != 16U) {
    return false;
  }
  for (char c : value) {
    const bool digit = c >= '0' && c <= '9';
    const bool lower = c >= 'a' && c <= 'f';
    if (!digit && !lower) {
      return false;
    }
  }
  return true;
}

}  // namespace

SaveCompatibilityResult checkSaveCompatibility(const SaveCompatibilityRequest& request) {
  const SaveEnvelope& envelope = request.envelope;
  if (envelope.metadata.schemaVersion > request.currentSchemaVersion ||
      envelope.metadata.minimumReadableSchemaVersion > request.currentSchemaVersion ||
      envelope.metadata.schemaVersion < request.minimumReadableSchemaVersion) {
    return failure(SaveCompatibilityStatus::UnsupportedSchemaVersion, "metadata.schemaVersion",
                   std::to_string(request.minimumReadableSchemaVersion) + ".." +
                       std::to_string(request.currentSchemaVersion),
                   std::to_string(envelope.metadata.schemaVersion));
  }
  if (envelope.metadata.runtimeSaveVersion != request.currentRuntimeSaveVersion) {
    return failure(SaveCompatibilityStatus::UnsupportedRuntimeVersion,
                   "metadata.runtimeSaveVersion",
                   std::to_string(request.currentRuntimeSaveVersion),
                   std::to_string(envelope.metadata.runtimeSaveVersion));
  }
  if (!request.expectedPackageId.empty() && envelope.metadata.packageId != request.expectedPackageId) {
    return failure(SaveCompatibilityStatus::PackageMismatch, "metadata.packageId",
                   request.expectedPackageId, envelope.metadata.packageId);
  }
  if (!request.expectedScenarioId.empty() && envelope.metadata.scenarioId != request.expectedScenarioId) {
    return failure(SaveCompatibilityStatus::ScenarioMismatch, "metadata.scenarioId",
                   request.expectedScenarioId, envelope.metadata.scenarioId);
  }
  if (!isLowerHex16(envelope.metadata.savedStateHashHex) ||
      envelope.metadata.savedStateHashHex != formatStateHash(envelope.metadata.savedStateHash)) {
    return failure(SaveCompatibilityStatus::HashMetadataMalformed,
                   "metadata.savedStateHashHex",
                   formatStateHash(envelope.metadata.savedStateHash),
                   envelope.metadata.savedStateHashHex);
  }
  return {};
}

}  // namespace iggy3d
