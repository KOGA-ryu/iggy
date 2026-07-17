#include "app/iggy3d/creative/world/WorldService.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingOps.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot() {
  return std::filesystem::temp_directory_path() /
         "iggy3d_world_layout_persistence_tests";
}

cr::CreativeDocument document() {
  cr::CreativeDocument result = cr::CreativeDocument::create("Layout Save");
  static_cast<void>(result.assignId(9001U));
  return result;
}

cr::CreativeWorldLayout layout() {
  cr::CreativeWorldLayout result;
  result.stableKey = "layout_save_test";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_1";
  building.name = "House";
  result.buildings.push_back(building);
  cr::CreativeWorldLayoutBox floor;
  floor.buildingIndex = 0U;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.stableKey = "floor_1";
  floor.name = "Floor";
  floor.footprint = {{0, 0}, {8, 6}};
  floor.anchorLayer = 1.25;
  floor.layerCount = 2U;
  result.boxes.push_back(floor);
  static_cast<void>(
      cr::setCreativeWorldLayoutBuildingTemplateInstanceProvenance(
          result, 0U,
          {true, true, "house_template", 0x1234U, 0x5678U,
           cr::CreativeWorldLayoutBuildingTemplateOrientation::Rotate180,
           {12, -4}}));
  return result;
}

bool layoutTravelsInsideTheAtomicSaveEnvelope() {
  const std::filesystem::path root = testRoot();
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  cr::CreativeDocument doc = document();
  const cr::CreativeWorldLayout source = layout();

  iggy3d::CreativeWorldSaveRequest request;
  request.saveRoot = root;
  request.saveId = "layout_world";
  request.document = &doc;
  request.worldLayout = &source;
  request.worldTitle = "Layout World";
  request.saveTitle = "Layout World";
  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(request);

  std::ifstream input(saved.path);
  const std::string bytes{std::istreambuf_iterator<char>(input),
                          std::istreambuf_iterator<char>()};
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root, "layout_world"});
  const cr::CreativeWorldLayoutEncodeResult sourceBytes =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutEncodeResult openedBytes =
      cr::encodeCreativeWorldLayout(opened.worldLayout);
  const auto provenance =
      cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(
          opened.worldLayout, 0U);

  const bool ok =
      expect(saved.accepted && saved.saved && saved.worldLayoutPresent,
             "world save accepts the layout source") &&
      expect(bytes.find("creativeWorldLayout.present=true\n") !=
                     std::string::npos &&
                 bytes.find("creativeWorldLayout.encodedText=") !=
                     std::string::npos,
             "layout source is inside the same save envelope") &&
      expect(opened.accepted && opened.worldLayoutPresent,
             "world open restores the layout source") &&
      expect(sourceBytes.accepted && openedBytes.accepted &&
                 sourceBytes.encodedText == openedBytes.encodedText,
             "restored layout source is byte-equivalent") &&
      expect(opened.worldLayout.boxes.size() == 1U &&
                 opened.worldLayout.boxes[0].anchorLayer == 1.25 &&
                 opened.worldLayout.boxes[0].layerCount == 2U,
             "fractional structural anchor and layer count survive world save") &&
      expect(provenance.valid && provenance.templateId == "house_template" &&
                 provenance.sourceFingerprint == 0x1234U &&
                 provenance.instanceBaselineFingerprint == 0x5678U &&
                 provenance.orientation ==
                     cr::CreativeWorldLayoutBuildingTemplateOrientation::
                         Rotate180 &&
                 provenance.anchor == cr::CreativeTerrainCoord2{12, -4},
             "building template provenance travels inside the world save");
  std::filesystem::remove_all(root);
  return ok;
}

bool oldSaveWithoutLayoutRemainsReadable() {
  const std::filesystem::path root = testRoot();
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  cr::CreativeDocument doc = document();
  iggy3d::CreativeWorldSaveRequest request;
  request.saveRoot = root;
  request.saveId = "document_only";
  request.document = &doc;
  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(request);
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root, "document_only"});
  const bool ok = expect(saved.accepted && !saved.worldLayoutPresent,
                         "document-only save remains supported") &&
                  expect(opened.accepted && !opened.worldLayoutPresent,
                         "document-only save opens without fabricated source");
  std::filesystem::remove_all(root);
  return ok;
}

bool corruptEmbeddedLayoutRejectsTheWholeWorldOpen() {
  const std::filesystem::path root = testRoot();
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  cr::CreativeDocument doc = document();
  const std::string corrupt = "not a world layout";
  iggy3d::ProductCreativeSaveWriteRequest write;
  write.saveRoot = root;
  write.saveIdHint = "corrupt_layout";
  write.attemptToken = "attempt_001";
  write.document = &doc;
  write.creativeWorldLayoutEncoded = &corrupt;
  write.creativeWorldLayoutVersion = cr::kCreativeWorldLayoutCodecVersion;
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(write);
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root, "corrupt_layout"});
  const bool ok = expect(written.ok, "corrupt source fixture is transported") &&
                  expect(!opened.accepted,
                         "corrupt source rejects the complete world open") &&
                  expect(opened.worldLayoutCodecStatus !=
                             cr::CreativeWorldLayoutCodecStatus::Ready,
                         "corrupt source exposes codec failure");
  std::filesystem::remove_all(root);
  return ok;
}

bool versionOneLayoutSectionMigratesInsideWorldOpen() {
  const std::filesystem::path root = testRoot();
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  cr::CreativeDocument doc = document();
  const std::string versionOne =
      "IGGY3D_WORLD_LAYOUT 1\n"
      "L 1 6c65676163795f73617665 0 0 0 0 0 0 0 0\n"
      "END\n";
  iggy3d::ProductCreativeSaveWriteRequest write;
  write.saveRoot = root;
  write.saveIdHint = "legacy_layout";
  write.attemptToken = "attempt_legacy_layout";
  write.document = &doc;
  write.creativeWorldLayoutEncoded = &versionOne;
  write.creativeWorldLayoutVersion = 1U;
  const iggy3d::ProductCreativeSaveWriteResult written =
      iggy3d::writeCreativeDocumentSaveDurably(write);
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({root, "legacy_layout"});
  const bool ok =
      expect(written.ok && opened.accepted && opened.worldLayoutPresent,
             "version-one layout section remains openable") &&
      expect(opened.worldLayout.schemaVersion ==
                     cr::kCreativeWorldLayoutSchemaVersion &&
                 opened.worldLayout.stableKey == "legacy_save",
             "world open migrates version-one source to current schema");
  std::filesystem::remove_all(root);
  return ok;
}

}  // namespace

int main() {
  const bool ok = layoutTravelsInsideTheAtomicSaveEnvelope() &&
                  oldSaveWithoutLayoutRemainsReadable() &&
                  corruptEmbeddedLayoutRejectsTheWholeWorldOpen() &&
                  versionOneLayoutSectionMigratesInsideWorldOpen();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
