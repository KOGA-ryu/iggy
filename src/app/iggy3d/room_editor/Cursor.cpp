#include "app/iggy3d/room_editor/Cursor.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace iggy3d {
namespace {

bool finiteVec3(Vec3 value) {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

bool validCellSize(float cellSizeMeters) {
  return std::isfinite(cellSizeMeters) && cellSizeMeters > 0.0F;
}

ProductRoomEditorCursorResult cursorResult(ProductRoomEditorCursorState state,
                                           bool ok,
                                           std::string status) {
  ProductRoomEditorCursorResult result;
  result.ok = ok;
  result.status = std::move(status);
  result.reasonCode = result.status;
  result.state = state;
  return result;
}

Vec3 cursorCenter(ProductRoomEditorCursorState state) {
  return {static_cast<float>(state.gridX) * state.cellSizeMeters,
          -0.05F,
          static_cast<float>(state.gridZ) * state.cellSizeMeters};
}

std::string nextFloorId(const EditableRoomDocument& document) {
  return "edit_floor_" + std::to_string(nextEditableFloorIndex(document));
}

std::string nextWallId(const EditableRoomDocument& document) {
  return "edit_wall_" + std::to_string(nextEditableWallIndex(document));
}

bool fillWallEdge(ProductRoomEditorCursorState state,
                  Vec3& startMeters,
                  Vec3& endMeters) {
  const float half = state.cellSizeMeters * 0.5F;
  const float centerX = static_cast<float>(state.gridX) * state.cellSizeMeters;
  const float centerZ = static_cast<float>(state.gridZ) * state.cellSizeMeters;
  const float west = centerX - half;
  const float east = centerX + half;
  const float north = centerZ - half;
  const float south = centerZ + half;

  switch (state.wallDirection) {
    case ProductRoomEditorDirection::Up:
      startMeters = {west, 0.0F, north};
      endMeters = {east, 0.0F, north};
      return true;
    case ProductRoomEditorDirection::Right:
      startMeters = {east, 0.0F, north};
      endMeters = {east, 0.0F, south};
      return true;
    case ProductRoomEditorDirection::Down:
      startMeters = {east, 0.0F, south};
      endMeters = {west, 0.0F, south};
      return true;
    case ProductRoomEditorDirection::Left:
      startMeters = {west, 0.0F, south};
      endMeters = {west, 0.0F, north};
      return true;
  }
  return false;
}

struct WallDirectionCycleRow {
  ProductRoomEditorDirection from;
  ProductRoomEditorDirection to;
};

constexpr std::array kClockwiseWallDirectionCycle{
    WallDirectionCycleRow{ProductRoomEditorDirection::Up,
                          ProductRoomEditorDirection::Right},
    WallDirectionCycleRow{ProductRoomEditorDirection::Right,
                          ProductRoomEditorDirection::Down},
    WallDirectionCycleRow{ProductRoomEditorDirection::Down,
                          ProductRoomEditorDirection::Left},
    WallDirectionCycleRow{ProductRoomEditorDirection::Left,
                          ProductRoomEditorDirection::Up},
};

constexpr float kPi = 3.14159265358979323846F;

bool finiteVec2(float x, float y) {
  return std::isfinite(x) && std::isfinite(y);
}

bool finiteViewportConfig(const ProductViewportFrameConfig& config) {
  return std::isfinite(config.cameraYawDegrees) &&
         std::isfinite(config.cameraPitchDegrees) &&
         std::isfinite(config.centerX) && std::isfinite(config.centerY);
}

bool finiteAnchor(Vec3 anchor) {
  return std::isfinite(anchor.x) && std::isfinite(anchor.y) &&
         std::isfinite(anchor.z);
}

ProductRoomEditorMousePickResult mousePickResult(
    const ProductRoomEditorCursorState& cursor,
    std::string status) {
  ProductRoomEditorMousePickResult result;
  result.cursor = cursor;
  result.gridX = cursor.gridX;
  result.gridZ = cursor.gridZ;
  result.status = std::move(status);
  result.reasonCode = result.status;
  return result;
}

ProductRoomEditorCursorResult buildFloorCommand(ProductRoomEditorCursorState state,
                                                const EditableRoomDocument& document) {
  EditableRoomFloor floor;
  floor.id = nextFloorId(document);
  floor.storyIndex = state.storyIndex;
  floor.centerMeters = cursorCenter(state);
  floor.sizeMeters = {state.cellSizeMeters, 0.10F, state.cellSizeMeters};
  floor.semantics = defaultFloorSemantics();

  if (!finiteVec3(floor.centerMeters) || !finiteVec3(floor.sizeMeters)) {
    return cursorResult(state, false, "room_editor_invalid_geometry");
  }

  ProductRoomEditorCursorResult result =
      cursorResult(state, true, "room_editor_command_built");
  result.command = addFloorCommand(std::move(floor));
  return result;
}

ProductRoomEditorCursorResult buildWallCommand(ProductRoomEditorCursorState state,
                                               const EditableRoomDocument& document) {
  EditableRoomWall wall;
  wall.id = nextWallId(document);
  wall.storyIndex = state.storyIndex;
  wall.bottomY = 0.0F;
  wall.heightMeters = 2.5F;
  wall.thicknessMeters = 1.0F;
  wall.semantics = defaultWallSemantics();
  if (!fillWallEdge(state, wall.startMeters, wall.endMeters)) {
    return cursorResult(state, false, "room_editor_invalid_direction");
  }

  if (!finiteVec3(wall.startMeters) || !finiteVec3(wall.endMeters) ||
      !std::isfinite(wall.bottomY) || !std::isfinite(wall.heightMeters) ||
      !std::isfinite(wall.thicknessMeters)) {
    return cursorResult(state, false, "room_editor_invalid_geometry");
  }

  ProductRoomEditorCursorResult result =
      cursorResult(state, true, "room_editor_command_built");
  result.command = addWallCommand(std::move(wall));
  return result;
}

}  // namespace

std::string_view productRoomEditorToolName(ProductRoomEditorTool tool) {
  switch (tool) {
    case ProductRoomEditorTool::Floor:
      return "floor";
    case ProductRoomEditorTool::Wall:
      return "wall";
  }
  return "unknown";
}

std::string_view productRoomEditorDirectionName(ProductRoomEditorDirection direction) {
  switch (direction) {
    case ProductRoomEditorDirection::Up:
      return "up";
    case ProductRoomEditorDirection::Down:
      return "down";
    case ProductRoomEditorDirection::Left:
      return "left";
    case ProductRoomEditorDirection::Right:
      return "right";
  }
  return "unknown";
}

ProductRoomEditorCursorResult moveProductRoomEditorCursor(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction) {
  switch (direction) {
    case ProductRoomEditorDirection::Up:
      --state.gridZ;
      return cursorResult(state, true, "room_editor_cursor_moved");
    case ProductRoomEditorDirection::Down:
      ++state.gridZ;
      return cursorResult(state, true, "room_editor_cursor_moved");
    case ProductRoomEditorDirection::Left:
      --state.gridX;
      return cursorResult(state, true, "room_editor_cursor_moved");
    case ProductRoomEditorDirection::Right:
      ++state.gridX;
      return cursorResult(state, true, "room_editor_cursor_moved");
  }
  return cursorResult(state, false, "room_editor_invalid_direction");
}

ProductRoomEditorCursorResult cycleProductRoomEditorTool(
    ProductRoomEditorCursorState state) {
  switch (state.selectedTool) {
    case ProductRoomEditorTool::Floor:
      state.selectedTool = ProductRoomEditorTool::Wall;
      return cursorResult(state, true, "room_editor_tool_changed");
    case ProductRoomEditorTool::Wall:
      state.selectedTool = ProductRoomEditorTool::Floor;
      return cursorResult(state, true, "room_editor_tool_changed");
  }
  return cursorResult(state, false, "room_editor_invalid_tool");
}

ProductRoomEditorCursorResult setProductRoomEditorTool(
    ProductRoomEditorCursorState state,
    ProductRoomEditorTool tool) {
  switch (tool) {
    case ProductRoomEditorTool::Floor:
    case ProductRoomEditorTool::Wall:
      state.selectedTool = tool;
      return cursorResult(state, true, "room_editor_tool_changed");
  }
  return cursorResult(state, false, "room_editor_invalid_tool");
}

ProductRoomEditorCursorResult setProductRoomEditorWallDirection(
    ProductRoomEditorCursorState state,
    ProductRoomEditorDirection direction) {
  switch (direction) {
    case ProductRoomEditorDirection::Up:
    case ProductRoomEditorDirection::Down:
    case ProductRoomEditorDirection::Left:
    case ProductRoomEditorDirection::Right:
      state.wallDirection = direction;
      return cursorResult(state, true, "room_editor_wall_direction_changed");
  }
  return cursorResult(state, false, "room_editor_invalid_direction");
}

ProductRoomEditorCursorResult rotateProductRoomEditorWallDirectionClockwise(
    ProductRoomEditorCursorState state) {
  for (const WallDirectionCycleRow& row : kClockwiseWallDirectionCycle) {
    // branch-gate: BG-1042
    if (state.wallDirection == row.from) {
      return setProductRoomEditorWallDirection(state, row.to);
    }
  }
  return cursorResult(state, false, "room_editor_invalid_direction");
}

ProductRoomEditorCursorResult buildProductRoomEditorPlaceCommand(
    ProductRoomEditorCursorState state,
    const EditableRoomDocument* document) {
  if (document == nullptr) {
    return cursorResult(state, false, "room_editor_document_missing");
  }
  if (!validCellSize(state.cellSizeMeters)) {
    return cursorResult(state, false, "room_editor_invalid_cell_size");
  }

  switch (state.selectedTool) {
    case ProductRoomEditorTool::Floor:
      return buildFloorCommand(state, *document);
    case ProductRoomEditorTool::Wall:
      return buildWallCommand(state, *document);
  }
  return cursorResult(state, false, "room_editor_invalid_tool");
}

ProductRoomEditorMousePickResult pickProductRoomEditorCursorFromScreen(
    const ProductRoomEditorMousePickRequest& request) {
  ProductRoomEditorMousePickResult result =
      mousePickResult(request.cursor, "room_editor_mouse_pick_not_ready");
  // branch-gate: BG-1043
  if (!request.roomEditingReady) {
    return result;
  }
  // branch-gate: BG-1043
  if (!validCellSize(request.cursor.cellSizeMeters)) {
    return mousePickResult(request.cursor,
                           "room_editor_mouse_pick_invalid_cell_size");
  }
  // branch-gate: BG-1043
  if (!std::isfinite(request.viewportConfig.pixelsPerMeter) ||
      request.viewportConfig.pixelsPerMeter <= 0.0F) {
    return mousePickResult(request.cursor,
                           "room_editor_mouse_pick_invalid_pixels_per_meter");
  }
  // branch-gate: BG-1043
  if (!finiteVec2(request.screenX, request.screenY) ||
      !finiteViewportConfig(request.viewportConfig) ||
      !finiteAnchor(request.anchorWorld)) {
    return mousePickResult(request.cursor, "room_editor_mouse_pick_invalid_input");
  }

  const float yawRadians =
      request.viewportConfig.cameraYawDegrees * kPi / 180.0F;
  const float cosYaw = std::cos(yawRadians);
  const float sinYaw = std::sin(yawRadians);
  const float pitchOffsetPixels =
      request.viewportConfig.cameraPitchDegrees * 1.5F;
  const float viewX =
      (request.screenX - request.viewportConfig.centerX) /
      request.viewportConfig.pixelsPerMeter;
  const float viewZ =
      (request.viewportConfig.centerY + pitchOffsetPixels - request.screenY) /
      request.viewportConfig.pixelsPerMeter;
  const float worldX = request.anchorWorld.x + viewX * cosYaw + viewZ * sinYaw;
  const float worldZ = request.anchorWorld.z - viewX * sinYaw + viewZ * cosYaw;

  result = mousePickResult(request.cursor, "room_editor_mouse_pick_mapped");
  result.ok = true;
  result.worldX = worldX;
  result.worldZ = worldZ;
  result.gridX = static_cast<std::int32_t>(
      std::lround(worldX / request.cursor.cellSizeMeters));
  result.gridZ = static_cast<std::int32_t>(
      std::lround(worldZ / request.cursor.cellSizeMeters));
  result.cursor.gridX = result.gridX;
  result.cursor.gridZ = result.gridZ;
  return result;
}

}  // namespace iggy3d
