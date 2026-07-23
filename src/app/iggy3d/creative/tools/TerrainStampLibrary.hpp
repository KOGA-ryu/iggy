#pragma once

#include "app/iggy3d/creative/tools/TerrainStamp.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::uint32_t kCreativeTerrainStampAssetCodecVersion = 1U;
inline constexpr std::size_t kCreativeTerrainStampLibraryCapacity = 64U;
inline constexpr std::size_t kCreativeTerrainStampThumbnailEdge = 16U;
inline constexpr std::size_t kCreativeTerrainStampThumbnailPixelCapacity =
    kCreativeTerrainStampThumbnailEdge * kCreativeTerrainStampThumbnailEdge;

enum class CreativeTerrainStampLibraryMutationStatus : std::uint8_t {
  NotRequested,
  InvalidStamp,
  CapacityExceeded,
  Conflict,
  NotFound,
  NoChange,
  Applied,
};

enum class CreativeTerrainStampSourceStatus : std::uint8_t {
  Available,
  Missing,
  VersionMismatch,
  ContentMismatch,
  Incompatible,
};

enum class CreativeTerrainStampAssetCodecStatus : std::uint8_t {
  NotRequested,
  InvalidStamp,
  InvalidBytes,
  UnsupportedVersion,
  CapacityExceeded,
  Ready,
};

struct CreativeTerrainStampThumbnailPixel {
  std::uint8_t height = 0U;
  CreativeTerrainMaterial material = CreativeTerrainMaterial::Grass;
  bool present = false;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainStampThumbnailPixel,
      CreativeTerrainStampThumbnailPixel) noexcept = default;
};

struct CreativeTerrainStampThumbnail {
  std::uint16_t sourceWidthCells = 0U;
  std::uint16_t sourceDepthCells = 0U;
  std::uint16_t minimumHeightCells = 0U;
  std::uint16_t maximumHeightCells = 0U;
  std::array<CreativeTerrainStampThumbnailPixel,
             kCreativeTerrainStampThumbnailPixelCapacity>
      pixels{};
};

struct CreativeTerrainStampCatalogEntry {
  std::string assetId;
  std::string label;
  std::uint64_t assetVersion = 0U;
  std::uint64_t contentSignature = 0U;
  std::uint16_t widthCells = 0U;
  std::uint16_t depthCells = 0U;
  std::uint64_t presentCellCount = 0U;
  bool compatible = false;
  CreativeTerrainStampThumbnail thumbnail{};
};

struct CreativeTerrainStampLibrary {
  std::vector<CreativeTerrainStamp> stamps;
  std::string selectedAssetId;
};

struct CreativeTerrainStampLibraryMutationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTerrainStampLibraryMutationStatus status =
      CreativeTerrainStampLibraryMutationStatus::NotRequested;
  std::size_t index = 0U;
  std::string_view reasonCode =
      "creative_terrain_stamp_library_mutation_not_requested";
};

struct CreativeTerrainStampAssetEncodeResult {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainStampAssetCodecStatus status =
      CreativeTerrainStampAssetCodecStatus::NotRequested;
  std::vector<std::uint8_t> bytes;
  std::string_view reasonCode =
      "creative_terrain_stamp_asset_encode_not_requested";
};

struct CreativeTerrainStampAssetDecodeResult {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainStampAssetCodecStatus status =
      CreativeTerrainStampAssetCodecStatus::NotRequested;
  CreativeTerrainStamp stamp;
  std::string_view reasonCode =
      "creative_terrain_stamp_asset_decode_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeTerrainStampLibraryMutationStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainStampSourceStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTerrainStampAssetCodecStatus status) noexcept;

[[nodiscard]] const CreativeTerrainStamp* findCreativeTerrainStamp(
    const CreativeTerrainStampLibrary& library,
    std::string_view assetId) noexcept;
[[nodiscard]] CreativeTerrainStamp* findCreativeTerrainStamp(
    CreativeTerrainStampLibrary& library,
    std::string_view assetId) noexcept;
[[nodiscard]] const CreativeTerrainStamp* selectedCreativeTerrainStamp(
    const CreativeTerrainStampLibrary& library) noexcept;
[[nodiscard]] CreativeTerrainStampSourceStatus
creativeTerrainStampSourceStatus(
    const CreativeTerrainStampLibrary& library,
    const CreativeTerrainStampRecipe& recipe) noexcept;

[[nodiscard]] CreativeTerrainStampThumbnail buildCreativeTerrainStampThumbnail(
    const CreativeTerrainStamp& stamp) noexcept;
[[nodiscard]] std::vector<CreativeTerrainStampCatalogEntry>
buildCreativeTerrainStampCatalog(
    const CreativeTerrainStampLibrary& library);

[[nodiscard]] CreativeTerrainStampLibraryMutationReceipt
installCreativeTerrainStamp(CreativeTerrainStampLibrary& library,
                            const CreativeTerrainStamp& stamp,
                            bool replaceExisting = false);
[[nodiscard]] CreativeTerrainStampLibraryMutationReceipt
removeCreativeTerrainStamp(CreativeTerrainStampLibrary& library,
                           std::string_view assetId);
[[nodiscard]] CreativeTerrainStampLibraryMutationReceipt
selectCreativeTerrainStamp(CreativeTerrainStampLibrary& library,
                           std::string_view assetId);
[[nodiscard]] CreativeTerrainStampLibraryMutationReceipt
repairCreativeTerrainStampSource(
    CreativeTerrainStampLibrary& library,
    const CreativeTerrainStampRecipe& embeddedRecipe,
    bool replaceExisting);

[[nodiscard]] CreativeTerrainStampAssetEncodeResult encodeCreativeTerrainStamp(
    const CreativeTerrainStamp& stamp);
[[nodiscard]] CreativeTerrainStampAssetDecodeResult decodeCreativeTerrainStamp(
    std::span<const std::uint8_t> bytes);

}  // namespace iggy3d::creative
