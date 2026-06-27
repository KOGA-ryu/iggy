#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

struct AssetId {
  std::string value;
};

struct WorldObjectId {
  std::string value;
};

struct MeshId {
  std::string value;
};

struct MaterialId {
  std::string value;
};

enum class ObjectAssetKind : std::uint8_t {
  PrimitiveShape,
  GeneratedFloor,
  GeneratedWall,
  Prefab,
};

enum class ObjectPrimitiveShapeKind : std::uint8_t {
  Box,
  Slab,
  Pillar,
};

enum class ObjectCollisionShapeKind : std::uint8_t {
  None,
  Box,
  Capsule,
  MeshProxy,
};

enum class ObjectPhysicsMotionKind : std::uint8_t {
  Static,
  Dynamic,
  Kinematic,
};

enum class ObjectMaterialKind : std::uint8_t {
  Stone,
  Wood,
  Metal,
  Cloth,
  Water,
  Fire,
};

enum class ObjectInteractionVerb : std::uint8_t {
  Inspect,
  Activate,
  Pickup,
  Push,
  Damage,
};

struct ObjectPrimitiveShapeTemplate {
  ObjectPrimitiveShapeKind kind = ObjectPrimitiveShapeKind::Box;
  Vec3 sizeMeters = {1.0F, 1.0F, 1.0F};
};

struct ObjectCollisionShapeTemplate {
  ObjectCollisionShapeKind kind = ObjectCollisionShapeKind::Box;
  Vec3 sizeMeters = {1.0F, 1.0F, 1.0F};
  bool blocksMovement = true;
  bool blocksVision = false;
};

struct ObjectPhysicsProfile {
  ObjectPhysicsMotionKind motion = ObjectPhysicsMotionKind::Static;
  float massKilograms = 0.0F;
  float friction = 0.75F;
  float restitution = 0.0F;
  bool affectedByGravity = false;
};

struct ObjectMaterialTraits {
  ObjectMaterialKind kind = ObjectMaterialKind::Stone;
  bool flammable = false;
  bool conductive = false;
  bool liquid = false;
  float durability = 1.0F;
};

struct ObjectInteractionTraits {
  std::vector<ObjectInteractionVerb> verbs;
  bool selectable = true;
  bool damageable = false;
};

struct ObjectSoundProfile {
  std::string impactTag = "none";
  std::string scrapeTag = "none";
  std::string breakTag = "none";
};

struct ObjectRenderProfile {
  MeshId mesh;
  MaterialId material;
};

struct ObjectSaveFlags {
  bool persistent = true;
  bool saveTransform = true;
  bool saveRuntimeState = true;
};

struct ObjectEditorMetadata {
  std::string paletteGroup = "shapes";
  std::string displayName;
  bool placeable = true;
  bool rotatable = true;
  bool scalable = false;
};

struct ObjectAssetDefinition {
  AssetId id;
  ObjectAssetKind kind = ObjectAssetKind::PrimitiveShape;
  ObjectPrimitiveShapeTemplate primitiveShape;
  ObjectRenderProfile render;
  ObjectCollisionShapeTemplate collision;
  ObjectPhysicsProfile physics;
  ObjectMaterialTraits material;
  ObjectInteractionTraits interaction;
  ObjectSoundProfile sound;
  ObjectSaveFlags save;
  ObjectEditorMetadata editor;
};

struct ObjectAssetCatalog {
  std::vector<ObjectAssetDefinition> assets;
};

struct WorldObjectRuntimeState {
  bool active = true;
  bool visible = true;
  bool selected = false;
  bool burning = false;
  float health = 1.0F;
};

struct WorldObject {
  WorldObjectId id;
  AssetId assetId;
  Transform3 transform;
  WorldObjectRuntimeState state;
};

enum class ObjectValidationStatus : std::uint8_t {
  Valid,
  MissingAssetDefinition,
  CatalogEmpty,
  MissingId,
  InvalidId,
  DuplicateId,
  MissingRenderAsset,
  InvalidPrimitiveShape,
  InvalidCollisionShape,
  InvalidPhysicsProfile,
  InvalidMaterialTraits,
  InvalidInteractionTraits,
  MissingObject,
  MissingAssetReference,
  AssetReferenceMissing,
  InvalidTransform,
  InvalidRuntimeState,
};

struct ObjectValidationResult {
  bool ok = false;
  ObjectValidationStatus status = ObjectValidationStatus::MissingAssetDefinition;
  std::string_view reasonCode = "object_missing_asset_definition";
  std::size_t index = 0;
  AssetId assetId;
  WorldObjectId objectId;
};

struct ObjectAssetResolveRequest {
  const ObjectAssetCatalog* catalog = nullptr;
  std::string_view assetId;
};

struct ObjectAssetResolveResult {
  bool ok = false;
  ObjectValidationStatus status = ObjectValidationStatus::AssetReferenceMissing;
  std::string_view reasonCode = "object_asset_reference_missing";
  ObjectAssetDefinition asset;
};

struct WorldObjectValidationRequest {
  const ObjectAssetCatalog* catalog = nullptr;
  const WorldObject* object = nullptr;
};

std::string_view objectAssetKindName(ObjectAssetKind kind);
std::string_view objectPrimitiveShapeKindName(ObjectPrimitiveShapeKind kind);
std::string_view objectCollisionShapeKindName(ObjectCollisionShapeKind kind);
std::string_view objectPhysicsMotionKindName(ObjectPhysicsMotionKind kind);
std::string_view objectMaterialKindName(ObjectMaterialKind kind);
std::string_view objectInteractionVerbName(ObjectInteractionVerb verb);
std::string_view objectValidationStatusName(ObjectValidationStatus status);

bool isValidObjectStableId(std::string_view id);
bool isPositiveFiniteObjectSize(Vec3 size);
ObjectValidationResult validateObjectAssetDefinition(
    const ObjectAssetDefinition* asset);
ObjectValidationResult validateObjectAssetCatalog(
    const ObjectAssetCatalog& catalog);
ObjectAssetResolveResult resolveObjectAssetDefinition(
    const ObjectAssetResolveRequest& request);
ObjectValidationResult validateWorldObject(
    const WorldObjectValidationRequest& request);
WorldObject makeWorldObject(std::string_view objectId,
                            std::string_view assetId,
                            Transform3 transform);
ObjectAssetCatalog makeBuiltInObjectAssetCatalog();

}  // namespace iggy3d
