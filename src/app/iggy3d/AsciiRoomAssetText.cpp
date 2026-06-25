#include "app/iggy3d/AsciiRoomAssetText.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d {
namespace {

bool hasUnsafeStringCharacter(std::string_view value) {
  for (const char c : value) {
    if (c == '"' || c == '\\' || c == '\n' || c == '\r') {
      return true;
    }
  }
  return false;
}

bool roomHasUnsafeStrings(const RoomAsset& room) {
  if (hasUnsafeStringCharacter(room.id) || hasUnsafeStringCharacter(room.units) ||
      hasUnsafeStringCharacter(room.source) || hasUnsafeStringCharacter(room.sourceFile) ||
      hasUnsafeStringCharacter(room.sourceSubset)) {
    return true;
  }
  for (const RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (hasUnsafeStringCharacter(mesh.id) || hasUnsafeStringCharacter(mesh.meshId) ||
        hasUnsafeStringCharacter(mesh.materialId) || hasUnsafeStringCharacter(mesh.role)) {
      return true;
    }
  }
  for (const RoomOpeningAsset& opening : room.openings) {
    if (hasUnsafeStringCharacter(opening.id) || hasUnsafeStringCharacter(opening.edge) ||
        hasUnsafeStringCharacter(opening.kind)) {
      return true;
    }
  }
  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    if (hasUnsafeStringCharacter(surface.id) ||
        hasUnsafeStringCharacter(surface.sourceStaticMeshId) ||
        hasUnsafeStringCharacter(surface.openingId)) {
      return true;
    }
    for (const std::string& tag : surface.traversalTags) {
      if (hasUnsafeStringCharacter(tag)) {
        return true;
      }
    }
    for (const std::string& mask : surface.collisionMask) {
      if (hasUnsafeStringCharacter(mask)) {
        return true;
      }
    }
  }
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (hasUnsafeStringCharacter(anchor.id) || hasUnsafeStringCharacter(anchor.kind) ||
        hasUnsafeStringCharacter(anchor.runtimeStableName)) {
      return true;
    }
  }
  return false;
}

std::string quote(std::string_view value) {
  return "\"" + std::string(value) + "\"";
}

float metersToFeet(float meters, float feetToMeters) {
  return meters / feetToMeters;
}

void writeFloat(std::ostream& out, float value) {
  out << value;
}

void writeVec3Feet(std::ostream& out, const Vec3& value, float feetToMeters) {
  out << '[';
  writeFloat(out, metersToFeet(value.x, feetToMeters));
  out << ", ";
  writeFloat(out, metersToFeet(value.y, feetToMeters));
  out << ", ";
  writeFloat(out, metersToFeet(value.z, feetToMeters));
  out << ']';
}

void writeVec3Meters(std::ostream& out, const Vec3& value) {
  out << '[';
  writeFloat(out, value.x);
  out << ", ";
  writeFloat(out, value.y);
  out << ", ";
  writeFloat(out, value.z);
  out << ']';
}

std::string shapeName(RoomSpatialSurfaceShape shape) {
  switch (shape) {
    case RoomSpatialSurfaceShape::Box:
      return "box";
    case RoomSpatialSurfaceShape::Plane:
      return "plane";
    case RoomSpatialSurfaceShape::Opening:
      return "opening";
  }
  return "plane";
}

std::string roleName(RoomSpatialSurfaceRole role) {
  switch (role) {
    case RoomSpatialSurfaceRole::Walkable:
      return "walkable";
    case RoomSpatialSurfaceRole::Blocker:
      return "blocker";
    case RoomSpatialSurfaceRole::ProjectileBlocker:
      return "projectile_blocker";
    case RoomSpatialSurfaceRole::Opening:
      return "opening";
  }
  return "walkable";
}

bool parserCompatibleTraversalTag(std::string_view tag) {
  return tag == "walkable" || tag == "blocker" || tag == "projectile_blocker" ||
         tag == "opening" || tag == "clamber" || tag == "vault" ||
         tag == "wire_walk" || tag == "no_player" || tag == "debug_only";
}

bool contains(const std::vector<std::string>& values, std::string_view expected) {
  for (const std::string& value : values) {
    if (value == expected) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> traversalTagsForExport(const RoomSpatialSurface& surface) {
  std::vector<std::string> tags;
  const auto append = [&tags](std::string value) {
    if (!contains(tags, value)) {
      tags.push_back(std::move(value));
    }
  };
  if (surface.role == RoomSpatialSurfaceRole::Walkable) {
    append("walkable");
  } else if (surface.role == RoomSpatialSurfaceRole::Blocker) {
    append("blocker");
  } else if (surface.role == RoomSpatialSurfaceRole::ProjectileBlocker) {
    append("projectile_blocker");
  } else if (surface.role == RoomSpatialSurfaceRole::Opening) {
    append("opening");
  }
  for (const std::string& tag : surface.traversalTags) {
    if (parserCompatibleTraversalTag(tag)) {
      append(tag);
    }
  }
  return tags;
}

void writeStringArray(std::ostream& out, const std::vector<std::string>& values) {
  out << '[';
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0U) {
      out << ", ";
    }
    out << quote(values[index]);
  }
  out << ']';
}

std::vector<Vec3> pointsForExport(const RoomSpatialSurface& surface) {
  if (surface.pointsMeters.size() <= 4U) {
    return surface.pointsMeters;
  }
  return {surface.pointsMeters[0], surface.pointsMeters[1],
          surface.pointsMeters[2], surface.pointsMeters[3]};
}

void writePointsFeet(std::ostream& out,
                     const RoomSpatialSurface& surface,
                     float feetToMeters) {
  const std::vector<Vec3> points = pointsForExport(surface);
  out << '[';
  for (std::size_t index = 0; index < points.size(); ++index) {
    if (index != 0U) {
      out << ", ";
    }
    writeVec3Feet(out, points[index], feetToMeters);
  }
  out << ']';
}

void writeStaticMesh(std::ostream& out,
                     const RoomStaticMeshAsset& mesh,
                     const AsciiRoomAssetTextConfig& config) {
  out << "\n[[static_meshes]]\n";
  out << "id = " << quote(mesh.id) << "\n";
  out << "mesh = " << quote(mesh.meshId) << "\n";
  out << "role = " << quote(mesh.role) << "\n";
  out << "position_ft = ";
  writeVec3Feet(out, mesh.positionMeters, config.feetToMeters);
  out << "\nsize_ft = ";
  writeVec3Feet(out, mesh.sizeMeters, config.feetToMeters);
  out << "\nmaterial = " << quote(mesh.materialId) << "\n";
}

void writeOpening(std::ostream& out,
                  const RoomOpeningAsset& opening,
                  const AsciiRoomAssetTextConfig& config) {
  out << "\n[[openings]]\n";
  out << "id = " << quote(opening.id) << "\n";
  out << "edge = " << quote(opening.edge) << "\n";
  out << "kind = " << quote(opening.kind) << "\n";
  out << "offset_ft = ";
  writeFloat(out, metersToFeet(opening.offsetMeters, config.feetToMeters));
  out << "\nwidth_ft = ";
  writeFloat(out, metersToFeet(opening.widthMeters, config.feetToMeters));
  out << "\n";
}

void writeSpatialSurface(std::ostream& out,
                         const RoomSpatialSurface& surface,
                         const AsciiRoomAssetTextConfig& config) {
  out << "\n[[spatial_surfaces]]\n";
  out << "id = " << quote(surface.id) << "\n";
  out << "source_static_mesh = " << quote(surface.sourceStaticMeshId) << "\n";
  out << "shape = " << quote(shapeName(surface.shape)) << "\n";
  out << "role = " << quote(roleName(surface.role)) << "\n";
  out << "points_ft = ";
  writePointsFeet(out, surface, config.feetToMeters);
  out << "\nnormal = ";
  writeVec3Meters(out, surface.normal);
  out << "\ntraversal_tags = ";
  writeStringArray(out, traversalTagsForExport(surface));
  out << "\ncollision_mask = ";
  writeStringArray(out, surface.collisionMask);
  out << "\nblocks_actor = " << (surface.blocksActor ? "true" : "false") << "\n";
  out << "blocks_projectile = " << (surface.blocksProjectile ? "true" : "false") << "\n";
  out << "opening_id = " << quote(surface.openingId) << "\n";
}

void writeAnchor(std::ostream& out,
                 const RoomAnchorAsset& anchor,
                 const AsciiRoomAssetTextConfig& config) {
  out << "\n[[anchors]]\n";
  out << "id = " << quote(anchor.id) << "\n";
  out << "kind = " << quote(anchor.kind) << "\n";
  out << "position_ft = ";
  writeVec3Feet(out, anchor.positionMeters, config.feetToMeters);
  out << "\nruntime_stable_name = " << quote(anchor.runtimeStableName) << "\n";
}

}  // namespace

AsciiRoomAssetTextResult writeAsciiRoomAssetText(
    const RoomAsset& room,
    const AsciiRoomAssetTextConfig& config) {
  AsciiRoomAssetTextResult result;
  if (config.feetToMeters <= 0.0F) {
    result.status = "ascii_room_asset_text_invalid_conversion";
    result.reasonCode = result.status;
    return result;
  }
  if (room.id.empty() || room.sourceFile.empty() || room.sourceSubset.empty()) {
    result.status = "ascii_room_asset_text_missing_required";
    result.reasonCode = result.status;
    return result;
  }
  if (room.staticMeshes.empty()) {
    result.status = "ascii_room_asset_text_missing_static_meshes";
    result.reasonCode = result.status;
    return result;
  }
  if (room.anchors.empty()) {
    result.status = "ascii_room_asset_text_missing_anchors";
    result.reasonCode = result.status;
    return result;
  }
  if (roomHasUnsafeStrings(room)) {
    result.status = "ascii_room_asset_text_unsafe_string";
    result.reasonCode = result.status;
    return result;
  }

  std::ostringstream out;
  out << std::fixed << std::setprecision(config.decimalPlaces);
  out << "[room]\n";
  out << "id = " << quote(room.id) << "\n";
  out << "version = " << room.version << "\n";
  out << "units = " << quote(room.units) << "\n";
  out << "source = " << quote(room.source) << "\n";
  out << "source_file = " << quote(room.sourceFile) << "\n";
  out << "source_subset = " << quote(room.sourceSubset) << "\n";

  if (config.includeConversionTable) {
    out << "\n[conversion]\n";
    out << "feet_to_meters = ";
    writeFloat(out, config.feetToMeters);
    out << "\n";
  }

  for (const RoomStaticMeshAsset& mesh : room.staticMeshes) {
    writeStaticMesh(out, mesh, config);
  }
  for (const RoomOpeningAsset& opening : room.openings) {
    writeOpening(out, opening, config);
  }
  for (const RoomSpatialSurface& surface : room.spatialSurfaces) {
    writeSpatialSurface(out, surface, config);
  }
  for (const RoomAnchorAsset& anchor : room.anchors) {
    writeAnchor(out, anchor, config);
  }

  result.ok = true;
  result.status = "ascii_room_asset_text_written";
  result.reasonCode = result.status;
  result.text = out.str();
  if (result.text.empty() || result.text.back() != '\n') {
    result.text.push_back('\n');
  }
  result.staticMeshCount = room.staticMeshes.size();
  result.anchorCount = room.anchors.size();
  result.spatialSurfaceCount = room.spatialSurfaces.size();
  return result;
}

}  // namespace iggy3d
