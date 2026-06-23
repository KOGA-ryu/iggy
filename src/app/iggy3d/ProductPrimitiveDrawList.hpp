#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

struct DebugProjectionResult;
struct SceneProjectionResult;

enum class ProductPrimitiveDrawKind : std::uint8_t {
  PlayerMarker,
  NpcMarker,
  PickupMarker,
  InteractableMarker,
  ObjectiveMarker,
  TacticalMarker,
  DebugMarker,
  PlayerFocusIndicator,
};

struct ProductPrimitiveColor {
  std::uint8_t r = 255;
  std::uint8_t g = 255;
  std::uint8_t b = 255;
};

struct ProductPrimitiveDrawItem {
  ProductPrimitiveDrawKind kind = ProductPrimitiveDrawKind::DebugMarker;
  EntityId entityId;
  std::string stableName;
  Vec3 worldPosition;
  Aabb3 worldBounds;
  bool visible = false;
  bool targetable = false;
  bool interactable = false;
  bool tactical = false;
  ProductPrimitiveColor color;
  float markerSize = 14.0F;
};

struct ProductPrimitiveDrawList {
  std::vector<ProductPrimitiveDrawItem> items;
  bool gridVisible = false;
  bool roomVisible = false;
  bool playerVisible = false;
  bool objectiveVisible = false;
  bool playerFocusIndicatorVisible = false;
  std::uint64_t itemCount = 0;
  std::uint64_t playerCount = 0;
  std::uint64_t targetMarkerCount = 0;
  std::uint64_t objectiveMarkerCount = 0;
  std::uint64_t debugMarkerCount = 0;
};

ProductPrimitiveDrawList buildProductPrimitiveDrawList(
    const SceneProjectionResult* scene,
    const DebugProjectionResult* debug);

}  // namespace iggy3d
