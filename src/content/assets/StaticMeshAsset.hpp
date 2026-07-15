#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "content/assets/StaticMeshAuthoringMetadata.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

inline constexpr std::uint32_t kInvalidStaticMeshImageIndex =
    std::numeric_limits<std::uint32_t>::max();
inline constexpr std::size_t kMaxStaticMeshCollisionPartCount = 256U;

enum class StaticMeshTextureWrap : std::uint8_t {
  Repeat,
  ClampToEdge,
  MirroredRepeat,
};

enum class StaticMeshTextureFilter : std::uint8_t {
  Nearest,
  Linear,
};

struct StaticMeshVertex {
  Vec3 position;
  Vec3 normal;
  float uv[2]{};
};

struct StaticMeshMaterial {
  std::string name;
  float baseColorFactor[4]{1.0F, 1.0F, 1.0F, 1.0F};
  float metallicFactor = 1.0F;
  float roughnessFactor = 1.0F;
  std::string baseColorTextureUri;
  std::uint32_t baseColorImageIndex = kInvalidStaticMeshImageIndex;
  std::int32_t baseColorTexcoord = 0;
  float baseColorUvOffset[2]{};
  float baseColorUvScale[2]{1.0F, 1.0F};
  float baseColorUvRotationRadians = 0.0F;
  StaticMeshTextureWrap wrapS = StaticMeshTextureWrap::Repeat;
  StaticMeshTextureWrap wrapT = StaticMeshTextureWrap::Repeat;
  StaticMeshTextureFilter minFilter = StaticMeshTextureFilter::Linear;
  StaticMeshTextureFilter magFilter = StaticMeshTextureFilter::Linear;
  std::string textureFailureReason;
};

struct StaticMeshImage {
  std::string source;
  std::string mimeType;
  std::uint64_t contentHash = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::vector<std::uint8_t> rgba8;
};

struct StaticMeshPrimitive {
  std::uint32_t firstIndex = 0;
  std::uint32_t indexCount = 0;
  std::uint32_t materialIndex = 0;
  bool hasTexcoord0 = false;
};

struct StaticMeshCollisionPart {
  Vec3 boundsMin;
  Vec3 boundsMax;
  bool walkable = false;
};

struct StaticMeshAsset {
  std::string id;
  std::filesystem::path sourcePath;
  std::uint64_t contentHash = 0;
  std::vector<StaticMeshVertex> vertices;
  std::vector<std::uint32_t> indices;
  std::vector<StaticMeshPrimitive> primitives;
  std::vector<StaticMeshMaterial> materials;
  std::vector<StaticMeshImage> images;
  std::vector<StaticMeshCollisionPart> collisionParts;
  StaticMeshAuthoringMetadata authoringMetadata;
  std::size_t textureFailureCount = 0;
  Vec3 boundsMin;
  Vec3 boundsMax;
  bool hasBounds = false;
};

enum class StaticMeshImportStatus : std::uint8_t {
  NotRequested,
  InvalidAssetId,
  FileNotFound,
  ParseFailed,
  BufferLoadFailed,
  ValidationFailed,
  UnsupportedFeature,
  InvalidGeometry,
  Imported,
};

struct StaticMeshImportResult {
  StaticMeshAsset asset;
  StaticMeshImportStatus status = StaticMeshImportStatus::NotRequested;
  std::string reasonCode = "static_mesh_import_not_requested";

  [[nodiscard]] bool ok() const noexcept {
    return status == StaticMeshImportStatus::Imported;
  }
};

struct StaticMeshAssetCatalogEntry {
  std::string assetId;
  std::string label;
  // Flattened asset-space bounds relative to the shared glTF origin.
  Vec3 boundsMin;
  Vec3 boundsMax;
  std::uint64_t contentHash = 0;
  StaticMeshAuthoringMetadata authoringMetadata;
  std::vector<StaticMeshCollisionPart> collisionParts;
};

struct StaticMeshAssetCatalogFailure {
  std::filesystem::path sourcePath;
  std::string reasonCode;
};

struct StaticMeshAssetCatalog {
  std::vector<StaticMeshAssetCatalogEntry> entries;
  std::vector<StaticMeshAssetCatalogFailure> failures;

  // Discovery stores entries in asset-id order. Keeping lookup on the compact
  // immutable vector avoids a second catalog index in the frame path.
  [[nodiscard]] const StaticMeshAssetCatalogEntry* find(
      std::string_view assetId) const noexcept;
};

[[nodiscard]] bool validStaticMeshAssetId(std::string_view assetId) noexcept;
[[nodiscard]] StaticMeshImportResult importStaticMeshGlb(
    const std::filesystem::path& path,
    std::string_view assetId);
[[nodiscard]] StaticMeshAssetCatalog discoverStaticMeshAssetCatalog(
    const std::filesystem::path& root);

class StaticMeshAssetCache {
public:
  void setRoot(std::filesystem::path root);
  [[nodiscard]] const std::filesystem::path& root() const noexcept;
  [[nodiscard]] const StaticMeshAsset* find(std::string_view assetId);
  [[nodiscard]] std::string_view failureReason(
      std::string_view assetId) const noexcept;
  [[nodiscard]] std::size_t loadedAssetCount() const noexcept;
  [[nodiscard]] std::size_t failedAssetCount() const noexcept;

private:
  std::filesystem::path root_;
  std::unordered_map<std::string, StaticMeshAsset> assets_;
  std::unordered_map<std::string, std::string> failures_;
};

}  // namespace iggy3d
