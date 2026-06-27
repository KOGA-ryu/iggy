#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>

#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/view/PrimitiveDrawList.hpp"
#include "app/iggy3d/room/VisualProof.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/vulkan/BufferImageResources.hpp"
#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveFileStore.hpp"
#include "runtime/session/SessionState.hpp"
#include "AutomationSmokeSupport.hpp"

namespace {

constexpr iggy3d::ProductPrimitiveColor kFloorColor{54, 78, 68};
constexpr iggy3d::ProductPrimitiveColor kWallColor{76, 86, 92};

struct PpmProofCounts {
  bool ok = false;
  std::uint64_t floorPixels = 0;
  std::uint64_t wallPixels = 0;
};

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return condition;
}

bool fieldAsUint64(const iggy3d::smoke::ReceiptFields& fields,
                   std::string_view key,
                   std::uint64_t& out) {
  const auto found = fields.find(std::string(key));
  if (found == fields.end() || found->second.empty()) {
    return false;
  }
  std::uint64_t value = 0;
  for (const char character : found->second) {
    if (character < '0' || character > '9') {
      return false;
    }
    value = value * 10ULL + static_cast<std::uint64_t>(character - '0');
  }
  out = value;
  return true;
}

bool fileContains(const std::filesystem::path& path, const std::string& text) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return false;
  }
  const std::string bytes((std::istreambuf_iterator<char>(input)),
                          std::istreambuf_iterator<char>());
  return bytes.find(text) != std::string::npos;
}

bool readHeaderToken(std::istream& input, std::string& token) {
  token.clear();
  char character = '\0';
  while (input.get(character)) {
    if (character == '#') {
      std::string ignored;
      std::getline(input, ignored);
      continue;
    }
    if (character == ' ' || character == '\n' || character == '\r' ||
        character == '\t') {
      continue;
    }
    token.push_back(character);
    break;
  }
  while (input.get(character)) {
    if (character == ' ' || character == '\n' || character == '\r' ||
        character == '\t') {
      break;
    }
    token.push_back(character);
  }
  return !token.empty();
}

PpmProofCounts parsePpmProofCounts(const std::filesystem::path& path) {
  PpmProofCounts counts;
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return counts;
  }

  std::string token;
  if (!readHeaderToken(input, token) || token != "P6") {
    return counts;
  }
  if (!readHeaderToken(input, token)) {
    return counts;
  }
  const std::uint64_t width = std::stoull(token);
  if (!readHeaderToken(input, token)) {
    return counts;
  }
  const std::uint64_t height = std::stoull(token);
  if (!readHeaderToken(input, token) || token != "255") {
    return counts;
  }

  const std::uint64_t pixelCount = width * height;
  for (std::uint64_t index = 0; index < pixelCount; ++index) {
    char bytes[3]{};
    input.read(bytes, 3);
    if (!input) {
      return counts;
    }
    const iggy3d::ProductPrimitiveColor color{
        static_cast<std::uint8_t>(bytes[0]),
        static_cast<std::uint8_t>(bytes[1]),
        static_cast<std::uint8_t>(bytes[2]),
    };
    if (color.r == kFloorColor.r && color.g == kFloorColor.g &&
        color.b == kFloorColor.b) {
      ++counts.floorPixels;
    }
    if (color.r == kWallColor.r && color.g == kWallColor.g &&
        color.b == kWallColor.b) {
      ++counts.wallPixels;
    }
  }
  counts.ok = true;
  return counts;
}

iggy3d::SaveDecodeResult decodeSavedRoom(
    const std::filesystem::path& saveFile) {
  const iggy3d::SaveFileReadResult read = iggy3d::readSaveFile(saveFile);
  if (!read.ok) {
    return {};
  }
  return iggy3d::decodeSaveEnvelope(read.encodedText);
}

}  // namespace

int main() {
  const std::filesystem::path binary = iggy3d::smoke::productAppBinary();
  if (!iggy3d::smoke::productAppAvailable(binary)) {
    std::cerr << "product app unavailable\n";
    return 77;
  }

  const std::filesystem::path saveRoot =
      iggy3d::smoke::cleanSaveRoot("room_visual_proof_editor_input_save");
  const std::filesystem::path saveFile = saveRoot / "save_001.iggy3d.save";
  const std::filesystem::path artifact =
      saveRoot / "custom_dungeon_draft_continue_visual_proof.ppm";

  iggy3d::smoke::ReceiptFields fields;
  int exitCode = 77;
  const bool saveExit =
      iggy3d::smoke::runProductCase(
          binary,
          "room_visual_proof_editor_input_save_exit",
          "frontend.select=new_world\nfrontend.execute=true\n"
          "world.title=Custom Draft\n"
          "world.draft_cell=1,2,#\n"
          "world.create=true\n"
          "system.pause=true\n"
          "menu.down=true\n"
          "pause.execute=true\n"
          "editor.input=editor.nudge_x_pos,editor.next_tool,editor.place\n"
          "menu.back=true\n"
          "pause.select=save_and_exit\n"
          "menu.confirm=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "product_save_source",
                              "pause_save_and_exit") &&
      iggy3d::smoke::hasField(fields, "product_save_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "active_product_save_id", "save_001") &&
      iggy3d::smoke::hasField(fields, "room_editor_hud_visible", "false") &&
      iggy3d::smoke::hasField(fields, "room_editor_hud_tool", "wall") &&
      iggy3d::smoke::hasField(fields, "room_editor_hud_last_operation",
                              "editor.place") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_visible", "false") &&
      iggy3d::smoke::hasField(fields, "room_editor_last_operation",
                              "editor.place") &&
      std::filesystem::exists(saveFile);

  fields.clear();
  const bool rebootStarter =
      saveExit &&
      iggy3d::smoke::runProductReceiptCase(
          binary,
          "room_visual_proof_reboot_starter",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "starter") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "false") &&
      iggy3d::smoke::hasField(fields, "save_count", "1") &&
      iggy3d::smoke::hasField(fields, "compatible_save_count", "1");

  fields.clear();
  const bool continued =
      rebootStarter &&
      iggy3d::smoke::runProductCase(
          binary,
          "room_visual_proof_continue_edited_room",
          "frontend.select=continue\nfrontend.execute=true\n",
          iggy3d::smoke::saveRootArg(saveRoot),
          fields,
          exitCode) &&
      exitCode == 0 && iggy3d::smoke::productReceipt(fields) &&
      iggy3d::smoke::automationApplied(fields) &&
      iggy3d::smoke::hasField(fields, "frontend_screen", "gameplay") &&
      iggy3d::smoke::hasField(fields, "gameplay_active", "true") &&
      iggy3d::smoke::hasField(fields, "product_save_load_status",
                              "product_save_loaded") &&
      iggy3d::smoke::hasField(fields, "product_save_load_source",
                              "continue") &&
      iggy3d::smoke::hasField(fields, "product_save_load_save_id",
                              "save_001") &&
      iggy3d::smoke::hasField(fields, "active_room_source",
                              "saved_authored_room") &&
      iggy3d::smoke::hasField(fields, "active_room_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_floor_count",
                              "58") &&
      iggy3d::smoke::hasField(fields, "active_room_authored_wall_count",
                              "62") &&
      iggy3d::smoke::hasField(fields, "room_editor_hud_visible", "false") &&
      iggy3d::smoke::hasField(fields, "room_editor_overlay_visible", "false") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_mesh_cpu_ready",
                              "true") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_mesh_source",
                              "scene_room_projection") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_asset_id",
                              "custom_dungeon_draft") &&
      iggy3d::smoke::hasField(fields, "product_vulkan_room_wall_draw_count",
                              "22") &&
      iggy3d::smoke::positiveIntegerField(
          fields, "product_vulkan_room_geometry_signature");

  std::uint64_t receiptWallDrawCount = 0;
  std::uint64_t receiptGeometrySignature = 0;
  const bool receiptNumbers =
      fieldAsUint64(fields, "product_vulkan_room_wall_draw_count",
                    receiptWallDrawCount) &&
      fieldAsUint64(fields, "product_vulkan_room_geometry_signature",
                    receiptGeometrySignature);

  const iggy3d::SaveDecodeResult decoded = decodeSavedRoom(saveFile);
  const bool decodedAuthoredRoom =
      decoded.status == iggy3d::SaveCodecStatus::Ok &&
      decoded.envelope.authoredRoom.present &&
      decoded.envelope.authoredRoom.id == "custom_dungeon_draft" &&
      decoded.envelope.authoredRoom.floors.size() == 58U &&
      decoded.envelope.authoredRoom.walls.size() == 62U;

  const iggy3d::ProductActiveRoomState activeRoom =
      iggy3d::buildProductActiveRoomFromSavedAuthoredRoom(
          decoded.envelope.authoredRoom);
  iggy3d::SessionState emptyState;
  const iggy3d::SceneProjectionResult scene =
      iggy3d::buildSceneProjection(emptyState, &activeRoom.room);
  const iggy3d::ProductPrimitiveDrawList drawList =
      iggy3d::buildProductPrimitiveDrawList(&scene, nullptr, &activeRoom.room,
                                            nullptr, nullptr);
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(scene.room);

  iggy3d::ProductRoomVisualProofRequest proofRequest;
  proofRequest.drawList = &drawList;
  proofRequest.outputPath = artifact;
  proofRequest.metadata.roomId = activeRoom.roomId;
  proofRequest.metadata.activeRoomSource = activeRoom.source;
  proofRequest.metadata.authoredFloorCount = activeRoom.authoredFloorCount;
  proofRequest.metadata.authoredWallCount = activeRoom.authoredWallCount;
  proofRequest.metadata.optimizedWallDrawCount =
      static_cast<std::uint64_t>(geometry.roomWallDrawCount);
  proofRequest.metadata.roomGeometrySignature =
      geometry.sourceRoomGeometrySignature;

  const iggy3d::ProductRoomVisualProofResult proof =
      iggy3d::writeProductRoomVisualProofPpm(proofRequest);
  const PpmProofCounts ppmCounts = parsePpmProofCounts(artifact);

  const bool artifactProof =
      continued && receiptNumbers && decodedAuthoredRoom && activeRoom.loaded &&
      activeRoom.source == "saved_authored_room" &&
      activeRoom.roomId == "custom_dungeon_draft" &&
      activeRoom.authoredFloorCount == 58U &&
      activeRoom.authoredWallCount == 62U && scene.room.loaded &&
      scene.room.assetId == "custom_dungeon_draft" && geometry.ready &&
      geometry.roomWallDrawCount == receiptWallDrawCount &&
      geometry.sourceRoomGeometrySignature == receiptGeometrySignature &&
      proof.ok && proof.status == "room_visual_proof_written" &&
      std::filesystem::exists(artifact) &&
      std::filesystem::file_size(artifact) > 0U &&
      proof.floorPixelCount > 0U && proof.wallPixelCount > 0U &&
      ppmCounts.ok && ppmCounts.floorPixels == proof.floorPixelCount &&
      ppmCounts.wallPixels == proof.wallPixelCount &&
      fileContains(artifact, "# room_id=custom_dungeon_draft\n") &&
      fileContains(artifact, "# active_room_source=saved_authored_room\n") &&
      fileContains(artifact, "# authored_floor_count=58\n") &&
      fileContains(artifact, "# authored_wall_count=62\n") &&
      fileContains(artifact, "# optimized_wall_draw_count=22\n") &&
      fileContains(artifact,
                   "# room_geometry_signature=" +
                       std::to_string(receiptGeometrySignature) + "\n");

  const bool ok =
      expect(saveExit, "editor input save and exit") &&
      expect(rebootStarter, "fresh starter sees save") &&
      expect(continued, "continue restores edited room") &&
      expect(receiptNumbers, "continue receipt render numbers parsed") &&
      expect(decodedAuthoredRoom, "saved authored room decoded") &&
      expect(artifactProof, "visual proof artifact matches continued room");

  std::cout << "room_visual_proof_save_exit="
            << (saveExit ? "true" : "false") << '\n';
  std::cout << "room_visual_proof_reboot_starter="
            << (rebootStarter ? "true" : "false") << '\n';
  std::cout << "room_visual_proof_continue="
            << (continued ? "true" : "false") << '\n';
  std::cout << "room_visual_proof_artifact="
            << (artifactProof ? "true" : "false") << '\n';
  std::cout << "room_visual_proof_artifact_path="
            << artifact.generic_string() << '\n';
  std::cout << "room_visual_proof_floor_pixels="
            << proof.floorPixelCount << '\n';
  std::cout << "room_visual_proof_wall_pixels="
            << proof.wallPixelCount << '\n';
  std::cout << "room_visual_proof_geometry_signature="
            << proof.metadata.roomGeometrySignature << '\n';
  std::cout << "result=" << (ok ? "pass" : "fail") << '\n';

  return ok ? 0 : 1;
}
