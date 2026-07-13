#include "content/assets/StaticMeshAsset.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <utility>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace iggy3d {
namespace {

constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

void setFailure(StaticMeshImportResult& result,
                StaticMeshImportStatus status,
                std::string_view reasonCode) {
  result.status = status;
  result.reasonCode = std::string(reasonCode);
  result.asset = {};
}

[[nodiscard]] bool finiteVec3(Vec3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

[[nodiscard]] Vec3 normalized(Vec3 value) noexcept {
  const float lengthSquared =
      value.x * value.x + value.y * value.y + value.z * value.z;
  if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-12F) {
    return {};
  }
  const float inverseLength = 1.0F / std::sqrt(lengthSquared);
  return value * inverseLength;
}

[[nodiscard]] Vec3 transformPoint(const cgltf_float matrix[16],
                                  Vec3 value) noexcept {
  return {
      matrix[0] * value.x + matrix[4] * value.y + matrix[8] * value.z +
          matrix[12],
      matrix[1] * value.x + matrix[5] * value.y + matrix[9] * value.z +
          matrix[13],
      matrix[2] * value.x + matrix[6] * value.y + matrix[10] * value.z +
          matrix[14],
  };
}

[[nodiscard]] Vec3 transformDirection(const cgltf_float matrix[16],
                                      Vec3 value) noexcept {
  return normalized({
      matrix[0] * value.x + matrix[4] * value.y + matrix[8] * value.z,
      matrix[1] * value.x + matrix[5] * value.y + matrix[9] * value.z,
      matrix[2] * value.x + matrix[6] * value.y + matrix[10] * value.z,
  });
}

[[nodiscard]] const cgltf_accessor* findAttribute(
    const cgltf_primitive& primitive,
    cgltf_attribute_type type,
    cgltf_int index = 0) noexcept {
  for (cgltf_size attributeIndex = 0;
       attributeIndex < primitive.attributes_count; ++attributeIndex) {
    const cgltf_attribute& attribute = primitive.attributes[attributeIndex];
    if (attribute.type == type && attribute.index == index) {
      return attribute.data;
    }
  }
  return nullptr;
}

[[nodiscard]] bool readVec3(const cgltf_accessor* accessor,
                            cgltf_size index,
                            Vec3& out) noexcept {
  cgltf_float values[3]{};
  if (accessor == nullptr ||
      !cgltf_accessor_read_float(accessor, index, values, 3U)) {
    return false;
  }
  out = {values[0], values[1], values[2]};
  return finiteVec3(out);
}

[[nodiscard]] bool readUv(const cgltf_accessor* accessor,
                          cgltf_size index,
                          float out[2]) noexcept {
  if (accessor == nullptr) {
    out[0] = 0.0F;
    out[1] = 0.0F;
    return true;
  }
  cgltf_float values[2]{};
  if (!cgltf_accessor_read_float(accessor, index, values, 2U) ||
      !std::isfinite(values[0]) || !std::isfinite(values[1])) {
    return false;
  }
  out[0] = values[0];
  out[1] = values[1];
  return true;
}

void extendBounds(StaticMeshAsset& asset, Vec3 position) noexcept {
  if (!asset.hasBounds) {
    asset.boundsMin = position;
    asset.boundsMax = position;
    asset.hasBounds = true;
    return;
  }
  asset.boundsMin.x = std::min(asset.boundsMin.x, position.x);
  asset.boundsMin.y = std::min(asset.boundsMin.y, position.y);
  asset.boundsMin.z = std::min(asset.boundsMin.z, position.z);
  asset.boundsMax.x = std::max(asset.boundsMax.x, position.x);
  asset.boundsMax.y = std::max(asset.boundsMax.y, position.y);
  asset.boundsMax.z = std::max(asset.boundsMax.z, position.z);
}

[[nodiscard]] std::uint64_t fileHash(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  std::uint64_t hash = kFnvOffset;
  std::array<char, 4096> bytes{};
  while (input) {
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    const std::streamsize count = input.gcount();
    for (std::streamsize index = 0; index < count; ++index) {
      hash ^= static_cast<std::uint8_t>(bytes[static_cast<std::size_t>(index)]);
      hash *= kFnvPrime;
    }
  }
  return hash;
}

[[nodiscard]] std::uint32_t materialIndexFor(
    const cgltf_data& data,
    const cgltf_material* material) noexcept {
  if (material == nullptr || data.materials == nullptr) {
    return 0U;
  }
  const std::ptrdiff_t index = material - data.materials;
  return index >= 0 && static_cast<cgltf_size>(index) < data.materials_count
             ? static_cast<std::uint32_t>(index)
             : 0U;
}

[[nodiscard]] std::string textureUri(const cgltf_texture_view& view,
                                     const cgltf_data& data) {
  if (view.texture == nullptr || view.texture->image == nullptr) {
    return {};
  }
  const cgltf_image* image = view.texture->image;
  if (image->uri != nullptr) {
    return image->uri;
  }
  if (data.images != nullptr) {
    const std::ptrdiff_t index = image - data.images;
    if (index >= 0 && static_cast<cgltf_size>(index) < data.images_count) {
      return "embedded://image/" + std::to_string(index);
    }
  }
  return {};
}

void appendMaterials(const cgltf_data& data, StaticMeshAsset& asset) {
  asset.materials.reserve(std::max<cgltf_size>(1U, data.materials_count));
  if (data.materials_count == 0U) {
    asset.materials.push_back({});
    return;
  }
  for (cgltf_size index = 0; index < data.materials_count; ++index) {
    const cgltf_material& source = data.materials[index];
    StaticMeshMaterial material;
    if (source.name != nullptr) {
      material.name = source.name;
    }
    if (source.has_pbr_metallic_roughness) {
      const cgltf_pbr_metallic_roughness& pbr =
          source.pbr_metallic_roughness;
      std::copy_n(pbr.base_color_factor, 4U, material.baseColorFactor);
      material.metallicFactor = pbr.metallic_factor;
      material.roughnessFactor = pbr.roughness_factor;
      material.baseColorTextureUri = textureUri(pbr.base_color_texture, data);
    }
    asset.materials.push_back(std::move(material));
  }
}

[[nodiscard]] bool appendPrimitive(const cgltf_data& data,
                                   const cgltf_node& node,
                                   const cgltf_primitive& primitive,
                                   StaticMeshAsset& asset,
                                   std::string& failureReason) {
  if (primitive.type != cgltf_primitive_type_triangles ||
      primitive.targets_count != 0U || node.skin != nullptr) {
    failureReason = "static_mesh_feature_unsupported";
    return false;
  }
  const cgltf_accessor* positions =
      findAttribute(primitive, cgltf_attribute_type_position);
  const cgltf_accessor* normals =
      findAttribute(primitive, cgltf_attribute_type_normal);
  const cgltf_accessor* uvs =
      findAttribute(primitive, cgltf_attribute_type_texcoord, 0);
  if (positions == nullptr || positions->count == 0U ||
      positions->count > std::numeric_limits<std::uint32_t>::max()) {
    failureReason = "static_mesh_positions_invalid";
    return false;
  }

  cgltf_float world[16]{};
  cgltf_node_transform_world(&node, world);
  const std::uint32_t vertexBase =
      static_cast<std::uint32_t>(asset.vertices.size());
  if (asset.vertices.size() >
      std::numeric_limits<std::uint32_t>::max() - positions->count) {
    failureReason = "static_mesh_vertex_count_overflow";
    return false;
  }
  asset.vertices.reserve(asset.vertices.size() + positions->count);
  for (cgltf_size vertexIndex = 0; vertexIndex < positions->count;
       ++vertexIndex) {
    StaticMeshVertex vertex;
    Vec3 localPosition{};
    if (!readVec3(positions, vertexIndex, localPosition) ||
        !readUv(uvs, vertexIndex, vertex.uv)) {
      failureReason = "static_mesh_vertex_invalid";
      return false;
    }
    vertex.position = transformPoint(world, localPosition);
    if (!finiteVec3(vertex.position)) {
      failureReason = "static_mesh_vertex_invalid";
      return false;
    }
    Vec3 localNormal{};
    if (normals != nullptr && !readVec3(normals, vertexIndex, localNormal)) {
      failureReason = "static_mesh_normal_invalid";
      return false;
    }
    vertex.normal = transformDirection(world, localNormal);
    extendBounds(asset, vertex.position);
    asset.vertices.push_back(vertex);
  }

  const cgltf_size indexCount = primitive.indices != nullptr
                                   ? primitive.indices->count
                                   : positions->count;
  if (indexCount == 0U || indexCount % 3U != 0U ||
      asset.indices.size() >
          std::numeric_limits<std::uint32_t>::max() - indexCount) {
    failureReason = "static_mesh_indices_invalid";
    return false;
  }
  StaticMeshPrimitive output;
  output.firstIndex = static_cast<std::uint32_t>(asset.indices.size());
  output.indexCount = static_cast<std::uint32_t>(indexCount);
  output.materialIndex = materialIndexFor(data, primitive.material);
  asset.indices.reserve(asset.indices.size() + indexCount);
  for (cgltf_size index = 0; index < indexCount; ++index) {
    const cgltf_size localIndex = primitive.indices != nullptr
                                     ? cgltf_accessor_read_index(
                                           primitive.indices, index)
                                     : index;
    if (localIndex >= positions->count) {
      failureReason = "static_mesh_index_out_of_range";
      return false;
    }
    asset.indices.push_back(vertexBase + static_cast<std::uint32_t>(localIndex));
  }
  asset.primitives.push_back(output);
  return true;
}

}  // namespace

bool validStaticMeshAssetId(std::string_view assetId) noexcept {
  if (assetId.empty() || assetId.size() > 128U || assetId.front() == '/' ||
      assetId.find("..") != std::string_view::npos) {
    return false;
  }
  return std::all_of(assetId.begin(), assetId.end(), [](char value) {
    const unsigned char c = static_cast<unsigned char>(value);
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '/';
  });
}

StaticMeshImportResult importStaticMeshGlb(
    const std::filesystem::path& path,
    std::string_view assetId) {
  StaticMeshImportResult result;
  if (!validStaticMeshAssetId(assetId)) {
    setFailure(result, StaticMeshImportStatus::InvalidAssetId,
               "static_mesh_asset_id_invalid");
    return result;
  }
  if (!std::filesystem::is_regular_file(path)) {
    setFailure(result, StaticMeshImportStatus::FileNotFound,
               "static_mesh_file_not_found");
    return result;
  }

  cgltf_options options{};
  cgltf_data* data = nullptr;
  if (cgltf_parse_file(&options, path.string().c_str(), &data) !=
      cgltf_result_success) {
    setFailure(result, StaticMeshImportStatus::ParseFailed,
               "static_mesh_glb_parse_failed");
    return result;
  }
  const auto freeData = [&data] {
    cgltf_free(data);
    data = nullptr;
  };
  if (cgltf_load_buffers(&options, data, path.string().c_str()) !=
      cgltf_result_success) {
    freeData();
    setFailure(result, StaticMeshImportStatus::BufferLoadFailed,
               "static_mesh_glb_buffer_load_failed");
    return result;
  }
  if (cgltf_validate(data) != cgltf_result_success) {
    freeData();
    setFailure(result, StaticMeshImportStatus::ValidationFailed,
               "static_mesh_glb_validation_failed");
    return result;
  }

  result.asset.id = std::string(assetId);
  result.asset.sourcePath = path;
  result.asset.contentHash = fileHash(path);
  appendMaterials(*data, result.asset);
  std::string failureReason;
  for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex) {
    const cgltf_node& node = data->nodes[nodeIndex];
    if (node.mesh == nullptr) {
      continue;
    }
    for (cgltf_size primitiveIndex = 0;
         primitiveIndex < node.mesh->primitives_count; ++primitiveIndex) {
      if (!appendPrimitive(*data, node,
                           node.mesh->primitives[primitiveIndex],
                           result.asset, failureReason)) {
        freeData();
        setFailure(result,
                   failureReason == "static_mesh_feature_unsupported"
                       ? StaticMeshImportStatus::UnsupportedFeature
                       : StaticMeshImportStatus::InvalidGeometry,
                   failureReason);
        return result;
      }
    }
  }
  freeData();

  const Vec3 extent = result.asset.boundsMax - result.asset.boundsMin;
  if (!result.asset.hasBounds || result.asset.vertices.empty() ||
      result.asset.indices.empty() || result.asset.primitives.empty() ||
      !finiteVec3(extent) || extent.x <= 0.0F || extent.y <= 0.0F ||
      extent.z <= 0.0F) {
    setFailure(result, StaticMeshImportStatus::InvalidGeometry,
               "static_mesh_geometry_empty_or_degenerate");
    return result;
  }
  result.status = StaticMeshImportStatus::Imported;
  result.reasonCode = "static_mesh_imported";
  return result;
}

void StaticMeshAssetCache::setRoot(std::filesystem::path root) {
  if (root_ == root) {
    return;
  }
  root_ = std::move(root);
  assets_.clear();
  failures_.clear();
}

const std::filesystem::path& StaticMeshAssetCache::root() const noexcept {
  return root_;
}

const StaticMeshAsset* StaticMeshAssetCache::find(std::string_view assetId) {
  if (!validStaticMeshAssetId(assetId)) {
    return nullptr;
  }
  const std::string key(assetId);
  if (const auto found = assets_.find(key); found != assets_.end()) {
    return &found->second;
  }
  if (failures_.contains(key)) {
    return nullptr;
  }
  StaticMeshImportResult imported =
      importStaticMeshGlb(root_ / (key + ".glb"), key);
  if (!imported.ok()) {
    failures_.emplace(key, std::move(imported.reasonCode));
    return nullptr;
  }
  const auto [found, inserted] =
      assets_.emplace(key, std::move(imported.asset));
  (void)inserted;
  return &found->second;
}

std::string_view StaticMeshAssetCache::failureReason(
    std::string_view assetId) const noexcept {
  const auto found = failures_.find(std::string(assetId));
  return found == failures_.end() ? std::string_view{} : found->second;
}

std::size_t StaticMeshAssetCache::loadedAssetCount() const noexcept {
  return assets_.size();
}

std::size_t StaticMeshAssetCache::failedAssetCount() const noexcept {
  return failures_.size();
}

}  // namespace iggy3d
