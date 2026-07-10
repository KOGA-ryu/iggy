#pragma once

#include <cstdint>
#include <filesystem>
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

struct ProductAppOptions;

ProductWorldTemplate defaultProductWorldTemplate();
ProductWorldTemplate devOverrideProductWorldTemplate(std::string_view packagePath,
                                                     std::string_view scenarioId);

std::filesystem::path productPackagePathFromOptions(
    const ProductAppOptions& options);
ProductWorldTemplate productWorldTemplateFromOptions(
    const ProductAppOptions& options);

}  // namespace iggy3d
