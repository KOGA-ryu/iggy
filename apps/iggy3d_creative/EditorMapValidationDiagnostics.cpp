#include "EditorMapValidationDiagnostics.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

namespace {

using Code = cr::CreativeMapDiagnosticCode;
using Area = CreativeEditorMapDiagnosticArea;

constexpr std::array kDescriptors{
    CreativeEditorMapDiagnosticDescriptor{
        Code::Unknown, Area::Validation, "Unknown validation issue",
        "Inspect the diagnostic reason code."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::DocumentMissing, Area::Document, "Document unavailable",
        "Open or create a map before validating."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::DocumentInvalid, Area::Document, "Document is invalid",
        "Repair invalid document identity or object data."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::AssetCatalogUnavailable, Area::Assets,
        "Asset catalog unavailable", "Reload the project asset catalog."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::MissingStaticMeshAsset, Area::Assets,
        "Referenced asset is missing",
        "Restore the asset or replace the object reference."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::UnsupportedStaticMeshCollision, Area::Collision,
        "Asset collision is unsupported",
        "Author bounds or compound-bounds collision metadata."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::InvalidStaticMeshMetadata, Area::Assets,
        "Asset metadata is invalid",
        "Repair the asset authoring metadata and reload assets."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::NoPlayerSpawn, Area::PlayerSpawn, "Player spawn is missing",
        "Add a Player Spawn object on supported walkable ground."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::MultiplePlayerSpawns, Area::PlayerSpawn,
        "Multiple spawn candidates",
        "Verify spawn groups and fallback priorities."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::PlayerSpawnSettingsInvalid, Area::PlayerSpawn,
        "Player spawn settings are invalid",
        "Repair the profile, group, radius, or authored facing."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::PlayerSpawnProfileUnsupported, Area::PlayerSpawn,
        "Player profile is unsupported",
        "Select a runtime-supported player profile."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::PlayerSpawnOutsideWorldBounds, Area::PlayerSpawn,
        "Player spawn is outside map bounds",
        "Move the spawn and its clearance envelope inside the map."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::PlayerSpawnFloorUnsupported, Area::PlayerSpawn,
        "Player spawn has no supported floor",
        "Move the spawn onto a walkable surface."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::PlayerSpawnObstructed, Area::Collision,
        "Player spawn body is obstructed",
        "Clear walls or objects from the standing body envelope."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::PlayerSpawnUnreachable, Area::Navigation,
        "Player spawn is not on the reachable floor",
        "Connect the spawn floor to the walkable map."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::PlayerSpawnGroupUnavailable, Area::PlayerSpawn,
        "Requested spawn group is unavailable",
        "Add an eligible spawn to the requested group."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::NpcSpawnPlanInvalid, Area::Runtime,
        "NPC spawn plan is invalid",
        "Repair the NPC profile, placement, or assigned patrol route."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::RoomBakeRejected, Area::Runtime, "Runtime room bake failed",
        "Repair the reported geometry or bounds failure."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::RuntimeObjectSkipped, Area::Runtime,
        "Runtime object was skipped",
        "Give the object a supported runtime shape and valid bounds."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::InvalidRuntimeBounds, Area::Collision,
        "Runtime bounds are invalid",
        "Repair zero, inverted, or non-finite object bounds."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::AssetWalkabilitySkipped, Area::Navigation,
        "Asset walkability was skipped",
        "Use a supported transform or author explicit walkable surfaces."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::NoWalkableSurface, Area::Navigation,
        "Map has no walkable surface",
        "Add a floor or walkable asset collision surface."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::ReachabilityNotChecked, Area::Navigation,
        "Reachability was not checked",
        "Repair the room bake before checking navigation."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::ReachabilityInvalidCellSize, Area::Navigation,
        "Reachability cell size is invalid",
        "Use a positive finite validation cell size."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::ReachabilityGridTooLarge, Area::Navigation,
        "Reachability grid is too large",
        "Reduce map bounds or validate with a coarser cell size."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::ReachabilityNoUsableSeeds, Area::Navigation,
        "Reachability has no usable seed",
        "Place a valid spawn or navigation anchor on walkable ground."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::ReachabilityBlockedSeed, Area::Navigation,
        "Reachability seed is blocked",
        "Clear collision around the spawn or navigation anchor."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::ReachabilityIslands, Area::Navigation,
        "Disconnected walkable islands",
        "Connect isolated floors with doors, stairs, ramps, or bridges."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::LogicSourceUnlinked, Area::Logic,
        "Logic source is not linked",
        "Link the source to a compatible target or remove it."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::LogicTargetMissing, Area::Logic, "Logic target is missing",
        "Choose an existing compatible target."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::LogicTargetCollisionMissing, Area::Logic,
        "Logic target has no collision",
        "Give the target runtime collision or choose another target."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::LogicLinkInvalid, Area::Logic, "Logic link is invalid",
        "Repair the source-target relationship."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::ConflictingPressurePlates, Area::Logic,
        "Pressure-plate triggers conflict",
        "Remove duplicate competing triggers for the same target."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::ExitRequirementUnresolved, Area::Runtime,
        "Exit requirement cannot be satisfied",
        "Add visible matching loot or reduce the required item count."},
    CreativeEditorMapDiagnosticDescriptor{
        Code::DiagnosticCapacityExceeded, Area::Validation,
        "Diagnostic capacity exceeded",
        "Resolve visible errors, then run validation again."},
};

constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

void hashByte(std::uint64_t& hash, std::uint8_t value) noexcept {
  hash ^= value;
  hash *= kFnvPrime;
}

void hashU64(std::uint64_t& hash, std::uint64_t value) noexcept {
  for (std::size_t index = 0U; index < sizeof(value); ++index) {
    hashByte(hash, static_cast<std::uint8_t>(value & 0xffU));
    value >>= 8U;
  }
}

void hashText(std::uint64_t& hash, std::string_view value) noexcept {
  hashU64(hash, value.size());
  for (const char character : value) {
    hashByte(hash, static_cast<std::uint8_t>(character));
  }
}

}  // namespace

std::span<const CreativeEditorMapDiagnosticDescriptor>
creativeEditorMapDiagnosticDescriptors() noexcept {
  return kDescriptors;
}

const CreativeEditorMapDiagnosticDescriptor&
creativeEditorMapDiagnosticDescriptor(Code code) noexcept {
  for (const CreativeEditorMapDiagnosticDescriptor& descriptor :
       kDescriptors) {
    if (descriptor.code == code) {
      return descriptor;
    }
  }
  return kDescriptors.front();
}

std::string_view creativeEditorMapDiagnosticAreaLabel(Area area) noexcept {
  switch (area) {
    case Area::Document:
      return "Document";
    case Area::Assets:
      return "Assets";
    case Area::PlayerSpawn:
      return "Player Spawn";
    case Area::Runtime:
      return "Runtime";
    case Area::Collision:
      return "Collision";
    case Area::Navigation:
      return "Navigation";
    case Area::Logic:
      return "Logic";
    case Area::Validation:
      return "Validation";
    case Area::Count:
      break;
  }
  return "Validation";
}

std::uint64_t creativeEditorStaticMeshCatalogSignature(
    const iggy3d::StaticMeshAssetCatalog* catalog) noexcept {
  if (catalog == nullptr) {
    return 0U;
  }
  std::uint64_t hash = kFnvOffset;
  hashU64(hash, catalog->entries.size());
  for (const iggy3d::StaticMeshAssetCatalogEntry& entry : catalog->entries) {
    hashText(hash, entry.assetId);
    hashU64(hash, entry.contentHash);
    hashU64(hash, static_cast<std::uint64_t>(entry.authoringMetadata.status));
    hashU64(hash,
            static_cast<std::uint64_t>(entry.authoringMetadata.collisionMode));
    hashU64(hash, entry.authoringMetadata.walkable ? 1U : 0U);
    hashU64(hash, entry.collisionParts.size());
  }
  hashU64(hash, catalog->failures.size());
  for (const iggy3d::StaticMeshAssetCatalogFailure& failure :
       catalog->failures) {
    hashText(hash, failure.sourcePath.generic_string());
    hashText(hash, failure.reasonCode);
  }
  return hash;
}

CreativeEditorMapValidationCacheStatus
creativeEditorMapValidationCacheStatus(
    const CreativeEditorMapValidationCache& cache,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* catalog) noexcept {
  if (!cache.hasResult) {
    return CreativeEditorMapValidationCacheStatus::NotRun;
  }
  if (cache.documentId != document.id() ||
      cache.documentRevision != document.revision() ||
      cache.assetCatalogSignature !=
          creativeEditorStaticMeshCatalogSignature(catalog)) {
    return CreativeEditorMapValidationCacheStatus::Stale;
  }
  return CreativeEditorMapValidationCacheStatus::Current;
}

const cr::CreativeMapValidationResult& refreshCreativeEditorMapValidation(
    CreativeEditorMapValidationCache& cache,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* catalog,
    bool force) {
  if (!force && creativeEditorMapValidationCacheStatus(
                    cache, document, catalog) ==
                    CreativeEditorMapValidationCacheStatus::Current) {
    return cache.result;
  }

  cr::CreativeMapValidationRequest request;
  request.document = &document;
  request.staticMeshAssetCatalog = catalog;
  cache.result = cr::validateCreativeMap(request);
  cache.hasResult = true;
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.assetCatalogSignature =
      creativeEditorStaticMeshCatalogSignature(catalog);
  ++cache.buildCount;
  return cache.result;
}

}  // namespace iggy3d_creative_app
