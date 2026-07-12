#pragma once

#include "app/iggy3d/creative/input/Catalog.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace iggy3d_creative_app {

enum class CreativeEditorToolWheelPersistenceStatus : std::uint8_t {
  Missing,
  Loaded,
  Saved,
  Invalid,
  IoError,
};

struct CreativeEditorToolWheelPersistenceReceipt {
  CreativeEditorToolWheelPersistenceStatus status =
      CreativeEditorToolWheelPersistenceStatus::Missing;
  std::size_t entryCount = 0;
  bool accepted = false;
};

[[nodiscard]] CreativeEditorToolWheelPersistenceReceipt
loadCreativeEditorToolWheel(
    iggy3d::creative::CreativeToolWheelState& wheel,
    const iggy3d::creative::CreativeCatalogState& catalog,
    const std::filesystem::path& path);
[[nodiscard]] CreativeEditorToolWheelPersistenceReceipt
saveCreativeEditorToolWheel(
    const iggy3d::creative::CreativeToolWheelState& wheel,
    const iggy3d::creative::CreativeCatalogState& catalog,
    const std::filesystem::path& path);

}  // namespace iggy3d_creative_app
