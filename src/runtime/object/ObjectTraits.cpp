#include "runtime/object/ObjectTraits.hpp"

#include <array>
#include <cctype>
#include <cmath>

namespace iggy3d {
namespace {

template <typename Enum, std::size_t Count>
std::string_view enumName(Enum value,
                          const std::array<std::string_view, Count>& names,
                          std::string_view fallback) {
  const auto index = static_cast<std::size_t>(value);
  // branch-gate: BG-1082
  if (index >= names.size()) {
    return fallback;
  }
  return names[index];
}

bool finiteNonNegative(float value) {
  return std::isfinite(value) && value >= 0.0F;
}

bool finitePositive(float value) {
  return std::isfinite(value) && value > 0.0F;
}

bool validFriction(float value) {
  return std::isfinite(value) && value >= 0.0F && value <= 4.0F;
}

bool validRestitution(float value) {
  return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
}

bool validNormalizedPositive(float value) {
  return std::isfinite(value) && value > 0.0F && value <= 1.0F;
}

bool validAssetKind(ObjectAssetKind kind) {
  const auto index = static_cast<std::size_t>(kind);
  return index < 4U;
}

bool validPrimitiveShapeKind(ObjectPrimitiveShapeKind kind) {
  const auto index = static_cast<std::size_t>(kind);
  return index < 3U;
}

bool validCollisionShapeKind(ObjectCollisionShapeKind kind) {
  const auto index = static_cast<std::size_t>(kind);
  return index < 4U;
}

bool validPhysicsMotionKind(ObjectPhysicsMotionKind kind) {
  const auto index = static_cast<std::size_t>(kind);
  return index < 3U;
}

bool validMaterialKind(ObjectMaterialKind kind) {
  const auto index = static_cast<std::size_t>(kind);
  return index < 6U;
}

bool validInteractionVerb(ObjectInteractionVerb verb) {
  const auto index = static_cast<std::size_t>(verb);
  return index < 5U;
}

ObjectValidationResult validationFailure(ObjectValidationStatus status) {
  ObjectValidationResult result;
  result.status = status;
  result.reasonCode = objectValidationStatusName(status);
  return result;
}

ObjectValidationResult validationSuccess() {
  ObjectValidationResult result;
  result.ok = true;
  result.status = ObjectValidationStatus::Valid;
  result.reasonCode = objectValidationStatusName(result.status);
  return result;
}

ObjectValidationResult validatePhysics(const ObjectPhysicsProfile& physics) {
  // branch-gate: BG-1082
  if (!validPhysicsMotionKind(physics.motion) ||
      !validFriction(physics.friction) ||
      !validRestitution(physics.restitution)) {
    return validationFailure(ObjectValidationStatus::InvalidPhysicsProfile);
  }
  const bool dynamicBody = physics.motion == ObjectPhysicsMotionKind::Dynamic;
  // branch-gate: BG-1082
  if (dynamicBody && !finitePositive(physics.massKilograms)) {
    return validationFailure(ObjectValidationStatus::InvalidPhysicsProfile);
  }
  // branch-gate: BG-1082
  if (!dynamicBody && !finiteNonNegative(physics.massKilograms)) {
    return validationFailure(ObjectValidationStatus::InvalidPhysicsProfile);
  }
  return validationSuccess();
}

ObjectValidationResult validateMaterial(const ObjectMaterialTraits& material) {
  // branch-gate: BG-1082
  if (!validMaterialKind(material.kind) ||
      !validNormalizedPositive(material.durability)) {
    return validationFailure(ObjectValidationStatus::InvalidMaterialTraits);
  }
  return validationSuccess();
}

ObjectValidationResult validateInteraction(const ObjectInteractionTraits& interaction) {
  for (ObjectInteractionVerb verb : interaction.verbs) {
    // branch-gate: BG-1082
    if (!validInteractionVerb(verb)) {
      return validationFailure(ObjectValidationStatus::InvalidInteractionTraits);
    }
  }
  return validationSuccess();
}

ObjectAssetDefinition primitiveAsset(std::string id,
                                     std::string displayName,
                                     ObjectPrimitiveShapeKind shape,
                                     ObjectMaterialKind material,
                                     Vec3 size,
                                     bool blocksVision) {
  ObjectAssetDefinition asset;
  asset.id.value = std::move(id);
  asset.kind = ObjectAssetKind::PrimitiveShape;
  asset.primitiveShape.kind = shape;
  asset.primitiveShape.sizeMeters = size;
  asset.render.mesh.value =
      std::string{"generated_"} + std::string(objectPrimitiveShapeKindName(shape));
  asset.render.material.value =
      std::string{objectMaterialKindName(material)} + "_proxy";
  asset.collision.kind = ObjectCollisionShapeKind::Box;
  asset.collision.sizeMeters = size;
  asset.collision.blocksMovement = true;
  asset.collision.blocksVision = blocksVision;
  asset.physics.motion = ObjectPhysicsMotionKind::Static;
  asset.physics.massKilograms = 0.0F;
  asset.material.kind = material;
  asset.material.flammable = material == ObjectMaterialKind::Wood ||
                             material == ObjectMaterialKind::Cloth;
  asset.material.conductive = material == ObjectMaterialKind::Metal ||
                              material == ObjectMaterialKind::Water;
  asset.material.liquid = material == ObjectMaterialKind::Water;
  asset.interaction.verbs = {ObjectInteractionVerb::Inspect};
  asset.sound.impactTag = std::string{objectMaterialKindName(material)} + "_impact";
  asset.sound.scrapeTag = std::string{objectMaterialKindName(material)} + "_scrape";
  asset.sound.breakTag = std::string{objectMaterialKindName(material)} + "_break";
  asset.editor.displayName = std::move(displayName);
  return asset;
}

}  // namespace

std::string_view objectAssetKindName(ObjectAssetKind kind) {
  static constexpr std::array<std::string_view, 4> kNames{
      "primitive_shape",
      "generated_floor",
      "generated_wall",
      "prefab",
  };
  return enumName(kind, kNames, "unknown");
}

std::string_view objectPrimitiveShapeKindName(ObjectPrimitiveShapeKind kind) {
  static constexpr std::array<std::string_view, 3> kNames{
      "box",
      "slab",
      "pillar",
  };
  return enumName(kind, kNames, "unknown");
}

std::string_view objectCollisionShapeKindName(ObjectCollisionShapeKind kind) {
  static constexpr std::array<std::string_view, 4> kNames{
      "none",
      "box",
      "capsule",
      "mesh_proxy",
  };
  return enumName(kind, kNames, "unknown");
}

std::string_view objectPhysicsMotionKindName(ObjectPhysicsMotionKind kind) {
  static constexpr std::array<std::string_view, 3> kNames{
      "static",
      "dynamic",
      "kinematic",
  };
  return enumName(kind, kNames, "unknown");
}

std::string_view objectMaterialKindName(ObjectMaterialKind kind) {
  static constexpr std::array<std::string_view, 6> kNames{
      "stone",
      "wood",
      "metal",
      "cloth",
      "water",
      "fire",
  };
  return enumName(kind, kNames, "unknown");
}

std::string_view objectInteractionVerbName(ObjectInteractionVerb verb) {
  static constexpr std::array<std::string_view, 5> kNames{
      "inspect",
      "activate",
      "pickup",
      "push",
      "damage",
  };
  return enumName(verb, kNames, "unknown");
}

std::string_view objectValidationStatusName(ObjectValidationStatus status) {
  static constexpr std::array<std::string_view, 17> kNames{
      "object_valid",
      "object_missing_asset_definition",
      "object_catalog_empty",
      "object_missing_id",
      "object_invalid_id",
      "object_duplicate_id",
      "object_missing_render_asset",
      "object_invalid_primitive_shape",
      "object_invalid_collision_shape",
      "object_invalid_physics_profile",
      "object_invalid_material_traits",
      "object_invalid_interaction_traits",
      "object_missing_instance",
      "object_missing_asset_reference",
      "object_asset_reference_missing",
      "object_invalid_transform",
      "object_invalid_runtime_state",
  };
  return enumName(status, kNames, "object_invalid_id");
}

bool isValidObjectStableId(std::string_view id) {
  // branch-gate: BG-1082
  if (id.empty()) {
    return false;
  }
  for (const char c : id) {
    const auto uc = static_cast<unsigned char>(c);
    // branch-gate: BG-1082
    if (!(std::islower(uc) || std::isdigit(uc) || c == '_')) {
      return false;
    }
  }
  return true;
}

bool isPositiveFiniteObjectSize(Vec3 size) {
  return finitePositive(size.x) && finitePositive(size.y) && finitePositive(size.z);
}

ObjectValidationResult validateObjectAssetDefinition(
    const ObjectAssetDefinition* asset) {
  // branch-gate: BG-1082
  if (asset == nullptr) {
    return validationFailure(ObjectValidationStatus::MissingAssetDefinition);
  }
  ObjectValidationResult result;
  result.assetId = asset->id;
  // branch-gate: BG-1082
  if (asset->id.value.empty()) {
    result = validationFailure(ObjectValidationStatus::MissingId);
    result.assetId = asset->id;
    return result;
  }
  // branch-gate: BG-1082
  if (!isValidObjectStableId(asset->id.value) || !validAssetKind(asset->kind)) {
    result = validationFailure(ObjectValidationStatus::InvalidId);
    result.assetId = asset->id;
    return result;
  }
  // branch-gate: BG-1082
  if (asset->render.mesh.value.empty() || asset->render.material.value.empty()) {
    result = validationFailure(ObjectValidationStatus::MissingRenderAsset);
    result.assetId = asset->id;
    return result;
  }
  // branch-gate: BG-1082
  if (!validPrimitiveShapeKind(asset->primitiveShape.kind) ||
      !isPositiveFiniteObjectSize(asset->primitiveShape.sizeMeters)) {
    result = validationFailure(ObjectValidationStatus::InvalidPrimitiveShape);
    result.assetId = asset->id;
    return result;
  }
  // branch-gate: BG-1082
  if (!validCollisionShapeKind(asset->collision.kind) ||
      (asset->collision.kind != ObjectCollisionShapeKind::None &&
       !isPositiveFiniteObjectSize(asset->collision.sizeMeters))) {
    result = validationFailure(ObjectValidationStatus::InvalidCollisionShape);
    result.assetId = asset->id;
    return result;
  }
  result = validatePhysics(asset->physics);
  result.assetId = asset->id;
  // branch-gate: BG-1082
  if (!result.ok) {
    return result;
  }
  result = validateMaterial(asset->material);
  result.assetId = asset->id;
  // branch-gate: BG-1082
  if (!result.ok) {
    return result;
  }
  result = validateInteraction(asset->interaction);
  result.assetId = asset->id;
  return result;
}

ObjectValidationResult validateObjectAssetCatalog(
    const ObjectAssetCatalog& catalog) {
  // branch-gate: BG-1082
  if (catalog.assets.empty()) {
    return validationFailure(ObjectValidationStatus::CatalogEmpty);
  }
  for (std::size_t index = 0; index < catalog.assets.size(); ++index) {
    ObjectValidationResult result =
        validateObjectAssetDefinition(&catalog.assets[index]);
    result.index = index;
    // branch-gate: BG-1082
    if (!result.ok) {
      return result;
    }
    for (std::size_t compare = 0; compare < index; ++compare) {
      // branch-gate: BG-1082
      if (catalog.assets[compare].id.value == catalog.assets[index].id.value) {
        ObjectValidationResult duplicate =
            validationFailure(ObjectValidationStatus::DuplicateId);
        duplicate.index = index;
        duplicate.assetId = catalog.assets[index].id;
        return duplicate;
      }
    }
  }
  return validationSuccess();
}

ObjectAssetResolveResult resolveObjectAssetDefinition(
    const ObjectAssetResolveRequest& request) {
  ObjectAssetResolveResult result;
  // branch-gate: BG-1082
  if (!isValidObjectStableId(request.assetId)) {
    result.status = ObjectValidationStatus::InvalidId;
    result.reasonCode = objectValidationStatusName(result.status);
    return result;
  }
  // branch-gate: BG-1082
  if (request.catalog == nullptr || request.catalog->assets.empty()) {
    result.status = ObjectValidationStatus::CatalogEmpty;
    result.reasonCode = objectValidationStatusName(result.status);
    return result;
  }
  for (const ObjectAssetDefinition& asset : request.catalog->assets) {
    // branch-gate: BG-1082
    if (asset.id.value != request.assetId) {
      continue;
    }
    const ObjectValidationResult validation = validateObjectAssetDefinition(&asset);
    result.status = validation.status;
    result.reasonCode = validation.reasonCode;
    result.asset = asset;
    result.ok = validation.ok;
    return result;
  }
  result.status = ObjectValidationStatus::AssetReferenceMissing;
  result.reasonCode = objectValidationStatusName(result.status);
  return result;
}

ObjectValidationResult validateWorldObject(
    const WorldObjectValidationRequest& request) {
  // branch-gate: BG-1082
  if (request.object == nullptr) {
    return validationFailure(ObjectValidationStatus::MissingObject);
  }
  ObjectValidationResult result;
  result.objectId = request.object->id;
  result.assetId = request.object->assetId;
  // branch-gate: BG-1082
  if (!isValidObjectStableId(request.object->id.value)) {
    result = validationFailure(ObjectValidationStatus::InvalidId);
    result.objectId = request.object->id;
    result.assetId = request.object->assetId;
    return result;
  }
  // branch-gate: BG-1082
  if (request.object->assetId.value.empty()) {
    result = validationFailure(ObjectValidationStatus::MissingAssetReference);
    result.objectId = request.object->id;
    result.assetId = request.object->assetId;
    return result;
  }
  const ObjectAssetResolveResult asset = resolveObjectAssetDefinition(
      ObjectAssetResolveRequest{request.catalog, request.object->assetId.value});
  // branch-gate: BG-1082
  if (!asset.ok) {
    result = validationFailure(asset.status);
    result.objectId = request.object->id;
    result.assetId = request.object->assetId;
    return result;
  }
  // branch-gate: BG-1082
  if (!isFinite(request.object->transform) ||
      !hasPositiveFiniteScale(request.object->transform)) {
    result = validationFailure(ObjectValidationStatus::InvalidTransform);
    result.objectId = request.object->id;
    result.assetId = request.object->assetId;
    return result;
  }
  // branch-gate: BG-1082
  if (!finiteNonNegative(request.object->state.health)) {
    result = validationFailure(ObjectValidationStatus::InvalidRuntimeState);
    result.objectId = request.object->id;
    result.assetId = request.object->assetId;
    return result;
  }
  result = validationSuccess();
  result.objectId = request.object->id;
  result.assetId = request.object->assetId;
  return result;
}

WorldObject makeWorldObject(std::string_view objectId,
                            std::string_view assetId,
                            Transform3 transform) {
  WorldObject object;
  object.id.value = std::string(objectId);
  object.assetId.value = std::string(assetId);
  object.transform = transform;
  return object;
}

ObjectAssetCatalog makeBuiltInObjectAssetCatalog() {
  ObjectAssetCatalog catalog;
  catalog.assets.push_back(primitiveAsset("stone_block_proxy",
                                          "Stone Block",
                                          ObjectPrimitiveShapeKind::Box,
                                          ObjectMaterialKind::Stone,
                                          {1.0F, 1.0F, 1.0F},
                                          true));
  catalog.assets.push_back(primitiveAsset("stone_floor_slab",
                                          "Stone Floor Slab",
                                          ObjectPrimitiveShapeKind::Slab,
                                          ObjectMaterialKind::Stone,
                                          {1.0F, 0.15F, 1.0F},
                                          false));
  catalog.assets.push_back(primitiveAsset("stone_wall_panel",
                                          "Stone Wall Panel",
                                          ObjectPrimitiveShapeKind::Box,
                                          ObjectMaterialKind::Stone,
                                          {1.0F, 2.5F, 0.18F},
                                          true));
  ObjectAssetDefinition crate = primitiveAsset("wood_crate_proxy",
                                               "Wood Crate Proxy",
                                               ObjectPrimitiveShapeKind::Box,
                                               ObjectMaterialKind::Wood,
                                               {0.8F, 0.8F, 0.8F},
                                               false);
  crate.physics.motion = ObjectPhysicsMotionKind::Dynamic;
  crate.physics.massKilograms = 12.0F;
  crate.physics.affectedByGravity = true;
  crate.interaction.verbs = {ObjectInteractionVerb::Inspect,
                             ObjectInteractionVerb::Push,
                             ObjectInteractionVerb::Damage};
  crate.interaction.damageable = true;
  catalog.assets.push_back(crate);
  return catalog;
}

}  // namespace iggy3d
