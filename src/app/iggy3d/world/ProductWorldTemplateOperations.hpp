#pragma once

#include <filesystem>

#include "app/iggy3d/world/DefaultWorldTemplate.hpp"

namespace iggy3d {

struct ProductAppOptions;

std::filesystem::path productPackagePathFromOptions(
    const ProductAppOptions& options);
ProductWorldTemplate productWorldTemplateFromOptions(
    const ProductAppOptions& options);

}  // namespace iggy3d
