#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d {

struct ProductWorldTemplate {
  std::string packageId = "iggy3d.default_world";
  std::string scenarioId = "default";
  std::string displayName = "New World";
  std::string source = "builtin_default";
  std::uint64_t authoredFloorCount = 0;
  std::uint64_t authoredWallCount = 0;
};

ProductWorldTemplate defaultProductWorldTemplate();
ProductWorldTemplate devOverrideProductWorldTemplate(std::string_view packagePath,
                                                     std::string_view scenarioId);

}  // namespace iggy3d
