#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/VulkanMemoryAllocator.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

}  // namespace

int main() {
  bool ok = true;
  const std::vector<iggy3d::vulkan::FirstRoomVertex> vertices =
      iggy3d::vulkan::firstRoomBootstrapVertices();
  const std::vector<std::uint16_t> indices = iggy3d::vulkan::firstRoomBootstrapIndices();
  ok = expect(vertices.size() == 4U, "bootstrap vertex count") && ok;
  ok = expect(indices.size() == 6U, "bootstrap index count") && ok;

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
