#include "app/iggy3d/creative/Blockout.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

namespace iggy3d {
namespace {

struct ProductCreativePrimitiveKindDescriptor {
  ProductCreativeBlockoutPrimitiveKind kind = ProductCreativeBlockoutPrimitiveKind::Floor;
  std::string_view name = "floor";
};

struct ProductCreativeFaceDescriptor {
  ProductCreativeBlockoutFace face = ProductCreativeBlockoutFace::Top;
  std::string_view name = "top";
};

struct ProductCreativeBoxFaceDescriptor {
  ProductCreativeBlockoutFace face = ProductCreativeBlockoutFace::Top;
  Vec3 normal;
  Vec3 tangentU;
  Vec3 tangentV;
};

constexpr std::array kProductCreativePrimitiveKindDescriptors{
    ProductCreativePrimitiveKindDescriptor{
        ProductCreativeBlockoutPrimitiveKind::Floor, "floor"},
    ProductCreativePrimitiveKindDescriptor{
        ProductCreativeBlockoutPrimitiveKind::Wall, "wall"},
    ProductCreativePrimitiveKindDescriptor{
        ProductCreativeBlockoutPrimitiveKind::Object, "object"},
};

constexpr std::array kProductCreativeFaceDescriptors{
    ProductCreativeFaceDescriptor{ProductCreativeBlockoutFace::Top, "top"},
    ProductCreativeFaceDescriptor{ProductCreativeBlockoutFace::Bottom, "bottom"},
    ProductCreativeFaceDescriptor{ProductCreativeBlockoutFace::North, "north"},
    ProductCreativeFaceDescriptor{ProductCreativeBlockoutFace::South, "south"},
    ProductCreativeFaceDescriptor{ProductCreativeBlockoutFace::East, "east"},
    ProductCreativeFaceDescriptor{ProductCreativeBlockoutFace::West, "west"},
};

constexpr std::array kProductCreativeBoxFaceDescriptors{
    ProductCreativeBoxFaceDescriptor{ProductCreativeBlockoutFace::Top,
                                     {0.0F, 1.0F, 0.0F},
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 0.0F, 1.0F}},
    ProductCreativeBoxFaceDescriptor{ProductCreativeBlockoutFace::Bottom,
                                     {0.0F, -1.0F, 0.0F},
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 0.0F, 1.0F}},
    ProductCreativeBoxFaceDescriptor{ProductCreativeBlockoutFace::North,
                                     {0.0F, 0.0F, -1.0F},
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 1.0F, 0.0F}},
    ProductCreativeBoxFaceDescriptor{ProductCreativeBlockoutFace::South,
                                     {0.0F, 0.0F, 1.0F},
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 1.0F, 0.0F}},
    ProductCreativeBoxFaceDescriptor{ProductCreativeBlockoutFace::East,
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 0.0F, 1.0F},
                                     {0.0F, 1.0F, 0.0F}},
    ProductCreativeBoxFaceDescriptor{ProductCreativeBlockoutFace::West,
                                     {-1.0F, 0.0F, 0.0F},
                                     {0.0F, 0.0F, 1.0F},
                                     {0.0F, 1.0F, 0.0F}},
};

bool positiveFinite(float value) {
  return std::isfinite(value) && value > 0.0F;
}

float horizontalLength(Vec3 value) {
  return std::sqrt(value.x * value.x + value.z * value.z);
}

Vec3 horizontalUnit(Vec3 value) {
  const float length = horizontalLength(value);
  // branch-gate: BG-1222
  if (!positiveFinite(length)) {
    return {1.0F, 0.0F, 0.0F};
  }
  return {value.x / length, 0.0F, value.z / length};
}

Vec3 wallNormalFromTangent(Vec3 tangent) {
  return {-tangent.z, 0.0F, tangent.x};
}

Vec3 faceOffset(ProductCreativeBlockoutFace face, Vec3 size) {
  switch (face) {  // branch-gate: BG-1222
    case ProductCreativeBlockoutFace::Top:
      return {0.0F, size.y * 0.5F, 0.0F};
    case ProductCreativeBlockoutFace::Bottom:
      return {0.0F, -size.y * 0.5F, 0.0F};
    case ProductCreativeBlockoutFace::North:
      return {0.0F, 0.0F, -size.z * 0.5F};
    case ProductCreativeBlockoutFace::South:
      return {0.0F, 0.0F, size.z * 0.5F};
    case ProductCreativeBlockoutFace::East:
      return {size.x * 0.5F, 0.0F, 0.0F};
    case ProductCreativeBlockoutFace::West:
      return {-size.x * 0.5F, 0.0F, 0.0F};
  }
  return {};
}

float boxFaceWidth(ProductCreativeBlockoutFace face, Vec3 size) {
  switch (face) {  // branch-gate: BG-1222
    case ProductCreativeBlockoutFace::Top:
    case ProductCreativeBlockoutFace::Bottom:
    case ProductCreativeBlockoutFace::North:
    case ProductCreativeBlockoutFace::South:
      return size.x;
    case ProductCreativeBlockoutFace::East:
    case ProductCreativeBlockoutFace::West:
      return size.z;
  }
  return 0.0F;
}

float boxFaceHeight(ProductCreativeBlockoutFace face, Vec3 size) {
  switch (face) {  // branch-gate: BG-1222
    case ProductCreativeBlockoutFace::Top:
    case ProductCreativeBlockoutFace::Bottom:
      return size.z;
    case ProductCreativeBlockoutFace::North:
    case ProductCreativeBlockoutFace::South:
    case ProductCreativeBlockoutFace::East:
    case ProductCreativeBlockoutFace::West:
      return size.y;
  }
  return 0.0F;
}

std::uint32_t gridLineCount(float meters, float gridStepMeters) {
  // branch-gate: BG-1222
  if (!positiveFinite(meters) || !positiveFinite(gridStepMeters)) {
    return 0;
  }
  return static_cast<std::uint32_t>(std::floor(meters / gridStepMeters)) + 1U;
}

std::string meterLabel(std::string_view prefix, float meters) {
  std::ostringstream stream;
  stream << prefix << ' ' << std::fixed << std::setprecision(1) << meters << 'm';
  return stream.str();
}

bool primitiveRefsMatch(const ProductCreativeBlockoutPrimitiveRef& lhs,
                        const ProductCreativeBlockoutPrimitiveRef& rhs) {
  // branch-gate: BG-1222
  if (lhs.kind != rhs.kind) {
    return false;
  }
  // branch-gate: BG-1222
  if (!lhs.id.empty() || !rhs.id.empty()) {
    return lhs.id == rhs.id;
  }
  return lhs.sourceIndex == rhs.sourceIndex;
}

void applySelectionFlags(ProductCreativeBlockoutFaceOverlay& face,
                         const ProductCreativeBlockoutOverlayRequest& request,
                         ProductCreativeBlockoutOverlay& overlay) {
  face.selected = request.hasSelectedPrimitive &&
                  primitiveRefsMatch(face.primitive, request.selectedPrimitive);
  face.hovered = request.hasHoveredPrimitive &&
                 primitiveRefsMatch(face.primitive, request.hoveredPrimitive);
  // branch-gate: BG-1222
  if (face.selected) {
    ++overlay.selectedFaceCount;
  }
  // branch-gate: BG-1222
  if (face.hovered) {
    ++overlay.hoveredFaceCount;
  }
}

void appendLabel(ProductCreativeBlockoutOverlay& overlay,
                 ProductCreativeBlockoutPrimitiveRef primitive,
                 Vec3 position,
                 std::string label) {
  ProductCreativeMeasurementLabel out;
  out.primitive = std::move(primitive);
  out.worldPositionMeters = position;
  out.label = std::move(label);
  overlay.labels.push_back(std::move(out));
}

void appendFace(ProductCreativeBlockoutOverlay& overlay,
                const ProductCreativeBlockoutOverlayRequest& request,
                ProductCreativeBlockoutPrimitiveRef primitive,
                ProductCreativeBlockoutFace face,
                Vec3 center,
                Vec3 normal,
                Vec3 tangentU,
                Vec3 tangentV,
                float widthMeters,
                float heightMeters,
                float yawDegrees = 0.0F) {
  ProductCreativeBlockoutFaceOverlay out;
  out.primitive = std::move(primitive);
  out.face = face;
  out.centerMeters = center;
  out.normal = normal;
  out.tangentU = tangentU;
  out.tangentV = tangentV;
  out.widthMeters = widthMeters;
  out.heightMeters = heightMeters;
  out.gridLineCountU = gridLineCount(widthMeters, request.gridStepMeters);
  out.gridLineCountV = gridLineCount(heightMeters, request.gridStepMeters);
  out.yawDegrees = yawDegrees;
  applySelectionFlags(out, request, overlay);
  overlay.faces.push_back(std::move(out));
}

void appendBoxFaces(ProductCreativeBlockoutOverlay& overlay,
                    const ProductCreativeBlockoutOverlayRequest& request,
                    ProductCreativeBlockoutPrimitiveRef primitive,
                    Vec3 center,
                    Vec3 size,
                    float yawDegrees) {
  for (const ProductCreativeBoxFaceDescriptor& descriptor :
       kProductCreativeBoxFaceDescriptors) {
    appendFace(overlay,
               request,
               primitive,
               descriptor.face,
               center + faceOffset(descriptor.face, size),
               descriptor.normal,
               descriptor.tangentU,
               descriptor.tangentV,
               boxFaceWidth(descriptor.face, size),
               boxFaceHeight(descriptor.face, size),
               yawDegrees);
  }
}

void appendFloorOverlay(ProductCreativeBlockoutOverlay& overlay,
                        const ProductCreativeBlockoutOverlayRequest& request,
                        const EditableRoomFloor& floor,
                        std::uint32_t sourceIndex) {
  const ProductCreativeBlockoutPrimitiveRef primitive{
      ProductCreativeBlockoutPrimitiveKind::Floor,
      floor.id,
      sourceIndex,
  };
  appendFace(overlay,
             request,
             primitive,
             ProductCreativeBlockoutFace::Top,
             floor.centerMeters + Vec3{0.0F, floor.sizeMeters.y * 0.5F, 0.0F},
             {0.0F, 1.0F, 0.0F},
             {1.0F, 0.0F, 0.0F},
             {0.0F, 0.0F, 1.0F},
             floor.sizeMeters.x,
             floor.sizeMeters.z);
  appendLabel(overlay,
              primitive,
              floor.centerMeters + Vec3{0.0F, floor.sizeMeters.y + 0.05F, 0.0F},
              meterLabel("W", floor.sizeMeters.x));
  appendLabel(overlay,
              primitive,
              floor.centerMeters + Vec3{0.0F, floor.sizeMeters.y + 0.10F, 0.0F},
              meterLabel("D", floor.sizeMeters.z));
}

void appendWallOverlay(ProductCreativeBlockoutOverlay& overlay,
                       const ProductCreativeBlockoutOverlayRequest& request,
                       const EditableRoomWall& wall,
                       std::uint32_t sourceIndex) {
  const ProductCreativeBlockoutPrimitiveRef primitive{
      ProductCreativeBlockoutPrimitiveKind::Wall,
      wall.id,
      sourceIndex,
  };
  const Vec3 delta = wall.endMeters - wall.startMeters;
  const Vec3 tangent = horizontalUnit(delta);
  const Vec3 normal = wallNormalFromTangent(tangent);
  const float length = horizontalLength(delta);
  const Vec3 center = (wall.startMeters + wall.endMeters) * 0.5F +
                      Vec3{0.0F, wall.bottomY + wall.heightMeters * 0.5F, 0.0F};
  appendFace(overlay,
             request,
             primitive,
             ProductCreativeBlockoutFace::North,
             center + normal * (wall.thicknessMeters * 0.5F),
             normal,
             tangent,
             {0.0F, 1.0F, 0.0F},
             length,
             wall.heightMeters);
  appendFace(overlay,
             request,
             primitive,
             ProductCreativeBlockoutFace::South,
             center - normal * (wall.thicknessMeters * 0.5F),
             normal * -1.0F,
             tangent,
             {0.0F, 1.0F, 0.0F},
             length,
             wall.heightMeters);
  appendFace(overlay,
             request,
             primitive,
             ProductCreativeBlockoutFace::Top,
             center + Vec3{0.0F, wall.heightMeters * 0.5F, 0.0F},
             {0.0F, 1.0F, 0.0F},
             tangent,
             normal,
             length,
             wall.thicknessMeters);
  appendFace(overlay,
             request,
             primitive,
             ProductCreativeBlockoutFace::Bottom,
             center - Vec3{0.0F, wall.heightMeters * 0.5F, 0.0F},
             {0.0F, -1.0F, 0.0F},
             tangent,
             normal,
             length,
             wall.thicknessMeters);
  appendFace(overlay,
             request,
             primitive,
             ProductCreativeBlockoutFace::East,
             wall.endMeters + Vec3{0.0F,
                                   wall.bottomY + wall.heightMeters * 0.5F,
                                   0.0F},
             tangent,
             normal,
             {0.0F, 1.0F, 0.0F},
             wall.thicknessMeters,
             wall.heightMeters);
  appendFace(overlay,
             request,
             primitive,
             ProductCreativeBlockoutFace::West,
             wall.startMeters + Vec3{0.0F,
                                     wall.bottomY + wall.heightMeters * 0.5F,
                                     0.0F},
             tangent * -1.0F,
             normal,
             {0.0F, 1.0F, 0.0F},
             wall.thicknessMeters,
             wall.heightMeters);
  appendLabel(overlay,
              primitive,
              center + Vec3{0.0F, wall.heightMeters * 0.5F + 0.10F, 0.0F},
              meterLabel("W", length));
  appendLabel(overlay,
              primitive,
              center + normal * (wall.thicknessMeters * 0.5F + 0.05F),
              meterLabel("H", wall.heightMeters));
  appendLabel(overlay,
              primitive,
              center - normal * (wall.thicknessMeters * 0.5F + 0.05F),
              meterLabel("D", wall.thicknessMeters));
}

void appendObjectOverlay(ProductCreativeBlockoutOverlay& overlay,
                         const ProductCreativeBlockoutOverlayRequest& request,
                         const EditableRoomObject& object,
                         std::uint32_t sourceIndex) {
  const ProductCreativeBlockoutPrimitiveRef primitive{
      ProductCreativeBlockoutPrimitiveKind::Object,
      object.id,
      sourceIndex,
  };
  appendBoxFaces(overlay,
                 request,
                 primitive,
                 object.positionMeters,
                 object.sizeMeters,
                 object.yawDegrees);
  appendLabel(overlay,
              primitive,
              object.positionMeters + Vec3{0.0F, object.sizeMeters.y * 0.5F + 0.10F, 0.0F},
              meterLabel("W", object.sizeMeters.x));
  appendLabel(overlay,
              primitive,
              object.positionMeters + Vec3{0.0F, object.sizeMeters.y * 0.5F + 0.20F, 0.0F},
              meterLabel("H", object.sizeMeters.y));
  appendLabel(overlay,
              primitive,
              object.positionMeters + Vec3{0.0F, object.sizeMeters.y * 0.5F + 0.30F, 0.0F},
              meterLabel("D", object.sizeMeters.z));
}

}  // namespace

std::string_view productCreativeBlockoutPrimitiveKindName(
    ProductCreativeBlockoutPrimitiveKind kind) {
  for (const ProductCreativePrimitiveKindDescriptor& descriptor :
       kProductCreativePrimitiveKindDescriptors) {
    // branch-gate: BG-1222
    if (descriptor.kind == kind) {
      return descriptor.name;
    }
  }
  return "unknown";
}

std::string_view productCreativeBlockoutFaceName(
    ProductCreativeBlockoutFace face) {
  for (const ProductCreativeFaceDescriptor& descriptor :
       kProductCreativeFaceDescriptors) {
    // branch-gate: BG-1222
    if (descriptor.face == face) {
      return descriptor.name;
    }
  }
  return "unknown";
}

ProductCreativeBlockoutOverlay buildProductCreativeBlockoutOverlay(
    const ProductCreativeBlockoutOverlayRequest& request) {
  ProductCreativeBlockoutOverlay overlay;
  overlay.gridStepMeters = request.gridStepMeters;
  // branch-gate: BG-1222
  if (request.document == nullptr) {
    overlay.status = "creative_blockout_missing_document";
    overlay.reasonCode = "creative_blockout_missing_document";
    return overlay;
  }
  // branch-gate: BG-1222
  if (!positiveFinite(request.gridStepMeters)) {
    overlay.status = "creative_blockout_invalid_grid_step";
    overlay.reasonCode = "creative_blockout_invalid_grid_step";
    return overlay;
  }

  overlay.floorCount = request.document->floors.size();
  overlay.wallCount = request.document->walls.size();
  overlay.objectCount = request.document->objects.size();
  overlay.faces.reserve(overlay.floorCount +
                        overlay.wallCount * kProductCreativeBoxFaceDescriptors.size() +
                        overlay.objectCount * kProductCreativeBoxFaceDescriptors.size());
  overlay.labels.reserve(overlay.floorCount * 2U + overlay.wallCount * 3U +
                         overlay.objectCount * 3U);

  for (std::size_t index = 0; index < request.document->floors.size(); ++index) {
    appendFloorOverlay(overlay,
                       request,
                       request.document->floors[index],
                       static_cast<std::uint32_t>(index));
  }
  for (std::size_t index = 0; index < request.document->walls.size(); ++index) {
    appendWallOverlay(overlay,
                      request,
                      request.document->walls[index],
                      static_cast<std::uint32_t>(index));
  }
  for (std::size_t index = 0; index < request.document->objects.size(); ++index) {
    appendObjectOverlay(overlay,
                        request,
                        request.document->objects[index],
                        static_cast<std::uint32_t>(index));
  }

  overlay.ok = true;
  overlay.status = "creative_blockout_ready";
  overlay.reasonCode = "creative_blockout_ready";
  return overlay;
}

}  // namespace iggy3d
