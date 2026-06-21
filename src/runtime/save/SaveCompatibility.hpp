#pragma once

#include <cstdint>
#include <string>

#include "runtime/save/SaveEnvelope.hpp"

namespace iggy3d {

enum class SaveCompatibilityStatus : std::uint8_t {
  Compatible,
  UnsupportedSchemaVersion,
  UnsupportedRuntimeVersion,
  PackageMismatch,
  ScenarioMismatch,
  HashMetadataMalformed,
  MalformedEnvelope,
};

struct SaveCompatibilityRequest {
  const SaveEnvelope& envelope;
  std::string expectedPackageId;
  std::string expectedScenarioId;
  std::uint32_t currentSchemaVersion = kSaveSchemaVersion;
  std::uint32_t minimumReadableSchemaVersion = kMinimumReadableSaveSchemaVersion;
  std::uint32_t currentRuntimeSaveVersion = kRuntimeSaveVersion;
};

struct SaveCompatibilityResult {
  SaveCompatibilityStatus status = SaveCompatibilityStatus::Compatible;
  std::string fieldName;
  std::string expectedValue;
  std::string actualValue;
};

SaveCompatibilityResult checkSaveCompatibility(const SaveCompatibilityRequest& request);

}  // namespace iggy3d
