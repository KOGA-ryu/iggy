#include "EditorToolWheelPreferences.hpp"

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::string_view kToolWheelFileHeader =
    "iggy3d_creative_tool_wheel 1";

[[nodiscard]] std::optional<std::size_t> catalogToolEntryIndexForKind(
    const cr::CreativeCatalogState& catalog,
    cr::CreativeHeldItemKind kind) noexcept {
  for (std::size_t index = 0; index < catalog.entries.size(); ++index) {
    if (catalog.entries[index].category ==
            cr::CreativeCatalogEntryCategory::Tool &&
        catalog.entries[index].hotbarEntry.kind == kind) {
      return index;
    }
  }
  return std::nullopt;
}

}  // namespace

CreativeEditorToolWheelPersistenceReceipt loadCreativeEditorToolWheel(
    cr::CreativeToolWheelState& wheel,
    const cr::CreativeCatalogState& catalog,
    const std::filesystem::path& path) {
  CreativeEditorToolWheelPersistenceReceipt receipt;
  std::ifstream input(path);
  if (!input.is_open()) {
    return receipt;
  }
  std::string header;
  std::getline(input, header);
  if (header != kToolWheelFileHeader) {
    receipt.status = CreativeEditorToolWheelPersistenceStatus::Invalid;
    return receipt;
  }

  std::string line;
  if (!std::getline(input, line)) {
    receipt.status = CreativeEditorToolWheelPersistenceStatus::Invalid;
    return receipt;
  }
  std::istringstream countRow(line);
  std::string countLabel;
  std::size_t entryCount = 0U;
  std::string extraToken;
  if (!(countRow >> countLabel >> entryCount) ||
      (countRow >> extraToken) || countLabel != "entry_count" ||
      entryCount > cr::kCreativeToolWheelCapacity) {
    receipt.status = CreativeEditorToolWheelPersistenceStatus::Invalid;
    return receipt;
  }

  cr::CreativeToolWheelState candidate;
  candidate.entryCount = entryCount;
  for (std::size_t slot = 0; slot < entryCount; ++slot) {
    if (!std::getline(input, line)) {
      receipt.status = CreativeEditorToolWheelPersistenceStatus::Invalid;
      return receipt;
    }
    std::istringstream slotRow(line);
    std::string slotLabel;
    std::size_t parsedSlot = cr::kCreativeToolWheelCapacity;
    std::string kindLabel;
    std::string slotExtraToken;
    const bool parsed =
        static_cast<bool>(slotRow >> slotLabel >> parsedSlot >> kindLabel);
    const bool hasExtraToken = static_cast<bool>(slotRow >> slotExtraToken);
    cr::CreativeHeldItemKind kind = cr::CreativeHeldItemKind::Count;
    const std::optional<std::size_t> catalogIndex =
        cr::parseCreativeHeldItemKind(kindLabel, kind)
            ? catalogToolEntryIndexForKind(catalog, kind)
            : std::nullopt;
    if (!parsed || hasExtraToken || slotLabel != "slot" ||
        parsedSlot != slot || !catalogIndex.has_value()) {
      receipt.status = CreativeEditorToolWheelPersistenceStatus::Invalid;
      return receipt;
    }
    candidate.catalogEntryIndices[slot] = *catalogIndex;
  }
  while (std::getline(input, line)) {
    if (!line.empty()) {
      receipt.status = CreativeEditorToolWheelPersistenceStatus::Invalid;
      return receipt;
    }
  }
  if (!input.eof() || !cr::isValidCreativeToolWheel(candidate, catalog)) {
    receipt.status = CreativeEditorToolWheelPersistenceStatus::Invalid;
    return receipt;
  }
  wheel = candidate;
  receipt.status = CreativeEditorToolWheelPersistenceStatus::Loaded;
  receipt.entryCount = wheel.entryCount;
  receipt.accepted = true;
  return receipt;
}

CreativeEditorToolWheelPersistenceReceipt saveCreativeEditorToolWheel(
    const cr::CreativeToolWheelState& wheel,
    const cr::CreativeCatalogState& catalog,
    const std::filesystem::path& path) {
  CreativeEditorToolWheelPersistenceReceipt receipt;
  receipt.status = CreativeEditorToolWheelPersistenceStatus::IoError;
  if (!cr::isValidCreativeToolWheel(wheel, catalog)) {
    receipt.status = CreativeEditorToolWheelPersistenceStatus::Invalid;
    return receipt;
  }
  std::error_code error;
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
      return receipt;
    }
  }
  const std::filesystem::path temporary = path.string() + ".tmp";
  std::ofstream output(temporary, std::ios::trunc);
  if (!output.is_open()) {
    return receipt;
  }
  output << kToolWheelFileHeader << '\n';
  output << "entry_count " << wheel.entryCount << '\n';
  for (std::size_t slot = 0; slot < wheel.entryCount; ++slot) {
    const std::size_t catalogIndex = wheel.catalogEntryIndices[slot];
    output << "slot " << slot << ' '
           << cr::toString(catalog.entries[catalogIndex].hotbarEntry.kind)
           << '\n';
  }
  output.close();
  if (!output) {
    std::filesystem::remove(temporary, error);
    return receipt;
  }
  std::filesystem::rename(temporary, path, error);
  if (error) {
    error.clear();
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(temporary, path, error);
  }
  if (error) {
    std::filesystem::remove(temporary, error);
    return receipt;
  }
  receipt.status = CreativeEditorToolWheelPersistenceStatus::Saved;
  receipt.entryCount = wheel.entryCount;
  receipt.accepted = true;
  return receipt;
}

}  // namespace iggy3d_creative_app
