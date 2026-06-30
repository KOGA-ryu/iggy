#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace iggy3d {

enum class SaveSlotCompatibility {
  Compatible,
  IncompatiblePackage,
  IncompatibleScenario,
  DecodeFailed,
  LoadFailed,
  Unknown,
};

struct SaveSlotPreview {
  std::string id;
  std::filesystem::path path;
  std::string packageId = "none";
  std::string scenarioId = "none";
  std::uint64_t currentTick = 0;
  std::string savedStateHashHex = "none";
  std::uint64_t authoredFloorCount = 0;
  std::uint64_t authoredWallCount = 0;
  std::uint64_t authoredMarkerCount = 0;
  SaveSlotCompatibility compatibility = SaveSlotCompatibility::Unknown;
  bool enabled = false;
  bool corrupt = false;
  std::string reason = "unknown";
  std::string displayTitle;
  std::string timestampLabel = "unknown";
  std::filesystem::path snapshotPath;
  bool snapshotAvailable = false;
  bool snapshotFallback = true;
  std::string snapshotStatus = "missing";
};

struct SaveSlotList {
  std::vector<SaveSlotPreview> slots;
  std::uint64_t compatibleCount = 0;
  std::uint64_t corruptCount = 0;
};

std::string_view saveSlotCompatibilityName(SaveSlotCompatibility compatibility);

}  // namespace iggy3d
