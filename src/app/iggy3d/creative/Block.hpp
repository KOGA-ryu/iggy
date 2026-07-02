#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "content/authoring/EditableRoomDocument.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d::creative {

enum class Kind : std::uint8_t {
  Floor,
  Wall,
  Object,
};

enum class Face : std::uint8_t {
  Top,
  Bottom,
  North,
  South,
  East,
  West,
};

struct Ref {
  Kind kind = Kind::Floor;
  std::string id;
  std::uint32_t sourceIndex = 0;
};

struct FaceOverlay {
  Ref primitive;
  Face face = Face::Top;
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

struct MeasureLabel {
  Ref primitive;
  Vec3 worldPositionMeters;
  std::string label;
};

struct BlockView {
  bool ok = false;
  std::string status = "block_not_built";
  std::string reasonCode = "block_not_built";
  float gridStepMeters = 1.0F;
  std::vector<FaceOverlay> faces;
  std::vector<MeasureLabel> labels;
  std::uint64_t floorCount = 0;
  std::uint64_t wallCount = 0;
  std::uint64_t objectCount = 0;
  std::uint64_t selectedFaceCount = 0;
  std::uint64_t hoveredFaceCount = 0;
};

struct BlockRequest {
  const EditableRoomDocument* document = nullptr;
  float gridStepMeters = 1.0F;
  Ref selectedPrimitive;
  bool hasSelectedPrimitive = false;
  Ref hoveredPrimitive;
  bool hasHoveredPrimitive = false;
};

std::string_view kindName(Kind kind);
std::string_view faceName(Face face);

BlockView buildBlockView(const BlockRequest& request);

}  // namespace iggy3d::creative
