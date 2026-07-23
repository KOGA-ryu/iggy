#include "app/iggy3d/creative/tools/TerrainStampLibrary.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <type_traits>
#include <utility>

namespace iggy3d::creative {
namespace {

constexpr std::array<std::uint8_t, 8U> kAssetMagic{
    'I', 'G', 'G', 'Y', 'T', 'S', '0', '1'};

void setStatus(CreativeTerrainStampLibraryMutationReceipt& receipt,
               CreativeTerrainStampLibraryMutationStatus status,
               std::string_view reasonCode) noexcept {
  receipt.status = status;
  receipt.reasonCode = reasonCode;
}

template <typename Integer>
void appendUnsigned(std::vector<std::uint8_t>& bytes, Integer value) {
  static_assert(std::is_unsigned_v<Integer>);
  for (std::size_t index = 0U; index < sizeof(Integer); ++index) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    value >>= 8U;
  }
}

void appendSigned32(std::vector<std::uint8_t>& bytes, std::int32_t value) {
  appendUnsigned(bytes, static_cast<std::uint32_t>(value));
}

void appendString(std::vector<std::uint8_t>& bytes, std::string_view value) {
  appendUnsigned(bytes, static_cast<std::uint16_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

class ByteReader {
 public:
  explicit ByteReader(std::span<const std::uint8_t> bytes) noexcept
      : bytes_(bytes) {}

  template <typename Integer>
  [[nodiscard]] bool readUnsigned(Integer& output) noexcept {
    static_assert(std::is_unsigned_v<Integer>);
    if (remaining() < sizeof(Integer)) {
      return false;
    }
    output = 0U;
    for (std::size_t index = 0U; index < sizeof(Integer); ++index) {
      output |= static_cast<Integer>(bytes_[position_++]) << (index * 8U);
    }
    return true;
  }

  [[nodiscard]] bool readSigned32(std::int32_t& output) noexcept {
    std::uint32_t encoded = 0U;
    if (!readUnsigned(encoded)) {
      return false;
    }
    output = static_cast<std::int32_t>(encoded);
    return true;
  }

  [[nodiscard]] bool readString(std::string& output,
                                std::size_t capacity) {
    std::uint16_t size = 0U;
    if (!readUnsigned(size) || size == 0U || size > capacity ||
        remaining() < size) {
      return false;
    }
    output.assign(reinterpret_cast<const char*>(bytes_.data() + position_),
                  size);
    position_ += size;
    return true;
  }

  [[nodiscard]] bool readMagic() noexcept {
    if (remaining() < kAssetMagic.size() ||
        !std::equal(kAssetMagic.begin(), kAssetMagic.end(),
                    bytes_.begin() + static_cast<std::ptrdiff_t>(position_))) {
      return false;
    }
    position_ += kAssetMagic.size();
    return true;
  }

  [[nodiscard]] std::size_t remaining() const noexcept {
    return bytes_.size() - position_;
  }

 private:
  std::span<const std::uint8_t> bytes_;
  std::size_t position_ = 0U;
};

[[nodiscard]] std::size_t stampIndex(
    const CreativeTerrainStampLibrary& library,
    std::string_view assetId) noexcept {
  const auto found = std::find_if(
      library.stamps.begin(), library.stamps.end(),
      [assetId](const CreativeTerrainStamp& stamp) {
        return stamp.assetId == assetId;
      });
  return static_cast<std::size_t>(found - library.stamps.begin());
}

[[nodiscard]] std::uint64_t presentCellCount(
    const CreativeTerrainStamp& stamp) noexcept {
  return static_cast<std::uint64_t>(std::count_if(
      stamp.heights.begin(), stamp.heights.end(),
      [](std::uint16_t height) { return height != 0U; }));
}

}  // namespace

std::string_view toString(
    CreativeTerrainStampLibraryMutationStatus status) noexcept {
  switch (status) {
    case CreativeTerrainStampLibraryMutationStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainStampLibraryMutationStatus::InvalidStamp:
      return "InvalidStamp";
    case CreativeTerrainStampLibraryMutationStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainStampLibraryMutationStatus::Conflict: return "Conflict";
    case CreativeTerrainStampLibraryMutationStatus::NotFound: return "NotFound";
    case CreativeTerrainStampLibraryMutationStatus::NoChange: return "NoChange";
    case CreativeTerrainStampLibraryMutationStatus::Applied: return "Applied";
  }
  return "Invalid";
}

std::string_view toString(CreativeTerrainStampSourceStatus status) noexcept {
  switch (status) {
    case CreativeTerrainStampSourceStatus::Available: return "Available";
    case CreativeTerrainStampSourceStatus::Missing: return "Missing";
    case CreativeTerrainStampSourceStatus::VersionMismatch:
      return "VersionMismatch";
    case CreativeTerrainStampSourceStatus::ContentMismatch:
      return "ContentMismatch";
    case CreativeTerrainStampSourceStatus::Incompatible: return "Incompatible";
  }
  return "Invalid";
}

std::string_view toString(CreativeTerrainStampAssetCodecStatus status) noexcept {
  switch (status) {
    case CreativeTerrainStampAssetCodecStatus::NotRequested:
      return "NotRequested";
    case CreativeTerrainStampAssetCodecStatus::InvalidStamp:
      return "InvalidStamp";
    case CreativeTerrainStampAssetCodecStatus::InvalidBytes:
      return "InvalidBytes";
    case CreativeTerrainStampAssetCodecStatus::UnsupportedVersion:
      return "UnsupportedVersion";
    case CreativeTerrainStampAssetCodecStatus::CapacityExceeded:
      return "CapacityExceeded";
    case CreativeTerrainStampAssetCodecStatus::Ready: return "Ready";
  }
  return "Invalid";
}

const CreativeTerrainStamp* findCreativeTerrainStamp(
    const CreativeTerrainStampLibrary& library,
    std::string_view assetId) noexcept {
  const std::size_t index = stampIndex(library, assetId);
  return index < library.stamps.size() ? &library.stamps[index] : nullptr;
}

CreativeTerrainStamp* findCreativeTerrainStamp(
    CreativeTerrainStampLibrary& library,
    std::string_view assetId) noexcept {
  const std::size_t index = stampIndex(library, assetId);
  return index < library.stamps.size() ? &library.stamps[index] : nullptr;
}

const CreativeTerrainStamp* selectedCreativeTerrainStamp(
    const CreativeTerrainStampLibrary& library) noexcept {
  return findCreativeTerrainStamp(library, library.selectedAssetId);
}

CreativeTerrainStampSourceStatus creativeTerrainStampSourceStatus(
    const CreativeTerrainStampLibrary& library,
    const CreativeTerrainStampRecipe& recipe) noexcept {
  if (!isValidCreativeTerrainStampRecipe(recipe)) {
    return CreativeTerrainStampSourceStatus::Incompatible;
  }
  const CreativeTerrainStamp* source =
      findCreativeTerrainStamp(library, recipe.stamp.assetId);
  if (source == nullptr) {
    return CreativeTerrainStampSourceStatus::Missing;
  }
  if (source->assetVersion != recipe.stamp.assetVersion) {
    return CreativeTerrainStampSourceStatus::VersionMismatch;
  }
  if (source->contentSignature != recipe.stamp.contentSignature) {
    return CreativeTerrainStampSourceStatus::ContentMismatch;
  }
  return CreativeTerrainStampSourceStatus::Available;
}

CreativeTerrainStampThumbnail buildCreativeTerrainStampThumbnail(
    const CreativeTerrainStamp& stamp) noexcept {
  CreativeTerrainStampThumbnail thumbnail;
  if (!isValidCreativeTerrainStamp(stamp)) {
    return thumbnail;
  }
  thumbnail.sourceWidthCells = stamp.widthCells;
  thumbnail.sourceDepthCells = stamp.depthCells;
  thumbnail.minimumHeightCells = stamp.minimumHeightCells;
  thumbnail.maximumHeightCells = stamp.minimumHeightCells;
  for (const std::uint16_t height : stamp.heights) {
    thumbnail.maximumHeightCells =
        height == 0U ? thumbnail.maximumHeightCells
                     : std::max(thumbnail.maximumHeightCells, height);
  }
  const std::uint16_t range = static_cast<std::uint16_t>(
      thumbnail.maximumHeightCells - thumbnail.minimumHeightCells);
  for (std::size_t z = 0U; z < kCreativeTerrainStampThumbnailEdge; ++z) {
    const std::size_t sourceMinimumZ =
        z * static_cast<std::size_t>(stamp.depthCells) /
        kCreativeTerrainStampThumbnailEdge;
    const std::size_t sourceMaximumZ = std::min(
        static_cast<std::size_t>(stamp.depthCells),
        std::max(sourceMinimumZ + 1U,
                 (z + 1U) * static_cast<std::size_t>(stamp.depthCells) /
                     kCreativeTerrainStampThumbnailEdge));
    for (std::size_t x = 0U; x < kCreativeTerrainStampThumbnailEdge; ++x) {
      const std::size_t sourceMinimumX =
          x * static_cast<std::size_t>(stamp.widthCells) /
          kCreativeTerrainStampThumbnailEdge;
      const std::size_t sourceMaximumX = std::min(
          static_cast<std::size_t>(stamp.widthCells),
          std::max(sourceMinimumX + 1U,
                   (x + 1U) * static_cast<std::size_t>(stamp.widthCells) /
                       kCreativeTerrainStampThumbnailEdge));
      CreativeTerrainStampThumbnailPixel& pixel =
          thumbnail.pixels[z * kCreativeTerrainStampThumbnailEdge + x];
      std::size_t representativeIndex = stamp.heights.size();
      std::uint16_t representativeHeight = 0U;
      for (std::size_t sourceZ = sourceMinimumZ;
           sourceZ < sourceMaximumZ; ++sourceZ) {
        for (std::size_t sourceX = sourceMinimumX;
             sourceX < sourceMaximumX; ++sourceX) {
          const std::size_t sourceIndex =
              sourceZ * static_cast<std::size_t>(stamp.widthCells) + sourceX;
          if (stamp.heights[sourceIndex] > representativeHeight) {
            representativeHeight = stamp.heights[sourceIndex];
            representativeIndex = sourceIndex;
          }
        }
      }
      if (representativeIndex >= stamp.heights.size()) {
        continue;
      }
      pixel.present = true;
      pixel.height = range == 0U
                         ? 255U
                         : static_cast<std::uint8_t>(
                               32U + 223U *
                                          (representativeHeight -
                                           thumbnail.minimumHeightCells) /
                                          range);
      pixel.material =
          dominantCreativeTerrainMaterial(stamp.materials[representativeIndex]);
    }
  }
  return thumbnail;
}

std::vector<CreativeTerrainStampCatalogEntry> buildCreativeTerrainStampCatalog(
    const CreativeTerrainStampLibrary& library) {
  std::vector<CreativeTerrainStampCatalogEntry> entries;
  entries.reserve(library.stamps.size());
  for (const CreativeTerrainStamp& stamp : library.stamps) {
    CreativeTerrainStampCatalogEntry entry;
    entry.assetId = stamp.assetId;
    entry.label = stamp.label;
    entry.assetVersion = stamp.assetVersion;
    entry.contentSignature = stamp.contentSignature;
    entry.widthCells = stamp.widthCells;
    entry.depthCells = stamp.depthCells;
    entry.presentCellCount = presentCellCount(stamp);
    entry.compatible = isValidCreativeTerrainStamp(stamp);
    entry.thumbnail = buildCreativeTerrainStampThumbnail(stamp);
    entries.push_back(std::move(entry));
  }
  return entries;
}

CreativeTerrainStampLibraryMutationReceipt installCreativeTerrainStamp(
    CreativeTerrainStampLibrary& library,
    const CreativeTerrainStamp& stamp,
    bool replaceExisting) {
  CreativeTerrainStampLibraryMutationReceipt receipt;
  receipt.requested = true;
  if (!isValidCreativeTerrainStamp(stamp)) {
    setStatus(receipt, CreativeTerrainStampLibraryMutationStatus::InvalidStamp,
              "creative_terrain_stamp_library_stamp_invalid");
    return receipt;
  }
  const std::size_t index = stampIndex(library, stamp.assetId);
  receipt.index = index;
  if (index < library.stamps.size()) {
    if (library.stamps[index] == stamp) {
      receipt.accepted = true;
      setStatus(receipt, CreativeTerrainStampLibraryMutationStatus::NoChange,
                "creative_terrain_stamp_library_no_change");
      return receipt;
    }
    if (!replaceExisting) {
      setStatus(receipt, CreativeTerrainStampLibraryMutationStatus::Conflict,
                "creative_terrain_stamp_library_conflict");
      return receipt;
    }
    library.stamps[index] = stamp;
  } else {
    if (library.stamps.size() >= kCreativeTerrainStampLibraryCapacity) {
      setStatus(receipt,
                CreativeTerrainStampLibraryMutationStatus::CapacityExceeded,
                "creative_terrain_stamp_library_capacity_exceeded");
      return receipt;
    }
    receipt.index = library.stamps.size();
    library.stamps.push_back(stamp);
  }
  library.selectedAssetId = stamp.assetId;
  receipt.accepted = true;
  receipt.changed = true;
  setStatus(receipt, CreativeTerrainStampLibraryMutationStatus::Applied,
            "creative_terrain_stamp_library_installed");
  return receipt;
}

CreativeTerrainStampLibraryMutationReceipt removeCreativeTerrainStamp(
    CreativeTerrainStampLibrary& library,
    std::string_view assetId) {
  CreativeTerrainStampLibraryMutationReceipt receipt;
  receipt.requested = true;
  const std::size_t index = stampIndex(library, assetId);
  receipt.index = index;
  if (index >= library.stamps.size()) {
    setStatus(receipt, CreativeTerrainStampLibraryMutationStatus::NotFound,
              "creative_terrain_stamp_library_source_not_found");
    return receipt;
  }
  library.stamps.erase(library.stamps.begin() +
                       static_cast<std::ptrdiff_t>(index));
  if (library.selectedAssetId == assetId) {
    library.selectedAssetId =
        library.stamps.empty() ? std::string{} : library.stamps.front().assetId;
  }
  receipt.accepted = true;
  receipt.changed = true;
  setStatus(receipt, CreativeTerrainStampLibraryMutationStatus::Applied,
            "creative_terrain_stamp_library_removed");
  return receipt;
}

CreativeTerrainStampLibraryMutationReceipt selectCreativeTerrainStamp(
    CreativeTerrainStampLibrary& library,
    std::string_view assetId) {
  CreativeTerrainStampLibraryMutationReceipt receipt;
  receipt.requested = true;
  const std::size_t index = stampIndex(library, assetId);
  receipt.index = index;
  if (index >= library.stamps.size()) {
    setStatus(receipt, CreativeTerrainStampLibraryMutationStatus::NotFound,
              "creative_terrain_stamp_library_source_not_found");
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed = library.selectedAssetId != assetId;
  library.selectedAssetId = std::string(assetId);
  setStatus(receipt,
            receipt.changed ? CreativeTerrainStampLibraryMutationStatus::Applied
                            : CreativeTerrainStampLibraryMutationStatus::NoChange,
            receipt.changed ? "creative_terrain_stamp_library_selected"
                            : "creative_terrain_stamp_library_selection_unchanged");
  return receipt;
}

CreativeTerrainStampLibraryMutationReceipt repairCreativeTerrainStampSource(
    CreativeTerrainStampLibrary& library,
    const CreativeTerrainStampRecipe& embeddedRecipe,
    bool replaceExisting) {
  if (!isValidCreativeTerrainStampRecipe(embeddedRecipe)) {
    CreativeTerrainStampLibraryMutationReceipt receipt;
    receipt.requested = true;
    setStatus(receipt, CreativeTerrainStampLibraryMutationStatus::InvalidStamp,
              "creative_terrain_stamp_library_repair_recipe_invalid");
    return receipt;
  }
  return installCreativeTerrainStamp(library, embeddedRecipe.stamp,
                                     replaceExisting);
}

CreativeTerrainStampAssetEncodeResult encodeCreativeTerrainStamp(
    const CreativeTerrainStamp& stamp) {
  CreativeTerrainStampAssetEncodeResult result;
  result.requested = true;
  if (!isValidCreativeTerrainStamp(stamp)) {
    result.status = CreativeTerrainStampAssetCodecStatus::InvalidStamp;
    result.reasonCode = "creative_terrain_stamp_asset_encode_stamp_invalid";
    return result;
  }
  result.bytes.reserve(96U + stamp.assetId.size() + stamp.label.size() +
                       stamp.heights.size() * 6U);
  result.bytes.insert(result.bytes.end(), kAssetMagic.begin(), kAssetMagic.end());
  appendUnsigned(result.bytes, kCreativeTerrainStampAssetCodecVersion);
  appendUnsigned(result.bytes, stamp.version);
  appendString(result.bytes, stamp.assetId);
  appendString(result.bytes, stamp.label);
  appendUnsigned(result.bytes, stamp.assetVersion);
  appendUnsigned(result.bytes, stamp.sourceDocumentId);
  appendUnsigned(result.bytes, stamp.sourceRevision);
  appendUnsigned(result.bytes, stamp.contentSignature);
  appendSigned32(result.bytes, stamp.sourceMinimum.x);
  appendSigned32(result.bytes, stamp.sourceMinimum.z);
  appendUnsigned(result.bytes, stamp.widthCells);
  appendUnsigned(result.bytes, stamp.depthCells);
  appendUnsigned(result.bytes, stamp.minimumHeightCells);
  appendUnsigned(result.bytes,
                 static_cast<std::uint32_t>(stamp.heights.size()));
  for (const std::uint16_t height : stamp.heights) {
    appendUnsigned(result.bytes, height);
  }
  for (const CreativeTerrainMaterialWeights& weights : stamp.materials) {
    result.bytes.insert(result.bytes.end(), weights.begin(), weights.end());
  }
  result.accepted = true;
  result.status = CreativeTerrainStampAssetCodecStatus::Ready;
  result.reasonCode = "creative_terrain_stamp_asset_encoded";
  return result;
}

CreativeTerrainStampAssetDecodeResult decodeCreativeTerrainStamp(
    std::span<const std::uint8_t> bytes) {
  CreativeTerrainStampAssetDecodeResult result;
  result.requested = true;
  ByteReader reader(bytes);
  std::uint32_t codecVersion = 0U;
  std::uint32_t cellCount = 0U;
  if (!reader.readMagic() || !reader.readUnsigned(codecVersion)) {
    result.status = CreativeTerrainStampAssetCodecStatus::InvalidBytes;
    result.reasonCode = "creative_terrain_stamp_asset_bytes_invalid";
    return result;
  }
  if (codecVersion != kCreativeTerrainStampAssetCodecVersion) {
    result.status = CreativeTerrainStampAssetCodecStatus::UnsupportedVersion;
    result.reasonCode = "creative_terrain_stamp_asset_version_unsupported";
    return result;
  }
  CreativeTerrainStamp stamp;
  if (!reader.readUnsigned(stamp.version) ||
      !reader.readString(stamp.assetId,
                         kCreativeTerrainStampAssetIdCapacity) ||
      !reader.readString(stamp.label, kCreativeTerrainStampLabelCapacity) ||
      !reader.readUnsigned(stamp.assetVersion) ||
      !reader.readUnsigned(stamp.sourceDocumentId) ||
      !reader.readUnsigned(stamp.sourceRevision) ||
      !reader.readUnsigned(stamp.contentSignature) ||
      !reader.readSigned32(stamp.sourceMinimum.x) ||
      !reader.readSigned32(stamp.sourceMinimum.z) ||
      !reader.readUnsigned(stamp.widthCells) ||
      !reader.readUnsigned(stamp.depthCells) ||
      !reader.readUnsigned(stamp.minimumHeightCells) ||
      !reader.readUnsigned(cellCount)) {
    result.status = CreativeTerrainStampAssetCodecStatus::InvalidBytes;
    result.reasonCode = "creative_terrain_stamp_asset_bytes_invalid";
    return result;
  }
  if (cellCount == 0U || cellCount > kCreativeTerrainStampCellCapacity ||
      cellCount != static_cast<std::uint64_t>(stamp.widthCells) *
                       stamp.depthCells) {
    result.status = CreativeTerrainStampAssetCodecStatus::CapacityExceeded;
    result.reasonCode = "creative_terrain_stamp_asset_cell_count_invalid";
    return result;
  }
  constexpr std::size_t kEncodedMaterialBytes = kCreativeTerrainMaterialCount;
  const std::size_t requiredCellBytes =
      static_cast<std::size_t>(cellCount) *
      (sizeof(std::uint16_t) + kEncodedMaterialBytes);
  if (reader.remaining() != requiredCellBytes) {
    result.status = CreativeTerrainStampAssetCodecStatus::InvalidBytes;
    result.reasonCode = "creative_terrain_stamp_asset_size_invalid";
    return result;
  }
  stamp.heights.resize(cellCount);
  for (std::uint16_t& height : stamp.heights) {
    if (!reader.readUnsigned(height)) {
      result.status = CreativeTerrainStampAssetCodecStatus::InvalidBytes;
      result.reasonCode = "creative_terrain_stamp_asset_height_invalid";
      return result;
    }
  }
  stamp.materials.resize(cellCount);
  for (CreativeTerrainMaterialWeights& weights : stamp.materials) {
    for (std::uint8_t& weight : weights) {
      if (!reader.readUnsigned(weight)) {
        result.status = CreativeTerrainStampAssetCodecStatus::InvalidBytes;
        result.reasonCode = "creative_terrain_stamp_asset_material_invalid";
        return result;
      }
    }
  }
  if (reader.remaining() != 0U || !isValidCreativeTerrainStamp(stamp)) {
    result.status = CreativeTerrainStampAssetCodecStatus::InvalidStamp;
    result.reasonCode = "creative_terrain_stamp_asset_stamp_invalid";
    return result;
  }
  result.stamp = std::move(stamp);
  result.accepted = true;
  result.status = CreativeTerrainStampAssetCodecStatus::Ready;
  result.reasonCode = "creative_terrain_stamp_asset_decoded";
  return result;
}

}  // namespace iggy3d::creative
