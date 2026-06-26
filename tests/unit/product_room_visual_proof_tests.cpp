#include "app/iggy3d/ProductRoomVisualProof.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "core/math/Aabb3.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
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

iggy3d::ProductPrimitiveDrawItem roomItem(
    iggy3d::ProductPrimitiveDrawKind kind,
    std::string stableName,
    iggy3d::Aabb3 bounds) {
  iggy3d::ProductPrimitiveDrawItem item;
  item.kind = kind;
  item.stableName = std::move(stableName);
  item.worldBounds = bounds;
  item.worldPosition = iggy3d::center(bounds);
  item.visible = true;
  return item;
}

}  // namespace

int main() {
  iggy3d::ProductPrimitiveDrawList drawList;
  drawList.items.push_back(roomItem(
      iggy3d::ProductPrimitiveDrawKind::FloorTile,
      "floor_1",
      iggy3d::makeAabb3({0.0F, 0.0F, 0.0F}, {2.0F, 0.1F, 2.0F})));
  drawList.items.push_back(roomItem(
      iggy3d::ProductPrimitiveDrawKind::WallTile,
      "wall_1",
      iggy3d::makeAabb3({0.75F, 0.0F, 0.0F}, {1.0F, 1.5F, 2.0F})));

  const std::filesystem::path output =
      std::filesystem::temp_directory_path() /
      "iggy3d_product_room_visual_proof_unit.ppm";
  std::filesystem::remove(output);

  iggy3d::ProductRoomVisualProofRequest request;
  request.drawList = &drawList;
  request.outputPath = output;
  request.width = 64;
  request.height = 64;
  request.metadata.roomId = "custom_dungeon_draft";
  request.metadata.activeRoomSource = "saved_authored_room";
  request.metadata.authoredFloorCount = 58;
  request.metadata.authoredWallCount = 62;
  request.metadata.optimizedWallDrawCount = 22;
  request.metadata.roomGeometrySignature = 123456789ULL;

  const iggy3d::ProductRoomVisualProofResult result =
      iggy3d::writeProductRoomVisualProofPpm(request);

  const bool ok =
      expect(result.ok, "visual proof writes") &&
      expect(result.status == "room_visual_proof_written",
             "visual proof status") &&
      expect(std::filesystem::exists(output), "artifact exists") &&
      expect(std::filesystem::file_size(output) > 0U, "artifact non-empty") &&
      expect(result.floorPixelCount > 0U, "floor pixels present") &&
      expect(result.wallPixelCount > 0U, "wall pixels present") &&
      expect(result.floorItemCount == 1U, "floor item counted") &&
      expect(result.wallItemCount == 1U, "wall item counted") &&
      expect(fileContains(output, "# iggy3d_room_visual_proof_v1\n"),
             "artifact version comment") &&
      expect(fileContains(output, "# room_id=custom_dungeon_draft\n"),
             "room id metadata") &&
      expect(fileContains(output, "# active_room_source=saved_authored_room\n"),
             "active room source metadata") &&
      expect(fileContains(output, "# authored_floor_count=58\n"),
             "floor count metadata") &&
      expect(fileContains(output, "# authored_wall_count=62\n"),
             "wall count metadata") &&
      expect(fileContains(output, "# optimized_wall_draw_count=22\n"),
             "wall draw metadata") &&
      expect(fileContains(output, "# room_geometry_signature=123456789\n"),
             "geometry signature metadata");

  std::cout << "product_room_visual_proof_unit="
            << (ok ? "pass" : "fail") << '\n';
  return ok ? 0 : 1;
}
