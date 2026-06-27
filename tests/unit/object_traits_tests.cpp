#include "runtime/object/ObjectTraits.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ObjectAssetDefinition validAsset() {
  iggy3d::ObjectAssetDefinition asset;
  asset.id.value = "test_box";
  asset.primitiveShape.kind = iggy3d::ObjectPrimitiveShapeKind::Box;
  asset.primitiveShape.sizeMeters = {1.0F, 1.0F, 1.0F};
  asset.render.mesh.value = "generated_box";
  asset.render.material.value = "stone_proxy";
  asset.collision.kind = iggy3d::ObjectCollisionShapeKind::Box;
  asset.collision.sizeMeters = {1.0F, 1.0F, 1.0F};
  asset.physics.motion = iggy3d::ObjectPhysicsMotionKind::Static;
  asset.physics.massKilograms = 0.0F;
  asset.material.kind = iggy3d::ObjectMaterialKind::Stone;
  asset.interaction.verbs = {iggy3d::ObjectInteractionVerb::Inspect};
  asset.editor.displayName = "Test Box";
  return asset;
}

bool stableNamesAreLowerSnake() {
  return expect(iggy3d::objectAssetKindName(
                    iggy3d::ObjectAssetKind::PrimitiveShape) ==
                    "primitive_shape",
                "asset kind name") &&
         expect(iggy3d::objectPrimitiveShapeKindName(
                    iggy3d::ObjectPrimitiveShapeKind::Pillar) == "pillar",
                "shape kind name") &&
         expect(iggy3d::objectCollisionShapeKindName(
                    iggy3d::ObjectCollisionShapeKind::MeshProxy) ==
                    "mesh_proxy",
                "collision name") &&
         expect(iggy3d::objectPhysicsMotionKindName(
                    iggy3d::ObjectPhysicsMotionKind::Dynamic) == "dynamic",
                "motion name") &&
         expect(iggy3d::objectMaterialKindName(
                    iggy3d::ObjectMaterialKind::Wood) == "wood",
                "material name") &&
         expect(iggy3d::objectInteractionVerbName(
                    iggy3d::ObjectInteractionVerb::Damage) == "damage",
                "verb name") &&
         expect(iggy3d::objectValidationStatusName(
                    iggy3d::ObjectValidationStatus::AssetReferenceMissing) ==
                    "object_asset_reference_missing",
                "status name");
}

bool stableIdsAcceptLowerSnakeOnly() {
  return expect(iggy3d::isValidObjectStableId("wood_crate_001"),
                "lower snake valid") &&
         expect(!iggy3d::isValidObjectStableId("WoodCrate"),
                "caps invalid") &&
         expect(!iggy3d::isValidObjectStableId("wood-crate"),
                "dash invalid") &&
         expect(!iggy3d::isValidObjectStableId(""), "empty invalid");
}

bool builtInCatalogDefinesProxyShapeAssets() {
  const iggy3d::ObjectAssetCatalog catalog =
      iggy3d::makeBuiltInObjectAssetCatalog();
  const iggy3d::ObjectValidationResult validation =
      iggy3d::validateObjectAssetCatalog(catalog);
  const iggy3d::ObjectAssetResolveResult crate =
      iggy3d::resolveObjectAssetDefinition({&catalog, "wood_crate_proxy"});
  const iggy3d::ObjectAssetResolveResult wall =
      iggy3d::resolveObjectAssetDefinition({&catalog, "stone_wall_panel"});

  return expect(catalog.assets.size() == 4U, "built-in proxy count") &&
         expect(validation.ok, "built-in catalog valid") &&
         expect(catalog.assets[0].id.value == "stone_block_proxy",
                "stone block first") &&
         expect(catalog.assets[1].id.value == "stone_floor_slab",
                "floor slab second") &&
         expect(catalog.assets[2].id.value == "stone_wall_panel",
                "wall panel third") &&
         expect(catalog.assets[3].id.value == "wood_crate_proxy",
                "crate fourth") &&
         expect(crate.ok, "crate resolves") &&
         expect(crate.asset.material.kind == iggy3d::ObjectMaterialKind::Wood,
                "crate material") &&
         expect(crate.asset.physics.motion ==
                    iggy3d::ObjectPhysicsMotionKind::Dynamic,
                "crate dynamic") &&
         expect(crate.asset.material.flammable, "crate flammable") &&
         expect(wall.ok, "wall resolves") &&
         expect(wall.asset.collision.blocksVision, "wall blocks vision");
}

bool invalidAssetDefinitionsRejectWithStableReasons() {
  auto reject = [](auto mutate,
                   iggy3d::ObjectValidationStatus expected,
                   std::string_view reason,
                   std::string_view message) {
    iggy3d::ObjectAssetDefinition asset = validAsset();
    mutate(asset);
    const iggy3d::ObjectValidationResult result =
        iggy3d::validateObjectAssetDefinition(&asset);
    if (result.status != expected) {
      std::cerr << "status mismatch for " << message << ": got "
                << iggy3d::objectValidationStatusName(result.status)
                << " expected "
                << iggy3d::objectValidationStatusName(expected) << '\n';
    }
    return expect(!result.ok, message) &&
           expect(result.status == expected, "expected status") &&
           expect(result.reasonCode == reason, "expected reason");
  };

  bool ok = reject([](iggy3d::ObjectAssetDefinition& asset) {
                     asset.id.value = "BadId";
                   },
                   iggy3d::ObjectValidationStatus::InvalidId,
                   "object_invalid_id",
                   "invalid id rejects");
  ok = ok && reject([](iggy3d::ObjectAssetDefinition& asset) {
                      asset.render.mesh.value.clear();
                    },
                    iggy3d::ObjectValidationStatus::MissingRenderAsset,
                    "object_missing_render_asset",
                    "missing mesh rejects");
  ok = ok && reject([](iggy3d::ObjectAssetDefinition& asset) {
                      asset.primitiveShape.sizeMeters.x = 0.0F;
                    },
                    iggy3d::ObjectValidationStatus::InvalidPrimitiveShape,
                    "object_invalid_primitive_shape",
                    "zero primitive size rejects");
  ok = ok && reject([](iggy3d::ObjectAssetDefinition& asset) {
                      asset.collision.sizeMeters.y =
                          std::numeric_limits<float>::infinity();
                    },
                    iggy3d::ObjectValidationStatus::InvalidCollisionShape,
                    "object_invalid_collision_shape",
                    "infinite collision size rejects");
  ok = ok && reject([](iggy3d::ObjectAssetDefinition& asset) {
                      asset.physics.motion =
                          iggy3d::ObjectPhysicsMotionKind::Dynamic;
                      asset.physics.massKilograms = 0.0F;
                    },
                    iggy3d::ObjectValidationStatus::InvalidPhysicsProfile,
                    "object_invalid_physics_profile",
                    "dynamic zero mass rejects");
  ok = ok && reject([](iggy3d::ObjectAssetDefinition& asset) {
                      asset.material.durability = -1.0F;
                    },
                    iggy3d::ObjectValidationStatus::InvalidMaterialTraits,
                    "object_invalid_material_traits",
                    "negative durability rejects");
  ok = ok && reject([](iggy3d::ObjectAssetDefinition& asset) {
                      asset.interaction.verbs = {
                          static_cast<iggy3d::ObjectInteractionVerb>(255)};
                    },
                    iggy3d::ObjectValidationStatus::InvalidInteractionTraits,
                    "object_invalid_interaction_traits",
                    "invalid verb rejects");
  return ok;
}

bool catalogRejectsEmptyAndDuplicateIds() {
  const iggy3d::ObjectAssetCatalog empty;
  iggy3d::ObjectAssetCatalog duplicate;
  duplicate.assets = {validAsset(), validAsset()};
  const iggy3d::ObjectValidationResult emptyResult =
      iggy3d::validateObjectAssetCatalog(empty);
  const iggy3d::ObjectValidationResult duplicateResult =
      iggy3d::validateObjectAssetCatalog(duplicate);
  return expect(!emptyResult.ok, "empty rejects") &&
         expect(emptyResult.status == iggy3d::ObjectValidationStatus::CatalogEmpty,
                "empty status") &&
         expect(!duplicateResult.ok, "duplicate rejects") &&
         expect(duplicateResult.status ==
                    iggy3d::ObjectValidationStatus::DuplicateId,
                "duplicate status") &&
         expect(duplicateResult.assetId.value == "test_box",
                "duplicate id reported");
}

bool assetResolutionSeparatesAssetFromObjectInstance() {
  const iggy3d::ObjectAssetCatalog catalog =
      iggy3d::makeBuiltInObjectAssetCatalog();
  const iggy3d::ObjectAssetResolveResult asset =
      iggy3d::resolveObjectAssetDefinition({&catalog, "stone_block_proxy"});
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {10.0F, 0.0F, 6.0F};
  transform.rotationEulerRadians.y = 1.5707963F;
  iggy3d::WorldObject object =
      iggy3d::makeWorldObject("stone_block_184",
                              "stone_block_proxy",
                              transform);
  const iggy3d::ObjectValidationResult objectResult =
      iggy3d::validateWorldObject({&catalog, &object});

  return expect(asset.ok, "asset resolves") &&
         expect(asset.asset.id.value == "stone_block_proxy",
                "asset id unchanged") &&
         expect(object.id.value == "stone_block_184", "object id") &&
         expect(object.assetId.value == "stone_block_proxy",
                "object references asset") &&
         expect(object.transform.position.x == 10.0F,
                "object transform state") &&
         expect(objectResult.ok, "world object valid") &&
         expect(objectResult.objectId.value == "stone_block_184",
                "object id reported") &&
         expect(objectResult.assetId.value == "stone_block_proxy",
                "object asset id reported");
}

bool invalidWorldObjectsRejectWithoutMutatingAssetCatalog() {
  const iggy3d::ObjectAssetCatalog catalog =
      iggy3d::makeBuiltInObjectAssetCatalog();
  iggy3d::WorldObject missingAsset =
      iggy3d::makeWorldObject("crate_001", "missing_asset", iggy3d::identityTransform3());
  iggy3d::WorldObject invalidTransform =
      iggy3d::makeWorldObject("crate_002", "wood_crate_proxy", iggy3d::identityTransform3());
  invalidTransform.transform.scale.x = 0.0F;
  iggy3d::WorldObject invalidRuntime =
      iggy3d::makeWorldObject("crate_003", "wood_crate_proxy", iggy3d::identityTransform3());
  invalidRuntime.state.health = -1.0F;

  const iggy3d::ObjectValidationResult missingAssetResult =
      iggy3d::validateWorldObject({&catalog, &missingAsset});
  const iggy3d::ObjectValidationResult invalidTransformResult =
      iggy3d::validateWorldObject({&catalog, &invalidTransform});
  const iggy3d::ObjectValidationResult invalidRuntimeResult =
      iggy3d::validateWorldObject({&catalog, &invalidRuntime});

  return expect(!missingAssetResult.ok, "missing asset rejects") &&
         expect(missingAssetResult.status ==
                    iggy3d::ObjectValidationStatus::AssetReferenceMissing,
                "missing asset status") &&
         expect(!invalidTransformResult.ok, "invalid transform rejects") &&
         expect(invalidTransformResult.status ==
                    iggy3d::ObjectValidationStatus::InvalidTransform,
                "invalid transform status") &&
         expect(!invalidRuntimeResult.ok, "invalid runtime rejects") &&
         expect(invalidRuntimeResult.status ==
                    iggy3d::ObjectValidationStatus::InvalidRuntimeState,
                "invalid runtime status") &&
         expect(catalog.assets.size() == 4U, "catalog not mutated");
}

}  // namespace

int main() {
  const bool ok = stableNamesAreLowerSnake() &&
                  stableIdsAcceptLowerSnakeOnly() &&
                  builtInCatalogDefinesProxyShapeAssets() &&
                  invalidAssetDefinitionsRejectWithStableReasons() &&
                  catalogRejectsEmptyAndDuplicateIds() &&
                  assetResolutionSeparatesAssetFromObjectInstance() &&
                  invalidWorldObjectsRejectWithoutMutatingAssetCatalog();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
