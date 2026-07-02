#include "app/iggy3d/creative/Block.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

namespace iggy3d::creative {
namespace {

struct KindDescriptor {
  Kind kind = Kind::Floor;
  std::string_view name = "floor";
};

struct FaceDescriptor {
  Face face = Face::Top;
  std::string_view name = "top";
};

struct BoxFaceDescriptor {
  Face face = Face::Top;
  Vec3 normal;
  Vec3 tangentU;
  Vec3 tangentV;
};

constexpr std::array kKindDescriptors{
    KindDescriptor{
        Kind::Floor, "floor"},
    KindDescriptor{
        Kind::Wall, "wall"},
    KindDescriptor{
        Kind::Object, "object"},
};

constexpr std::array kFaceDescriptors{
    FaceDescriptor{Face::Top, "top"},
    FaceDescriptor{Face::Bottom, "bottom"},
    FaceDescriptor{Face::North, "north"},
    FaceDescriptor{Face::South, "south"},
    FaceDescriptor{Face::East, "east"},
    FaceDescriptor{Face::West, "west"},
};

constexpr std::array kBoxFaceDescriptors{
    BoxFaceDescriptor{Face::Top,
                                     {0.0F, 1.0F, 0.0F},
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 0.0F, 1.0F}},
    BoxFaceDescriptor{Face::Bottom,
                                     {0.0F, -1.0F, 0.0F},
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 0.0F, 1.0F}},
    BoxFaceDescriptor{Face::North,
                                     {0.0F, 0.0F, -1.0F},
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 1.0F, 0.0F}},
    BoxFaceDescriptor{Face::South,
                                     {0.0F, 0.0F, 1.0F},
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 1.0F, 0.0F}},
    BoxFaceDescriptor{Face::East,
                                     {1.0F, 0.0F, 0.0F},
                                     {0.0F, 0.0F, 1.0F},
                                     {0.0F, 1.0F, 0.0F}},
    BoxFaceDescriptor{Face::West,
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

Vec3 faceOffset(Face face, Vec3 size) {
  switch (face) {  // branch-gate: BG-1222
    case Face::Top:
      return {0.0F, size.y * 0.5F, 0.0F};
    case Face::Bottom:
      return {0.0F, -size.y * 0.5F, 0.0F};
    case Face::North:
      return {0.0F, 0.0F, -size.z * 0.5F};
    case Face::South:
      return {0.0F, 0.0F, size.z * 0.5F};
    case Face::East:
      return {size.x * 0.5F, 0.0F, 0.0F};
    case Face::West:
      return {-size.x * 0.5F, 0.0F, 0.0F};
  }
  return {};
}

float boxFaceWidth(Face face, Vec3 size) {
  switch (face) {  // branch-gate: BG-1222
    case Face::Top:
    case Face::Bottom:
    case Face::North:
    case Face::South:
      return size.x;
    case Face::East:
    case Face::West:
      return size.z;
  }
  return 0.0F;
}

float boxFaceHeight(Face face, Vec3 size) {
  switch (face) {  // branch-gate: BG-1222
    case Face::Top:
    case Face::Bottom:
      return size.z;
    case Face::North:
    case Face::South:
    case Face::East:
    case Face::West:
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

bool primitiveRefsMatch(const Ref& lhs,
                        const Ref& rhs) {
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

void applySelectionFlags(FaceOverlay& face,
                         const BlockRequest& request,
                         BlockView& overlay) {
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

void appendLabel(BlockView& overlay,
                 Ref primitive,
                 Vec3 position,
                 std::string label) {
  MeasureLabel out;
  out.primitive = std::move(primitive);
  out.worldPositionMeters = position;
  out.label = std::move(label);
  overlay.labels.push_back(std::move(out));
}

void appendFace(BlockView& overlay,
                const BlockRequest& request,
                Ref primitive,
                Face face,
                Vec3 center,
                Vec3 normal,
                Vec3 tangentU,
                Vec3 tangentV,
                float widthMeters,
                float heightMeters,
                float yawDegrees = 0.0F) {
  FaceOverlay out;
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

void appendBoxFaces(BlockView& overlay,
                    const BlockRequest& request,
                    Ref primitive,
                    Vec3 center,
                    Vec3 size,
                    float yawDegrees) {
  for (const BoxFaceDescriptor& descriptor :
       kBoxFaceDescriptors) {
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

void appendFloorOverlay(BlockView& overlay,
                        const BlockRequest& request,
                        const EditableRoomFloor& floor,
                        std::uint32_t sourceIndex) {
  const Ref primitive{
      Kind::Floor,
      floor.id,
      sourceIndex,
  };
  appendFace(overlay,
             request,
             primitive,
             Face::Top,
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

void appendWallOverlay(BlockView& overlay,
                       const BlockRequest& request,
                       const EditableRoomWall& wall,
                       std::uint32_t sourceIndex) {
  const Ref primitive{
      Kind::Wall,
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
             Face::North,
             center + normal * (wall.thicknessMeters * 0.5F),
             normal,
             tangent,
             {0.0F, 1.0F, 0.0F},
             length,
             wall.heightMeters);
  appendFace(overlay,
             request,
             primitive,
             Face::South,
             center - normal * (wall.thicknessMeters * 0.5F),
             normal * -1.0F,
             tangent,
             {0.0F, 1.0F, 0.0F},
             length,
             wall.heightMeters);
  appendFace(overlay,
             request,
             primitive,
             Face::Top,
             center + Vec3{0.0F, wall.heightMeters * 0.5F, 0.0F},
             {0.0F, 1.0F, 0.0F},
             tangent,
             normal,
             length,
             wall.thicknessMeters);
  appendFace(overlay,
             request,
             primitive,
             Face::Bottom,
             center - Vec3{0.0F, wall.heightMeters * 0.5F, 0.0F},
             {0.0F, -1.0F, 0.0F},
             tangent,
             normal,
             length,
             wall.thicknessMeters);
  appendFace(overlay,
             request,
             primitive,
             Face::East,
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
             Face::West,
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

void appendObjectOverlay(BlockView& overlay,
                         const BlockRequest& request,
                         const EditableRoomObject& object,
                         std::uint32_t sourceIndex) {
  const Ref primitive{
      Kind::Object,
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

std::string_view kindName(
    Kind kind) {
  for (const KindDescriptor& descriptor :
       kKindDescriptors) {
    // branch-gate: BG-1222
    if (descriptor.kind == kind) {
      return descriptor.name;
    }
  }
  return "unknown";
}

std::string_view faceName(
    Face face) {
  for (const FaceDescriptor& descriptor :
       kFaceDescriptors) {
    // branch-gate: BG-1222
    if (descriptor.face == face) {
      return descriptor.name;
    }
  }
  return "unknown";
}

BlockView buildBlockView(
    const BlockRequest& request) {
  BlockView overlay;
  overlay.gridStepMeters = request.gridStepMeters;
  // branch-gate: BG-1222
  if (request.document == nullptr) {
    overlay.status = "missing_document";
    overlay.reasonCode = "missing_document";
    return overlay;
  }
  // branch-gate: BG-1222
  if (!positiveFinite(request.gridStepMeters)) {
    overlay.status = "invalid_grid_step";
    overlay.reasonCode = "invalid_grid_step";
    return overlay;
  }

  overlay.floorCount = request.document->floors.size();
  overlay.wallCount = request.document->walls.size();
  overlay.objectCount = request.document->objects.size();
  overlay.faces.reserve(overlay.floorCount +
                        overlay.wallCount * kBoxFaceDescriptors.size() +
                        overlay.objectCount * kBoxFaceDescriptors.size());
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
  overlay.status = "block_ready";
  overlay.reasonCode = "block_ready";
  return overlay;
}

}  // namespace iggy3d::creative
