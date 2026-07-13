#include "content/assets/StaticMeshMaterialImport.hpp"

#include "content/assets/ImageDecode.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cgltf/cgltf.h"

namespace iggy3d::detail {
namespace {

constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr std::uint64_t kMaximumEncodedImageBytes = 64ULL * 1024ULL * 1024ULL;

[[nodiscard]] std::uint64_t bytesHash(
    std::span<const std::uint8_t> bytes) noexcept {
  std::uint64_t hash = kFnvOffset;
  for (const std::uint8_t byte : bytes) {
    hash ^= byte;
    hash *= kFnvPrime;
  }
  return hash;
}

void mixHash(std::uint64_t& hash, std::uint64_t value) noexcept {
  for (std::uint32_t shift = 0U; shift < 64U; shift += 8U) {
    hash ^= static_cast<std::uint8_t>((value >> shift) & 0xFFU);
    hash *= kFnvPrime;
  }
}

[[nodiscard]] bool safeRelativeImageUri(std::string_view uri) {
  if (uri.empty() || uri.find("://") != std::string_view::npos) {
    return false;
  }
  const std::filesystem::path path(uri);
  if (path.is_absolute()) {
    return false;
  }
  return std::none_of(path.begin(), path.end(), [](const auto& component) {
    return component == "..";
  });
}

[[nodiscard]] bool readFileBytes(const std::filesystem::path& path,
                                 std::vector<std::uint8_t>& bytes) {
  std::error_code error;
  const std::uintmax_t size = std::filesystem::file_size(path, error);
  if (error || size == 0U || size > kMaximumEncodedImageBytes) {
    return false;
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return false;
  }
  bytes.resize(static_cast<std::size_t>(size));
  input.read(reinterpret_cast<char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
  return input.gcount() == static_cast<std::streamsize>(bytes.size());
}

[[nodiscard]] int base64Value(char value) noexcept {
  if (value >= 'A' && value <= 'Z') {
    return value - 'A';
  }
  if (value >= 'a' && value <= 'z') {
    return value - 'a' + 26;
  }
  if (value >= '0' && value <= '9') {
    return value - '0' + 52;
  }
  if (value == '+') {
    return 62;
  }
  return value == '/' ? 63 : -1;
}

[[nodiscard]] bool decodeBase64(std::string_view encoded,
                                std::vector<std::uint8_t>& bytes) {
  if (encoded.empty() || encoded.size() > kMaximumEncodedImageBytes * 2ULL) {
    return false;
  }
  bytes.clear();
  bytes.reserve(std::min<std::size_t>(
      (encoded.size() / 4U) * 3U,
      static_cast<std::size_t>(kMaximumEncodedImageBytes) + 1U));
  std::uint32_t accumulator = 0U;
  std::uint32_t bitCount = 0U;
  bool padding = false;
  for (const char value : encoded) {
    if (value == '=') {
      padding = true;
      continue;
    }
    if (value == '\r' || value == '\n' || value == ' ' || value == '\t') {
      continue;
    }
    const int decoded = base64Value(value);
    if (padding || decoded < 0) {
      return false;
    }
    accumulator = (accumulator << 6U) | static_cast<std::uint32_t>(decoded);
    bitCount += 6U;
    if (bitCount >= 8U) {
      bitCount -= 8U;
      bytes.push_back(static_cast<std::uint8_t>(accumulator >> bitCount));
      accumulator &= (1U << bitCount) - 1U;
      if (bytes.size() > kMaximumEncodedImageBytes) {
        return false;
      }
    }
  }
  return !bytes.empty();
}

[[nodiscard]] bool encodedImageBytes(const cgltf_image& image,
                                     const std::filesystem::path& assetPath,
                                     std::vector<std::uint8_t>& bytes,
                                     std::string& source) {
  if (image.buffer_view != nullptr) {
    const cgltf_buffer_view& view = *image.buffer_view;
    const auto* begin = static_cast<const std::uint8_t*>(view.data);
    if (begin == nullptr && view.buffer != nullptr &&
        view.buffer->data != nullptr && view.offset <= view.buffer->size &&
        view.size <= view.buffer->size - view.offset) {
      begin = static_cast<const std::uint8_t*>(view.buffer->data) + view.offset;
    }
    if (begin == nullptr || view.size == 0U ||
        view.size > kMaximumEncodedImageBytes) {
      return false;
    }
    bytes.assign(begin, begin + view.size);
    source = "embedded://buffer_view";
    return true;
  }
  if (image.uri == nullptr) {
    return false;
  }
  const std::string_view uri(image.uri);
  if (uri.starts_with("data:")) {
    const std::size_t comma = uri.find(',');
    if (comma == std::string_view::npos ||
        uri.substr(0U, comma).find(";base64") == std::string_view::npos ||
        !decodeBase64(uri.substr(comma + 1U), bytes)) {
      return false;
    }
    source = "embedded://data_uri";
    return true;
  }
  if (!safeRelativeImageUri(uri)) {
    return false;
  }
  const std::filesystem::path resolved =
      (assetPath.parent_path() / std::filesystem::path(uri)).lexically_normal();
  if (!readFileBytes(resolved, bytes)) {
    return false;
  }
  source = uri;
  return true;
}

[[nodiscard]] StaticMeshTextureWrap textureWrap(cgltf_wrap_mode value) noexcept {
  switch (value) {
    case cgltf_wrap_mode_clamp_to_edge:
      return StaticMeshTextureWrap::ClampToEdge;
    case cgltf_wrap_mode_mirrored_repeat:
      return StaticMeshTextureWrap::MirroredRepeat;
    case cgltf_wrap_mode_repeat:
    default:
      return StaticMeshTextureWrap::Repeat;
  }
}

[[nodiscard]] StaticMeshTextureFilter textureFilter(
    cgltf_filter_type value) noexcept {
  switch (value) {
    case cgltf_filter_type_nearest:
    case cgltf_filter_type_nearest_mipmap_nearest:
    case cgltf_filter_type_nearest_mipmap_linear:
      return StaticMeshTextureFilter::Nearest;
    case cgltf_filter_type_undefined:
    case cgltf_filter_type_linear:
    case cgltf_filter_type_linear_mipmap_nearest:
    case cgltf_filter_type_linear_mipmap_linear:
    default:
      return StaticMeshTextureFilter::Linear;
  }
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

void appendImages(const cgltf_data& data,
                  const std::filesystem::path& assetPath,
                  StaticMeshAsset& asset,
                  std::vector<std::uint32_t>& importedIndices,
                  std::vector<std::string>& failureReasons) {
  importedIndices.assign(data.images_count, kInvalidStaticMeshImageIndex);
  failureReasons.resize(data.images_count);
  asset.images.reserve(data.images_count);
  for (cgltf_size index = 0U; index < data.images_count; ++index) {
    const cgltf_image& source = data.images[index];
    std::vector<std::uint8_t> encoded;
    std::string sourceName;
    if (!encodedImageBytes(source, assetPath, encoded, sourceName)) {
      failureReasons[index] = "static_mesh_texture_source_unavailable";
      continue;
    }
    DecodedImageRgba8 decoded = decodeImageRgba8(encoded);
    if (!decoded.ok()) {
      failureReasons[index] = decoded.reasonCode;
      continue;
    }
    StaticMeshImage image;
    image.source = std::move(sourceName);
    if (source.mime_type != nullptr) {
      image.mimeType = source.mime_type;
    }
    image.contentHash = bytesHash(encoded);
    image.width = decoded.width;
    image.height = decoded.height;
    image.rgba8 = std::move(decoded.pixels);
    importedIndices[index] = static_cast<std::uint32_t>(asset.images.size());
    asset.images.push_back(std::move(image));
  }
}

void applyTextureView(const cgltf_data& data,
                      const cgltf_texture_view& view,
                      const std::vector<std::uint32_t>& importedImageIndices,
                      const std::vector<std::string>& imageFailureReasons,
                      StaticMeshMaterial& material) {
  material.baseColorTextureUri = textureUri(view, data);
  if (view.texture == nullptr || view.texture->image == nullptr ||
      data.images == nullptr) {
    return;
  }
  const std::ptrdiff_t sourceImageIndex = view.texture->image - data.images;
  if (sourceImageIndex < 0 ||
      static_cast<cgltf_size>(sourceImageIndex) >= data.images_count) {
    material.textureFailureReason = "static_mesh_texture_image_invalid";
    return;
  }
  const std::size_t imageIndex = static_cast<std::size_t>(sourceImageIndex);
  material.baseColorTexcoord = view.texcoord;
  if (view.has_transform) {
    material.baseColorUvOffset[0] = view.transform.offset[0];
    material.baseColorUvOffset[1] = view.transform.offset[1];
    material.baseColorUvScale[0] = view.transform.scale[0];
    material.baseColorUvScale[1] = view.transform.scale[1];
    material.baseColorUvRotationRadians = view.transform.rotation;
    if (view.transform.has_texcoord) {
      material.baseColorTexcoord = view.transform.texcoord;
    }
  }
  const bool finiteTransform =
      std::isfinite(material.baseColorUvOffset[0]) &&
      std::isfinite(material.baseColorUvOffset[1]) &&
      std::isfinite(material.baseColorUvScale[0]) &&
      std::isfinite(material.baseColorUvScale[1]) &&
      std::isfinite(material.baseColorUvRotationRadians);
  if (!finiteTransform || material.baseColorTexcoord != 0) {
    material.textureFailureReason =
        !finiteTransform ? "static_mesh_texture_transform_invalid"
                         : "static_mesh_texture_texcoord_unsupported";
    return;
  }
  if (view.texture->sampler != nullptr) {
    const cgltf_sampler& sampler = *view.texture->sampler;
    material.wrapS = textureWrap(sampler.wrap_s);
    material.wrapT = textureWrap(sampler.wrap_t);
    material.minFilter = textureFilter(sampler.min_filter);
    material.magFilter = textureFilter(sampler.mag_filter);
  }
  material.baseColorImageIndex = importedImageIndices[imageIndex];
  if (material.baseColorImageIndex == kInvalidStaticMeshImageIndex) {
    material.textureFailureReason = imageFailureReasons[imageIndex].empty()
                                        ? "static_mesh_texture_decode_failed"
                                        : imageFailureReasons[imageIndex];
  }
}

void appendMaterials(
    const cgltf_data& data,
    const std::vector<std::uint32_t>& importedImageIndices,
    const std::vector<std::string>& imageFailureReasons,
    StaticMeshAsset& asset) {
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
      applyTextureView(data, pbr.base_color_texture, importedImageIndices,
                       imageFailureReasons, material);
      if (source.alpha_mode != cgltf_alpha_mode_opaque &&
          pbr.base_color_texture.texture != nullptr) {
        material.baseColorImageIndex = kInvalidStaticMeshImageIndex;
        material.textureFailureReason =
            "static_mesh_texture_alpha_mode_unsupported";
      }
      if (pbr.base_color_texture.texture != nullptr &&
          !material.textureFailureReason.empty()) {
        ++asset.textureFailureCount;
      }
    }
    asset.materials.push_back(std::move(material));
  }
}

}  // namespace

void importStaticMeshMaterialsAndImages(
    const cgltf_data& data,
    const std::filesystem::path& assetPath,
    StaticMeshAsset& asset) {
  std::vector<std::uint32_t> importedImageIndices;
  std::vector<std::string> imageFailureReasons;
  appendImages(data, assetPath, asset, importedImageIndices,
               imageFailureReasons);
  appendMaterials(data, importedImageIndices, imageFailureReasons, asset);
  for (const StaticMeshImage& image : asset.images) {
    mixHash(asset.contentHash, image.contentHash);
  }
}

}  // namespace iggy3d::detail
