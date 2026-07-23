#include "app/iggy3d/creative/tools/TerrainStampLibrary.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainStamp makeStamp(
    std::string assetId = "terrain_stamp_0001",
    std::string label = "Rock Shelf",
    std::uint64_t assetVersion = 3U) {
  cr::CreativeTerrainStamp stamp;
  stamp.assetId = std::move(assetId);
  stamp.label = std::move(label);
  stamp.assetVersion = assetVersion;
  stamp.sourceDocumentId = 17U;
  stamp.sourceRevision = 23U;
  stamp.sourceMinimum = {10, 20};
  stamp.widthCells = 4U;
  stamp.depthCells = 2U;
  stamp.minimumHeightCells = 2U;
  stamp.heights = {2U, 0U, 4U, 8U, 3U, 5U, 0U, 6U};
  stamp.materials.assign(
      stamp.heights.size(),
      cr::creativeTerrainMaterialSolidWeights(
          cr::CreativeTerrainMaterial::Grass));
  stamp.materials[3U] = cr::creativeTerrainMaterialSolidWeights(
      cr::CreativeTerrainMaterial::Stone);
  stamp.materials[5U] = cr::creativeTerrainMaterialSolidWeights(
      cr::CreativeTerrainMaterial::Dirt);
  stamp.contentSignature = cr::creativeTerrainStampContentSignature(stamp);
  return stamp;
}

cr::CreativeTerrainStamp variant(cr::CreativeTerrainStamp stamp,
                                 std::uint16_t firstHeight) {
  stamp.heights[0U] = firstHeight;
  stamp.minimumHeightCells = firstHeight;
  for (const std::uint16_t height : stamp.heights) {
    if (height != 0U) {
      stamp.minimumHeightCells =
          std::min(stamp.minimumHeightCells, height);
    }
  }
  stamp.contentSignature = cr::creativeTerrainStampContentSignature(stamp);
  return stamp;
}

bool catalogAndThumbnailRetainUsefulFacts() {
  cr::CreativeTerrainStampLibrary library;
  const cr::CreativeTerrainStamp stamp = makeStamp();
  const cr::CreativeTerrainStampLibraryMutationReceipt installed =
      cr::installCreativeTerrainStamp(library, stamp);
  const std::vector<cr::CreativeTerrainStampCatalogEntry> catalog =
      cr::buildCreativeTerrainStampCatalog(library);
  const cr::CreativeTerrainStampThumbnail direct =
      cr::buildCreativeTerrainStampThumbnail(stamp);

  const std::size_t holePixel = 4U;
  const std::size_t stonePixel = 12U;
  return expect(installed.accepted && installed.changed &&
                    library.selectedAssetId == stamp.assetId &&
                    catalog.size() == 1U &&
                    catalog[0U].assetId == stamp.assetId &&
                    catalog[0U].label == stamp.label &&
                    catalog[0U].assetVersion == stamp.assetVersion &&
                    catalog[0U].widthCells == 4U &&
                    catalog[0U].depthCells == 2U &&
                    catalog[0U].presentCellCount == 6U &&
                    catalog[0U].compatible,
                "catalog retains stable identity dimensions and compatibility") &&
         expect(direct.sourceWidthCells == 4U &&
                    direct.sourceDepthCells == 2U &&
                    direct.minimumHeightCells == 2U &&
                    direct.maximumHeightCells == 8U &&
                    direct.pixels[0U].present &&
                    direct.pixels[0U].height == 32U &&
                    !direct.pixels[holePixel].present &&
                    direct.pixels[stonePixel].present &&
                    direct.pixels[stonePixel].height == 255U &&
                    direct.pixels[stonePixel].material ==
                        cr::CreativeTerrainMaterial::Stone &&
                    catalog[0U].thumbnail.pixels == direct.pixels,
                "thumbnail is deterministic and preserves holes height and material");
}

bool libraryMutationIsBoundedAndExplicit() {
  cr::CreativeTerrainStampLibrary library;
  const cr::CreativeTerrainStamp original = makeStamp();
  const cr::CreativeTerrainStamp changed = variant(original, 7U);
  const auto installed = cr::installCreativeTerrainStamp(library, original);
  const auto noChange = cr::installCreativeTerrainStamp(library, original);
  const auto conflict = cr::installCreativeTerrainStamp(library, changed);
  const auto replaced =
      cr::installCreativeTerrainStamp(library, changed, true);
  const cr::CreativeTerrainStamp* replacement =
      cr::findCreativeTerrainStamp(library, original.assetId);
  const bool replacementMatches =
      replacement != nullptr && *replacement == changed;
  const auto selected = cr::selectCreativeTerrainStamp(library,
                                                        original.assetId);
  const auto missing = cr::selectCreativeTerrainStamp(library, "missing");
  const auto removed =
      cr::removeCreativeTerrainStamp(library, original.assetId);
  const auto removeMissing =
      cr::removeCreativeTerrainStamp(library, original.assetId);

  bool ok = expect(installed.accepted && installed.changed &&
                       noChange.accepted && !noChange.changed &&
                       noChange.status ==
                           cr::CreativeTerrainStampLibraryMutationStatus::NoChange &&
                       !conflict.accepted &&
                       conflict.status ==
                           cr::CreativeTerrainStampLibraryMutationStatus::Conflict &&
                       replaced.accepted && replaced.changed &&
                       replacementMatches &&
                       selected.accepted && !selected.changed &&
                       !missing.accepted && removed.accepted &&
                       library.stamps.empty() &&
                       library.selectedAssetId.empty() &&
                       !removeMissing.accepted,
                   "install conflict replacement selection and removal are explicit");

  for (std::size_t index = 0U;
       index < cr::kCreativeTerrainStampLibraryCapacity; ++index) {
    cr::CreativeTerrainStamp stamp = makeStamp(
        "stamp_" + std::to_string(index), "Capacity Stamp");
    ok = cr::installCreativeTerrainStamp(library, stamp).accepted && ok;
  }
  const cr::CreativeTerrainStampLibrary before = library;
  const auto overflow = cr::installCreativeTerrainStamp(
      library, makeStamp("stamp_overflow", "Overflow Stamp"));
  return expect(ok && !overflow.accepted &&
                    overflow.status ==
                        cr::CreativeTerrainStampLibraryMutationStatus::CapacityExceeded &&
                    library.stamps == before.stamps &&
                    library.selectedAssetId == before.selectedAssetId,
                "library capacity rejection is atomic");
}

bool sourceStatusAndRepairDistinguishDrift() {
  const cr::CreativeTerrainStamp source = makeStamp();
  cr::CreativeTerrainStampRecipe recipe;
  recipe.stamp = source;
  cr::CreativeTerrainStampLibrary library;
  const auto missing = cr::creativeTerrainStampSourceStatus(library, recipe);
  const auto repaired =
      cr::repairCreativeTerrainStampSource(library, recipe, false);
  const auto available =
      cr::creativeTerrainStampSourceStatus(library, recipe);

  cr::CreativeTerrainStampRecipe newer = recipe;
  newer.stamp.assetVersion += 1U;
  newer.stamp.contentSignature =
      cr::creativeTerrainStampContentSignature(newer.stamp);
  const auto versionMismatch =
      cr::creativeTerrainStampSourceStatus(library, newer);

  cr::CreativeTerrainStampRecipe different = recipe;
  different.stamp = variant(different.stamp, 7U);
  const auto contentMismatch =
      cr::creativeTerrainStampSourceStatus(library, different);
  const auto conflict =
      cr::repairCreativeTerrainStampSource(library, different, false);
  const auto replaced =
      cr::repairCreativeTerrainStampSource(library, different, true);

  cr::CreativeTerrainStampRecipe corrupt = recipe;
  corrupt.stamp.contentSignature ^= 1U;
  const auto incompatible =
      cr::creativeTerrainStampSourceStatus(library, corrupt);
  return expect(missing == cr::CreativeTerrainStampSourceStatus::Missing &&
                    repaired.accepted && available ==
                        cr::CreativeTerrainStampSourceStatus::Available &&
                    versionMismatch ==
                        cr::CreativeTerrainStampSourceStatus::VersionMismatch &&
                    contentMismatch ==
                        cr::CreativeTerrainStampSourceStatus::ContentMismatch &&
                    !conflict.accepted && replaced.accepted &&
                    *cr::findCreativeTerrainStamp(library, source.assetId) ==
                        different.stamp &&
                    incompatible ==
                        cr::CreativeTerrainStampSourceStatus::Incompatible,
                "source status and explicit repair distinguish every drift class");
}

bool codecRoundTripsExactlyAndRejectsCorruption() {
  const cr::CreativeTerrainStamp stamp = makeStamp();
  const cr::CreativeTerrainStampAssetEncodeResult encoded =
      cr::encodeCreativeTerrainStamp(stamp);
  const cr::CreativeTerrainStampAssetDecodeResult decoded =
      cr::decodeCreativeTerrainStamp(encoded.bytes);

  std::vector<std::uint8_t> truncated = encoded.bytes;
  truncated.pop_back();
  const auto truncatedResult = cr::decodeCreativeTerrainStamp(truncated);
  std::vector<std::uint8_t> badMagic = encoded.bytes;
  badMagic[0U] ^= 0xffU;
  const auto badMagicResult = cr::decodeCreativeTerrainStamp(badMagic);
  std::vector<std::uint8_t> futureCodec = encoded.bytes;
  futureCodec[8U] = 2U;
  const auto futureCodecResult = cr::decodeCreativeTerrainStamp(futureCodec);
  std::vector<std::uint8_t> corruptPayload = encoded.bytes;
  corruptPayload.back() ^= 1U;
  const auto corruptPayloadResult =
      cr::decodeCreativeTerrainStamp(corruptPayload);

  cr::CreativeTerrainStamp invalid = stamp;
  invalid.contentSignature ^= 1U;
  const auto invalidEncode = cr::encodeCreativeTerrainStamp(invalid);
  return expect(encoded.accepted && !encoded.bytes.empty() &&
                    decoded.accepted && decoded.stamp == stamp,
                "terrain stamp codec round trips every identity and payload fact") &&
         expect(!truncatedResult.accepted &&
                    !badMagicResult.accepted &&
                    !futureCodecResult.accepted &&
                    futureCodecResult.status ==
                        cr::CreativeTerrainStampAssetCodecStatus::UnsupportedVersion &&
                    !corruptPayloadResult.accepted &&
                    corruptPayloadResult.status ==
                        cr::CreativeTerrainStampAssetCodecStatus::InvalidStamp &&
                    !invalidEncode.accepted,
                "codec rejects truncation magic version payload and invalid source");
}

}  // namespace

int main() {
  bool ok = true;
  ok = catalogAndThumbnailRetainUsefulFacts() && ok;
  ok = libraryMutationIsBoundedAndExplicit() && ok;
  ok = sourceStatusAndRepairDistinguishDrift() && ok;
  ok = codecRoundTripsExactlyAndRejectsCorruption() && ok;
  return ok ? 0 : 1;
}
