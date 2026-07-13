#pragma once

#include <filesystem>

#include "content/assets/StaticMeshAsset.hpp"

struct cgltf_data;

namespace iggy3d::detail {

void importStaticMeshMaterialsAndImages(
    const cgltf_data& data,
    const std::filesystem::path& assetPath,
    StaticMeshAsset& asset);

}  // namespace iggy3d::detail
