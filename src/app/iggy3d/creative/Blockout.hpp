#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "content/authoring/EditableRoomDocument.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class ProductCreativeBlockoutPrimitiveKind : std::uint8_t {
  Floor,
  Wall,
  Object,
};

enum class ProductCreativeBlockoutFace : std::uint8_t {
  Top,
  Bottom,
  North,
  South,
  East,
  West,
};

struct ProductCreativeBlockoutPrimitiveRef {
  ProductCreativeBlockoutPrimitiveKind kind = ProductCreativeBlockoutPrimitiveKind::Floor;
  std::string id;
  std::uint32_t sourceIndex = 0;
};

struct ProductCreativeBlockoutFaceOverlay {
  ProductCreativeBlockoutPrimitiveRef primitive;
  ProductCreativeBlockoutFace face = ProductCreativeBlockoutFace::Top;
  Vec3 centerMeters;
  Vec3 normal;
  Vec3 tangentU;
  Vec3 tangentV;
  float widthMeters = 0.0F;
  float heightMeters = 0.0F;
  std::uint32_t gridLineCountU = 0;
  std::uint32_t gridLineCountV = 0;
  float yawDegrees = 0.0F;
  bool selectable = true;
  bool selected = false;
  bool hovered = false;
};

struct ProductCreativeMeasurementLabel {
  ProductCreativeBlockoutPrimitiveRef primitive;
  Vec3 worldPositionMeters;
  std::string label;
};

struct ProductCreativeBlockoutOverlay {
  bool ok = false;
  std::string status = "creative_blockout_not_built";
  std::string reasonCode = "creative_blockout_not_built";
  float gridStepMeters = 1.0F;
  std::vector<ProductCreativeBlockoutFaceOverlay> faces;
  std::vector<ProductCreativeMeasurementLabel> labels;
  std::uint64_t floorCount = 0;
  std::uint64_t wallCount = 0;
  std::uint64_t objectCount = 0;
  std::uint64_t selectedFaceCount = 0;
  std::uint64_t hoveredFaceCount = 0;
};

struct ProductCreativeBlockoutOverlayRequest {
  const EditableRoomDocument* document = nullptr;
  float gridStepMeters = 1.0F;
  ProductCreativeBlockoutPrimitiveRef selectedPrimitive;
  bool hasSelectedPrimitive = false;
  ProductCreativeBlockoutPrimitiveRef hoveredPrimitive;
  bool hasHoveredPrimitive = false;
};

std::string_view productCreativeBlockoutPrimitiveKindName(
    ProductCreativeBlockoutPrimitiveKind kind);
std::string_view productCreativeBlockoutFaceName(ProductCreativeBlockoutFace face);

ProductCreativeBlockoutOverlay buildProductCreativeBlockoutOverlay(
    const ProductCreativeBlockoutOverlayRequest& request);

}  // namespace iggy3d
