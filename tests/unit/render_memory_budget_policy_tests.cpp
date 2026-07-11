#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/VulkanMemoryAllocator.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool near(float actual, float expected) {
  return std::fabs(actual - expected) <= 1.0e-5F;
}

}  // namespace

int main() {
  bool ok = true;
  const std::vector<iggy3d::vulkan::FirstRoomVertex> vertices =
      iggy3d::vulkan::firstRoomBootstrapVertices();
  const std::vector<std::uint16_t> indices = iggy3d::vulkan::firstRoomBootstrapIndices();
  ok = expect(vertices.size() == 4U, "bootstrap vertex count") && ok;
  ok = expect(indices.size() == 6U, "bootstrap index count") && ok;

  const iggy3d::vulkan::CreativePreviewCpuGeometry preview =
      iggy3d::vulkan::buildCreativePreviewCpuGeometry();
  ok = expect(preview.ready, "creative preview geometry ready") && ok;
  ok = expect(preview.vertices.size() == 248U,
              "canonical cubes and path overlays have fixed vertices") &&
       ok;
  ok = expect(preview.indices.size() == 2232U,
              "canonical cubes and path overlays have fixed indices") &&
       ok;
  ok = expect(preview.indexedDraws[0].firstIndex == 0U &&
                  preview.indexedDraws[0].indexCount == 72U,
              "held role has one stable cube draw") &&
       expect(preview.indexedDraws[1].firstIndex == 72U &&
                  preview.indexedDraws[1].indexCount == 936U,
              "valid role aggregates solid and exact boundary") &&
       expect(preview.indexedDraws[2].firstIndex == 1152U &&
                  preview.indexedDraws[2].indexCount == 936U,
              "invalid role aggregates solid and exact boundary") &&
       expect(preview.indexedDraws[3].firstIndex == 72U &&
                  preview.indexedDraws[3].indexCount == 1080U,
              "valid path range retains normalized path wireframe") &&
       expect(preview.indexedDraws[4].firstIndex == 1152U &&
                  preview.indexedDraws[4].indexCount == 1080U,
              "invalid path range retains normalized path wireframe") &&
       ok;
  const auto colorMatches = [&](std::size_t vertex, float r, float g, float b) {
    return near(preview.vertices[vertex].color[0], r) &&
           near(preview.vertices[vertex].color[1], g) &&
           near(preview.vertices[vertex].color[2], b);
  };
  ok = expect(colorMatches(0U, 0.95F, 0.86F, 0.28F),
              "held cube is precolored yellow") &&
       expect(colorMatches(8U, 0.20F, 0.82F, 0.48F),
              "valid cube is precolored green") &&
       expect(colorMatches(128U, 0.92F, 0.24F, 0.20F),
              "invalid cube is precolored red") &&
       ok;
  float validOuterExtent = 0.0F;
  for (std::size_t index = 8U; index < 112U; ++index) {
    validOuterExtent =
        std::max(validOuterExtent,
                 std::max({std::fabs(preview.vertices[index].position[0]),
                           std::fabs(preview.vertices[index].position[1]),
                           std::fabs(preview.vertices[index].position[2])}));
  }
  ok = expect(near(std::fabs(preview.vertices[8].position[0]), 0.48F) &&
                  near(validOuterExtent, 0.5F),
              "target geometry combines 96 percent solid with exact boundary") &&
       ok;

  iggy3d::vulkan::AllocationBudgetSnapshot budget;
  budget.allocationCount = 3U;
  const iggy3d::RenderReceipt receipt =
      iggy3d::vulkan::makeMemoryBudgetReceipt(budget, "pass", "packet6_resource_ready");
  ok = expect(iggy3d::hasReceiptField(receipt, "memory_allocator",
                                      "manual_packet6_bootstrap"),
              "manual allocator receipt") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt, "per_frame_allocation_count", "0"),
              "no per-frame allocations") &&
       ok;
  ok = expect(iggy3d::hasReceiptField(receipt, "allocation_count", "3"),
              "allocation count receipt") &&
       ok;
  return ok ? 0 : 1;
}
