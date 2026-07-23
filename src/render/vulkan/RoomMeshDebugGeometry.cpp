#include "render/vulkan/BufferImageResources.hpp"
#include "render/vulkan/RoomMeshCpuGeometryInternal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::vulkan {

using room_mesh_detail::appendBoxIfFits;
using room_mesh_detail::finiteVec3;
using room_mesh_detail::near;

namespace {

void hashByte(std::uint64_t& hash, std::uint8_t value) {
  hash ^= value;
  hash *= 1099511628211ULL;
}

void hashString(std::uint64_t& hash, const std::string& value) {
  for (const char character : value) {
    hashByte(hash, static_cast<std::uint8_t>(character));
  }
  hashByte(hash, 0U);
}

void hashFloat(std::uint64_t& hash, float value) {
  std::uint32_t bits = 0U;
  static_assert(sizeof(bits) == sizeof(value));
  std::memcpy(&bits, &value, sizeof(bits));
  for (std::uint32_t shift = 0U; shift < 32U; shift += 8U) {
    hashByte(hash, static_cast<std::uint8_t>((bits >> shift) & 0xFFU));
  }
}

void hashUint16(std::uint64_t& hash, std::uint16_t value) {
  hashByte(hash, static_cast<std::uint8_t>(value & 0xFFU));
  hashByte(hash, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void hashBool(std::uint64_t& hash, bool value) {
  hashByte(hash, value ? 1U : 0U);
}

void hashVec3(std::uint64_t& hash, Vec3 value) {
  hashFloat(hash, value.x);
  hashFloat(hash, value.y);
  hashFloat(hash, value.z);
}

std::uint64_t roomGeometrySignature(const SceneRoomProjection& room) {
  std::uint64_t hash = 1469598103934665603ULL;
  hashString(hash, room.assetId);
  for (const SceneRoomMeshItem& mesh : room.meshes) {
    hashString(hash, mesh.id);
    hashString(hash, mesh.meshId);
    hashString(hash, mesh.role);
    hashString(hash, mesh.materialId);
    hashString(hash, mesh.materialVariant);
    hashString(hash, mesh.semanticRole);
    hashFloat(hash, mesh.position.x);
    hashFloat(hash, mesh.position.y);
    hashFloat(hash, mesh.position.z);
    hashFloat(hash, mesh.size.x);
    hashFloat(hash, mesh.size.y);
    hashFloat(hash, mesh.size.z);
    hashVec3(hash, mesh.rotationEulerRadians);
    hashUint16(hash, mesh.proceduralSegmentCount);
    hashBool(hash, mesh.hasWallSegment);
    if (mesh.hasWallSegment) {
      hashVec3(hash, mesh.wallStartMeters);
      hashVec3(hash, mesh.wallEndMeters);
      hashFloat(hash, mesh.wallBottomY);
      hashFloat(hash, mesh.wallHeightMeters);
      hashFloat(hash, mesh.wallThicknessMeters);
    }
  }
  for (const SceneRoomSurfacePatchItem& patch : room.surfacePatches) {
    hashString(hash, patch.role);
    hashVec3(hash, patch.center);
    for (const Vec3 corner : patch.corners) {
      hashVec3(hash, corner);
    }
    hashBool(hash, patch.hasTint);
    if (patch.hasTint) {
      hashVec3(hash, patch.tint);
    }
  }
  return hash;
}


void setCreativeWireframeDebugGeometryStatus(
    CreativeWireframeDebugCpuGeometry& geometry,
    std::string_view status) {
  geometry.status = std::string(status);
  geometry.reasonCode = geometry.status;
}

bool creativeDebugLineBox(const RenderCreativeWireframeDebugLine& line,
                          Vec3& center,
                          Vec3& size,
                          Vec3& color,
                          Vec3& rotationEulerRadians) {
  if (!finiteVec3(line.start) || !finiteVec3(line.end) ||
      !std::isfinite(line.thickness) || line.thickness <= 0.0F ||
      !std::isfinite(line.color.r) || !std::isfinite(line.color.g) ||
      !std::isfinite(line.color.b)) {
    return false;
  }

  const float dx = line.end.x - line.start.x;
  const float dy = line.end.y - line.start.y;
  const float dz = line.end.z - line.start.z;
  const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (!std::isfinite(length) || near(length, 0.0F)) {
    return false;
  }

  center = {(line.start.x + line.end.x) * 0.5F,
            (line.start.y + line.end.y) * 0.5F,
            (line.start.z + line.end.z) * 0.5F};
  const float thickness = std::max(line.thickness, 0.001F);
  size = {length, thickness, thickness};
  const float horizontalLength = std::sqrt(dx * dx + dy * dy);
  rotationEulerRadians =
      {0.0F, std::atan2(-dz, horizontalLength), std::atan2(dy, dx)};
  color = {line.color.r, line.color.g, line.color.b};
  return true;
}

std::uint64_t creativeWireframeDebugGeometrySignature(
    const RenderCreativeWireframeDebugFrame* frame) {
  std::uint64_t hash = 1469598103934665603ULL;
  if (frame == nullptr || !frame->available) {
    hashString(hash, "not_requested");
    return hash;
  }
  hashString(hash, frame->visible ? "visible" : "hidden");
  hashByte(hash, static_cast<std::uint8_t>(frame->lineCount & 0xFFU));
  hashByte(hash, static_cast<std::uint8_t>((frame->lineCount >> 8U) & 0xFFU));
  if (frame->lines == nullptr || frame->lineCount == 0U) {
    return hash;
  }
  for (std::size_t index = 0; index < frame->lineCount; ++index) {
    const RenderCreativeWireframeDebugLine& line = frame->lines[index];
    hashVec3(hash, line.start);
    hashVec3(hash, line.end);
    hashFloat(hash, line.color.r);
    hashFloat(hash, line.color.g);
    hashFloat(hash, line.color.b);
    hashFloat(hash, line.color.a);
    hashFloat(hash, line.thickness);
  }
  return hash;
}

void appendCreativeWireframeDebugReceiptFields(
    RenderReceipt& receipt,
    const FirstRoomGeometryResources& geometry) {
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_input_line_count",
                     static_cast<std::uint64_t>(
                         geometry.creativeWireframeDebugLineInputCount));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_draw_count",
                     static_cast<std::uint64_t>(
                         geometry.creativeWireframeDebugGeometryDrawCount));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_box_count",
                     static_cast<std::uint64_t>(
                         geometry.creativeWireframeDebugGeometryDrawCount));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_skipped_count",
                     static_cast<std::uint64_t>(
                         geometry.creativeWireframeDebugGeometrySkippedCount));
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_status",
                     geometry.creativeWireframeDebugGeometryStatus);
  appendReceiptField(receipt,
                     "creative_wireframe_debug_geometry_reason_code",
                     geometry.creativeWireframeDebugGeometryReasonCode);
}

}  // namespace

CreativeWireframeDebugCpuGeometry buildCreativeWireframeDebugCpuGeometry(
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  CreativeWireframeDebugCpuGeometry result;
  result.geometrySignature =
      creativeWireframeDebugGeometrySignature(creativeWireframeDebug);

  if (creativeWireframeDebug == nullptr || !creativeWireframeDebug->available) {
    setCreativeWireframeDebugGeometryStatus(
        result,
        "vulkan_creative_wireframe_debug_geometry_not_requested");
    return result;
  }

  result.inputLineCount = creativeWireframeDebug->lineCount;
  if (creativeWireframeDebug->lineCount == 0U) {
    setCreativeWireframeDebugGeometryStatus(
        result,
        "vulkan_creative_wireframe_debug_geometry_no_lines");
    return result;
  }

  if (creativeWireframeDebug->lines == nullptr) {
    result.skippedCount = creativeWireframeDebug->lineCount;
    setCreativeWireframeDebugGeometryStatus(
        result,
        "vulkan_creative_wireframe_debug_geometry_source_missing");
    return result;
  }

  result.vertices.reserve(creativeWireframeDebug->lineCount * 8U);
  result.indices.reserve(creativeWireframeDebug->lineCount * 72U);
  for (std::size_t index = 0; index < creativeWireframeDebug->lineCount;
       ++index) {
    Vec3 center;
    Vec3 size;
    Vec3 color;
    Vec3 rotationEulerRadians;
    if (!creativeDebugLineBox(creativeWireframeDebug->lines[index],
                              center,
                              size,
                              color,
                              rotationEulerRadians) ||
        !appendBoxIfFits(result.vertices,
                         result.indices,
                         result.indexedDraws,
                         center,
                         size,
                         color,
                         rotationEulerRadians)) {
      ++result.skippedCount;
      continue;
    }
    ++result.emittedBoxCount;
  }

  result.ready = result.emittedBoxCount > 0U && !result.vertices.empty() &&
                 !result.indices.empty() && !result.indexedDraws.empty();
  if (result.ready) {
    setCreativeWireframeDebugGeometryStatus(
        result,
        "vulkan_creative_wireframe_debug_geometry_built");
    return result;
  }

  setCreativeWireframeDebugGeometryStatus(
      result,
      "vulkan_creative_wireframe_debug_geometry_no_geometry");
  return result;
}

void room_mesh_detail::appendCreativeWireframeDebugGeometry(
    RoomMeshCpuGeometry& roomGeometry,
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  const CreativeWireframeDebugCpuGeometry debugGeometry =
      buildCreativeWireframeDebugCpuGeometry(creativeWireframeDebug);
  roomGeometry.creativeWireframeDebugLineInputCount =
      debugGeometry.inputLineCount;
  roomGeometry.creativeWireframeDebugGeometryDrawCount =
      debugGeometry.emittedBoxCount;
  roomGeometry.creativeWireframeDebugGeometrySkippedCount =
      debugGeometry.skippedCount;
  roomGeometry.creativeWireframeDebugGeometryStatus = debugGeometry.status;
  roomGeometry.creativeWireframeDebugGeometryReasonCode =
      debugGeometry.reasonCode;
  roomGeometry.sourceCreativeWireframeDebugSignature =
      debugGeometry.geometrySignature;

  if (!debugGeometry.ready) {
    return;
  }

  if (roomGeometry.vertices.size() + debugGeometry.vertices.size() >
      static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
    roomGeometry.creativeWireframeDebugGeometryDrawCount = 0;
    roomGeometry.creativeWireframeDebugGeometrySkippedCount =
        debugGeometry.inputLineCount;
    roomGeometry.creativeWireframeDebugGeometryStatus =
        "vulkan_creative_wireframe_debug_geometry_no_geometry";
    roomGeometry.creativeWireframeDebugGeometryReasonCode =
        roomGeometry.creativeWireframeDebugGeometryStatus;
    return;
  }

  const std::uint16_t vertexBase =
      static_cast<std::uint16_t>(roomGeometry.vertices.size());
  const std::uint32_t indexBase =
      static_cast<std::uint32_t>(roomGeometry.indices.size());
  roomGeometry.vertices.insert(roomGeometry.vertices.end(),
                               debugGeometry.vertices.begin(),
                               debugGeometry.vertices.end());
  roomGeometry.indices.reserve(roomGeometry.indices.size() +
                               debugGeometry.indices.size());
  for (const std::uint16_t index : debugGeometry.indices) {
    roomGeometry.indices.push_back(static_cast<std::uint16_t>(vertexBase + index));
  }
  roomGeometry.indexedDraws.reserve(roomGeometry.indexedDraws.size() +
                                    debugGeometry.indexedDraws.size());
  for (IndexedDrawRange draw : debugGeometry.indexedDraws) {
    draw.firstIndex += indexBase;
    roomGeometry.indexedDraws.push_back(draw);
  }
}

std::uint64_t room_mesh_detail::roomGeometrySignature(
    const SceneRoomProjection& room) {
  return ::iggy3d::vulkan::roomGeometrySignature(room);
}

std::uint64_t room_mesh_detail::creativeWireframeDebugGeometrySignature(
    const RenderCreativeWireframeDebugFrame* creativeWireframeDebug) {
  return ::iggy3d::vulkan::creativeWireframeDebugGeometrySignature(
      creativeWireframeDebug);
}

void room_mesh_detail::appendCreativeWireframeDebugReceiptFields(
    RenderReceipt& receipt, const FirstRoomGeometryResources& geometry) {
  ::iggy3d::vulkan::appendCreativeWireframeDebugReceiptFields(receipt,
                                                              geometry);
}

}  // namespace iggy3d::vulkan
