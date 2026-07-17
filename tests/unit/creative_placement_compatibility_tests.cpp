#include "app/iggy3d/creative/spatial/PlacementCompatibility.hpp"

#include <iostream>
#include <limits>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativePlacementCompatibilityResult resolve(
    cr::CreativePlacementHostPolicy policy,
    cr::CreativePlacementTargetFacts target,
    cr::CreativeVec3 normal) {
  return cr::resolveCreativePlacementCompatibility({policy, target, normal});
}

bool targetFactsFailClosed() {
  const cr::CreativePlacementTargetFacts empty =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::EmptyPlane);
  const cr::CreativePlacementTargetFacts terrain =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::Terrain,
          cr::CreativeObjectKind::TerrainPatch);
  const cr::CreativePlacementTargetFacts voxel =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::Voxel,
          cr::CreativeObjectKind::Wall);
  const cr::CreativePlacementTargetFacts invalidVoxel =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::Voxel,
          cr::CreativeObjectKind::Unknown);
  const cr::CreativePlacementTargetFacts authored =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::AuthoredObject,
          cr::CreativeObjectKind::Wall, 42U);
  const cr::CreativePlacementTargetFacts missingObject =
      cr::makeCreativePlacementTargetFacts(
          cr::CreativePlacementTargetSource::AuthoredObject,
          cr::CreativeObjectKind::Wall);

  return expect(empty.valid && terrain.valid && voxel.valid && authored.valid,
                "well-formed target facts are valid") &&
         expect(!invalidVoxel.valid && !missingObject.valid,
                "mismatched or absent host facts fail closed") &&
         expect(!resolve(cr::CreativePlacementHostPolicy::AnyKnownTarget, {},
                         {0.0, 1.0, 0.0})
                     .allowed,
                "unknown target cannot satisfy even the broad policy");
}

bool hostAndDirectionPoliciesAreOrthogonal() {
  const auto empty = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::EmptyPlane);
  const auto terrain = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::Terrain,
      cr::CreativeObjectKind::TerrainPatch);
  const auto wall = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::AuthoredObject,
      cr::CreativeObjectKind::Wall, 11U);
  const auto crate = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::AuthoredObject,
      cr::CreativeObjectKind::Crate, 12U);
  const auto decal = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::AuthoredObject,
      cr::CreativeObjectKind::Decal, 13U);

  const auto support = resolve(
      cr::CreativePlacementHostPolicy::SupportingSurface, empty,
      {0.0, 1.0, 0.0});
  const auto supportSide = resolve(
      cr::CreativePlacementHostPolicy::SupportingSurface, wall,
      {1.0, 0.0, 0.0});
  const auto supportOnCrate = resolve(
      cr::CreativePlacementHostPolicy::SupportingSurface, crate,
      {0.0, 1.0, 0.0});
  const auto supportOnDecal = resolve(
      cr::CreativePlacementHostPolicy::SupportingSurface, decal,
      {0.0, 1.0, 0.0});
  const auto structuralWall = resolve(
      cr::CreativePlacementHostPolicy::StructuralVerticalSurface, wall,
      {1.0, 0.0, 0.0});
  const auto structuralCrate = resolve(
      cr::CreativePlacementHostPolicy::StructuralVerticalSurface, crate,
      {1.0, 0.0, 0.0});
  const auto structuralTop = resolve(
      cr::CreativePlacementHostPolicy::StructuralVerticalSurface, wall,
      {0.0, 1.0, 0.0});
  const auto solidCrate = resolve(
      cr::CreativePlacementHostPolicy::SolidVerticalSurface, crate,
      {0.0, 0.0, 1.0});
  const auto solidTerrain = resolve(
      cr::CreativePlacementHostPolicy::SolidSurface, terrain,
      {0.6, 0.8, 0.0});
  const auto solidEmpty = resolve(
      cr::CreativePlacementHostPolicy::SolidSurface, empty,
      {0.0, 1.0, 0.0});

  return expect(support.allowed && supportOnCrate.allowed,
                "supporting surfaces admit ground and solid top faces") &&
         expect(!supportSide.allowed &&
                    supportSide.status ==
                        cr::CreativePlacementCompatibilityStatus::
                            SurfaceDirectionDisallowed,
                "supporting policy rejects side faces") &&
         expect(!supportOnDecal.allowed &&
                    supportOnDecal.status ==
                        cr::CreativePlacementCompatibilityStatus::
                            HostDisallowed,
                "non-solid authored hosts reject stacked props") &&
         expect(structuralWall.allowed && !structuralCrate.allowed,
                "structural policy distinguishes walls from solid props") &&
         expect(!structuralTop.allowed &&
                    structuralTop.status ==
                        cr::CreativePlacementCompatibilityStatus::
                            SurfaceDirectionDisallowed,
                "structural attachments require a vertical face") &&
         expect(solidCrate.allowed && solidTerrain.allowed,
                "solid policies admit valid wall and terrain surfaces") &&
         expect(!solidEmpty.allowed &&
                    solidEmpty.status ==
                        cr::CreativePlacementCompatibilityStatus::
                            SourceDisallowed,
                "surface-bound items reject an empty construction plane");
}

bool invalidRequestsExposeStableReasons() {
  const auto empty = cr::makeCreativePlacementTargetFacts(
      cr::CreativePlacementTargetSource::EmptyPlane);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto invalidNormal = resolve(
      cr::CreativePlacementHostPolicy::SupportingSurface, empty,
      {nan, 0.0, 0.0});
  const auto invalidPolicy = resolve(
      cr::CreativePlacementHostPolicy::Count, empty, {0.0, 1.0, 0.0});

  return expect(invalidNormal.evaluated && !invalidNormal.allowed &&
                    invalidNormal.status ==
                        cr::CreativePlacementCompatibilityStatus::
                            InvalidRequest,
                "non-finite normals fail closed") &&
         expect(invalidPolicy.evaluated && !invalidPolicy.allowed,
                "invalid policy fails closed") &&
         expect(cr::toString(cr::CreativePlacementCompatibilityStatus::
                                 InvalidRequest) ==
                        "creative_placement_compatibility_invalid" &&
                    cr::toString(cr::CreativePlacementCompatibilityStatus::
                                     UnknownTarget) ==
                        "creative_placement_target_unknown" &&
                    cr::toString(cr::CreativePlacementCompatibilityStatus::
                                     SourceDisallowed) ==
                        "creative_placement_target_source_disallowed" &&
                    cr::toString(cr::CreativePlacementCompatibilityStatus::
                                     HostDisallowed) ==
                        "creative_placement_host_disallowed" &&
                    cr::toString(cr::CreativePlacementCompatibilityStatus::
                                     SurfaceDirectionDisallowed) ==
                        "creative_placement_surface_direction_disallowed" &&
                    cr::toString(cr::CreativePlacementCompatibilityStatus::
                                     Ready) ==
                        "creative_placement_compatible",
                "compatibility failures expose stable reason codes");
}

}  // namespace

int main() {
  const bool ok = targetFactsFailClosed() &&
                  hostAndDirectionPoliciesAreOrthogonal() &&
                  invalidRequestsExposeStableReasons();
  if (!ok) {
    return 1;
  }
  std::cout << "creative placement compatibility tests passed\n";
  return 0;
}
