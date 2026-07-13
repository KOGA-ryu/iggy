#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/math/Vec3.hpp"

namespace iggy3d {

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
};

struct StaticMeshPrimitive {
  std::uint32_t firstIndex = 0;
  std::uint32_t indexCount = 0;
  std::uint32_t materialIndex = 0;
};

struct StaticMeshAsset {
  std::string id;
  std::filesystem::path sourcePath;
  std::uint64_t contentHash = 0;
  std::vector<StaticMeshVertex> vertices;
  std::vector<std::uint32_t> indices;
  std::vector<StaticMeshPrimitive> primitives;
  std::vector<StaticMeshMaterial> materials;
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
  Vec3 boundsSize;
  std::uint64_t contentHash = 0;
};

struct StaticMeshAssetCatalogFailure {
  std::filesystem::path sourcePath;
  std::string reasonCode;
};

struct StaticMeshAssetCatalog {
  std::vector<StaticMeshAssetCatalogEntry> entries;
  std::vector<StaticMeshAssetCatalogFailure> failures;
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
