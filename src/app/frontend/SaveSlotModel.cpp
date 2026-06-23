#include "app/frontend/SaveSlotModel.hpp"

#include "runtime/save/SaveCodec.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace iggy3d {
namespace {

constexpr std::string_view kSaveFileExtension = ".iggy3d.save";

bool hasSaveFileExtension(const std::filesystem::path& path) {
  const std::string filename = path.filename().string();
  return filename.size() > kSaveFileExtension.size() &&
         filename.ends_with(kSaveFileExtension);
}

std::string idFromPath(const std::filesystem::path& path) {
  std::string filename = path.filename().string();
  if (filename.size() > kSaveFileExtension.size() &&
      filename.ends_with(kSaveFileExtension)) {
    filename.erase(filename.size() - kSaveFileExtension.size());
  }
  return filename.empty() ? "save" : filename;
}

std::string readWholeFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    return {};
  }
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
}

SaveSlotCompatibility compatibilityFor(std::string_view packageId,
                                       std::string_view scenarioId,
                                       std::string_view expectedPackageId,
                                       std::string_view expectedScenarioId) {
  if (!expectedPackageId.empty() && packageId != expectedPackageId) {
    return SaveSlotCompatibility::IncompatiblePackage;
  }
  if (!expectedScenarioId.empty() && scenarioId != expectedScenarioId) {
    return SaveSlotCompatibility::IncompatibleScenario;
  }
  return SaveSlotCompatibility::Compatible;
}

}  // namespace

std::string_view saveSlotCompatibilityName(SaveSlotCompatibility compatibility) {
  switch (compatibility) {
    case SaveSlotCompatibility::Compatible:
      return "compatible";
    case SaveSlotCompatibility::IncompatiblePackage:
      return "incompatible_package";
    case SaveSlotCompatibility::IncompatibleScenario:
      return "incompatible_scenario";
    case SaveSlotCompatibility::DecodeFailed:
      return "decode_failed";
    case SaveSlotCompatibility::LoadFailed:
      return "load_failed";
    case SaveSlotCompatibility::Unknown:
      return "unknown";
  }
  return "unknown";
}

SaveSlotPreview previewFromSaveFileRecord(const SaveFileRecord& record,
                                          std::string_view expectedPackageId,
                                          std::string_view expectedScenarioId) {
  SaveSlotPreview preview;
  preview.id = record.id;
  preview.path = record.path;
  preview.packageId = record.packageId.empty() ? "none" : record.packageId;
  preview.scenarioId = record.scenarioId.empty() ? "none" : record.scenarioId;
  preview.currentTick = record.currentTick;
  preview.savedStateHashHex =
      record.savedStateHashHex.empty() ? "none" : record.savedStateHashHex;
  preview.compatibility =
      compatibilityFor(record.packageId, record.scenarioId, expectedPackageId,
                       expectedScenarioId);
  preview.enabled = preview.compatibility == SaveSlotCompatibility::Compatible;
  preview.reason = std::string(saveSlotCompatibilityName(preview.compatibility));
  return preview;
}

SaveSlotList buildSaveSlotList(const std::filesystem::path& root,
                               std::string_view expectedPackageId,
                               std::string_view expectedScenarioId) {
  SaveSlotList list;
  std::error_code error;
  if (!std::filesystem::exists(root, error) ||
      !std::filesystem::is_directory(root, error)) {
    return list;
  }

  std::vector<std::filesystem::path> paths;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(root, error)) {
    if (error || !entry.is_regular_file(error) || !hasSaveFileExtension(entry.path())) {
      continue;
    }
    paths.push_back(entry.path());
  }
  std::sort(paths.begin(), paths.end());

  for (const std::filesystem::path& path : paths) {
    SaveSlotPreview preview;
    preview.id = idFromPath(path);
    preview.path = path;
    const std::string text = readWholeFile(path);
    if (text.empty()) {
      preview.compatibility = SaveSlotCompatibility::DecodeFailed;
      preview.corrupt = true;
      preview.reason = "save_file_read_failed";
      ++list.corruptCount;
      list.slots.push_back(std::move(preview));
      continue;
    }
    const SaveDecodeResult decoded = decodeSaveEnvelope(text);
    if (decoded.status != SaveCodecStatus::Ok) {
      preview.compatibility = SaveSlotCompatibility::DecodeFailed;
      preview.corrupt = true;
      preview.reason = "save_file_decode_failed";
      ++list.corruptCount;
      list.slots.push_back(std::move(preview));
      continue;
    }
    SaveFileRecord record;
    record.id = idFromPath(path);
    record.path = path;
    record.packageId = decoded.envelope.metadata.packageId;
    record.scenarioId = decoded.envelope.metadata.scenarioId;
    record.currentTick = decoded.envelope.session.currentTick;
    record.nextCommandId = decoded.envelope.session.nextCommandId;
    record.savedStateHash = decoded.envelope.metadata.savedStateHash;
    record.savedStateHashHex = decoded.envelope.metadata.savedStateHashHex;
    preview = previewFromSaveFileRecord(record, expectedPackageId, expectedScenarioId);
    preview.authoredFloorCount =
        static_cast<std::uint64_t>(decoded.envelope.authoredRoom.floors.size());
    preview.authoredWallCount =
        static_cast<std::uint64_t>(decoded.envelope.authoredRoom.walls.size());
    if (preview.enabled) {
      ++list.compatibleCount;
    }
    list.slots.push_back(std::move(preview));
  }
  return list;
}

}  // namespace iggy3d
