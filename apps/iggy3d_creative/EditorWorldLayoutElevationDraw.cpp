#include "EditorWorldLayoutElevationDrawInternal.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

#include "EditorDraftingStyle.hpp"
#include "EditorWorldLayoutOpenings.hpp"

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

ImU32 color(ImVec4 value) { return ImGui::ColorConvertFloat4ToU32(value); }

ImU32 draftingColor(CreativeEditorDraftingColor value) {
  return IM_COL32(value.r, value.g, value.b, value.a);
}

[[nodiscard]] bool selected(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutSelectionKind kind,
    std::size_t index) noexcept {
  return state.selection.kind == kind && state.selection.index == index;
}

ImVec2 toElevationScreen(
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationPoint point) {
  const CreativeEditorWorldLayoutElevationScreenPoint screen =
      planCreativeEditorWorldLayoutElevationScreenPoint(transform, point);
  return {screen.x, screen.y};
}

CreativeEditorWorldLayoutElevationPoint toElevationWorld(
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    ImVec2 screen) {
  return planCreativeEditorWorldLayoutElevationWorldPoint(
      transform, {screen.x, screen.y});
}

CreativeEditorWorldLayoutElevationPoint measurementElevationPoint(
    CreativeEditorWorldLayoutElevationAxis axis,
    cr::CreativeMeasurementPoint point) noexcept {
  return {axis == CreativeEditorWorldLayoutElevationAxis::X ? point.x : point.z,
          point.y};
}

void drawMeasurementGeometry(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    CreativeEditorWorldLayoutElevationAxis axis,
    const cr::CreativeMeasurementGeometry& geometry,
    std::string_view label, bool transient) {
  if (!geometry.visible) {
    return;
  }
  const CreativeEditorDraftingStyle& style = creativeEditorDraftingStyle(
      CreativeEditorDraftingRole::MeasurementOverlay);
  CreativeEditorDraftingColor tintValue = style.tint;
  if (!transient) {
    tintValue.a = static_cast<std::uint8_t>(
        static_cast<float>(tintValue.a) * 0.72F);
  }
  const ImU32 tint = draftingColor(tintValue);
  const float thickness = creativeEditorDraftingStrokeThicknessPixels(
      style, transform.pixelsPerCell);
  for (std::size_t index = 0U; index < geometry.segmentCount; ++index) {
    const cr::CreativeMeasurementSegment& segment = geometry.segments[index];
    drawList.AddLine(
        toElevationScreen(transform,
                          measurementElevationPoint(axis, segment.start)),
        toElevationScreen(transform,
                          measurementElevationPoint(axis, segment.end)),
        tint, thickness);
  }
  for (std::size_t index = 0U; index < geometry.pointCount; ++index) {
    drawList.AddCircleFilled(
        toElevationScreen(
            transform, measurementElevationPoint(axis, geometry.points[index])),
        index + 1U == geometry.pointCount ? 4.5F : 3.5F, tint);
  }
  if (!label.empty() && geometry.pointCount > 0U) {
    const ImVec2 anchor = toElevationScreen(
        transform,
        measurementElevationPoint(axis,
                                  geometry.points[geometry.pointCount - 1U]));
    drawList.AddText({anchor.x + 8.0F, anchor.y + 8.0F}, tint, label.data(),
                     label.data() + label.size());
  }
}

void drawElevationSectionAnnotations(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    const CreativeEditorWorldLayoutState& state) {
  for (const CreativeEditorWorldLayoutSectionLevel& level :
       projection.sectionLevels) {
    const float datumY =
        toElevationScreen(transform, {0.0, level.floorDatumCells}).y;
    if (datumY < minimum.y || datumY > maximum.y) {
      continue;
    }
    const bool active = state.activeLevelIndex == level.levelIndex;
    const ImU32 datumColor =
        active ? color({0.34F, 0.92F, 0.46F, 0.95F})
               : color({0.55F, 0.66F, 0.74F, 0.72F});
    drawList.AddLine({minimum.x + 4.0F, datumY},
                     {maximum.x - 4.0F, datumY}, datumColor,
                     active ? 1.8F : 1.0F);

    char label[256];
    const std::string_view extent =
        creativeEditorWorldLayoutSectionPartitionExtentLabel(
            level.partitionExtent);
    if (!level.occupied) {
      std::snprintf(label, sizeof(label), "%s  %+.2f m  |  %.*s",
                    level.name.c_str(), level.floorDatumMeters,
                    static_cast<int>(extent.size()), extent.data());
    } else if (level.floorToFloorMeters > 0.0) {
      std::snprintf(label, sizeof(label),
                    "%s  %+.2f m  |  clear %.2f m  |  F2F %.2f m  |  %.*s",
                    level.name.c_str(), level.floorDatumMeters,
                    level.clearHeightMeters, level.floorToFloorMeters,
                    static_cast<int>(extent.size()), extent.data());
    } else {
      std::snprintf(label, sizeof(label),
                    "%s  %+.2f m  |  clear %.2f m  |  %.*s",
                    level.name.c_str(), level.floorDatumMeters,
                    level.clearHeightMeters,
                    static_cast<int>(extent.size()), extent.data());
    }
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const ImVec2 textMinimum{minimum.x + 8.0F, datumY - textSize.y - 3.0F};
    const ImVec2 textMaximum{textMinimum.x + textSize.x + 8.0F,
                             textMinimum.y + textSize.y + 4.0F};
    drawList.AddRectFilled(textMinimum, textMaximum,
                           color({0.07F, 0.08F, 0.09F, 0.88F}), 2.0F);
    drawList.AddText({textMinimum.x + 4.0F, textMinimum.y + 2.0F},
                     datumColor, label);

    if (!active || !level.occupied) {
      continue;
    }
    const float partitionY =
        toElevationScreen(transform, {0.0, level.partitionTopCells}).y;
    if (partitionY >= minimum.y && partitionY <= maximum.y) {
      drawList.AddLine({minimum.x + 8.0F, partitionY},
                       {minimum.x + 116.0F, partitionY},
                       color({0.96F, 0.72F, 0.28F, 0.86F}), 1.2F);
      drawList.AddText({minimum.x + 122.0F, partitionY - 7.0F},
                       color({0.96F, 0.72F, 0.28F, 0.92F}),
                       "partition top");
    }
  }

  if (state.elevationManipulation.active &&
      !state.elevationManipulation.preview.accepted) {
    drawList.AddText(
        {minimum.x + 10.0F, maximum.y - 24.0F},
        color({0.96F, 0.28F, 0.24F, 1.0F}),
        "Invalid level position: release is blocked");
  }
}

void drawElevationGrid(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform) {
  const CreativeEditorWorldLayoutElevationPoint lowerLeft =
      toElevationWorld(transform, {minimum.x, maximum.y});
  const CreativeEditorWorldLayoutElevationPoint upperRight =
      toElevationWorld(transform, {maximum.x, minimum.y});
  const int firstHorizontal =
      static_cast<int>(std::floor(lowerLeft.horizontal));
  const int lastHorizontal =
      static_cast<int>(std::ceil(upperRight.horizontal));
  const int firstVertical =
      static_cast<int>(std::floor(lowerLeft.vertical));
  const int lastVertical = static_cast<int>(std::ceil(upperRight.vertical));
  for (int horizontal = firstHorizontal; horizontal <= lastHorizontal;
       ++horizontal) {
    const float screen =
        toElevationScreen(transform,
                          {static_cast<double>(horizontal), 0.0})
            .x;
    const bool major = horizontal % 5 == 0;
    drawList.AddLine({screen, minimum.y}, {screen, maximum.y},
                     major ? color({0.30F, 0.34F, 0.38F, 1.0F})
                           : color({0.20F, 0.23F, 0.26F, 1.0F}),
                     major ? 1.4F : 1.0F);
  }
  for (int vertical = firstVertical; vertical <= lastVertical; ++vertical) {
    const float screen =
        toElevationScreen(transform, {0.0, static_cast<double>(vertical)})
            .y;
    const bool major = vertical % 5 == 0;
    drawList.AddLine({minimum.x, screen}, {maximum.x, screen},
                     major ? color({0.30F, 0.34F, 0.38F, 1.0F})
                           : color({0.20F, 0.23F, 0.26F, 1.0F}),
                     major ? 1.4F : 1.0F);
  }
  const float zero = toElevationScreen(transform, {0.0, 0.0}).y;
  drawList.AddLine({minimum.x, zero}, {maximum.x, zero},
                   color({0.86F, 0.42F, 0.36F, 1.0F}), 1.8F);
}

ImU32 elevationItemColor(
    CreativeEditorWorldLayoutElevationItemKind kind) {
  switch (kind) {
    case CreativeEditorWorldLayoutElevationItemKind::FloorSlab:
      return color({0.38F, 0.45F, 0.52F, 0.90F});
    case CreativeEditorWorldLayoutElevationItemKind::WallEnvelope:
      return color({0.46F, 0.49F, 0.52F, 0.54F});
    case CreativeEditorWorldLayoutElevationItemKind::CeilingSlab:
      return color({0.53F, 0.58F, 0.63F, 0.88F});
    case CreativeEditorWorldLayoutElevationItemKind::RoofBase:
      return color({0.44F, 0.28F, 0.22F, 0.92F});
    case CreativeEditorWorldLayoutElevationItemKind::RoofSkylight:
      return color({0.20F, 0.65F, 0.82F, 0.92F});
    case CreativeEditorWorldLayoutElevationItemKind::RoofClearance:
      return color({0.78F, 0.54F, 0.22F, 0.82F});
    case CreativeEditorWorldLayoutElevationItemKind::Door:
      return color({0.72F, 0.45F, 0.20F, 0.96F});
    case CreativeEditorWorldLayoutElevationItemKind::Window:
      return color({0.20F, 0.65F, 0.82F, 0.88F});
    case CreativeEditorWorldLayoutElevationItemKind::Stair:
      return color({0.84F, 0.66F, 0.20F, 0.86F});
    case CreativeEditorWorldLayoutElevationItemKind::Ramp:
      return color({0.35F, 0.72F, 0.40F, 0.86F});
    case CreativeEditorWorldLayoutElevationItemKind::Volume:
      return color({0.62F, 0.42F, 0.74F, 0.82F});
    case CreativeEditorWorldLayoutElevationItemKind::Count:
      break;
  }
  return color({0.75F, 0.20F, 0.20F, 1.0F});
}

bool elevationItemSelected(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationItem& item) noexcept {
  switch (item.sourceKind) {
    case CreativeEditorWorldLayoutElevationSourceKind::Room:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Room,
                      item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Box:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Box,
                      item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Wall:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Wall,
                      item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::Opening:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Opening,
                      item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::RoofAperture:
      return selected(
          state, CreativeEditorWorldLayoutSelectionKind::RoofAperture,
          item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::VerticalConnector:
      return selected(
          state, CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
          item.sourceIndex);
    case CreativeEditorWorldLayoutElevationSourceKind::None:
    case CreativeEditorWorldLayoutElevationSourceKind::Count:
      break;
  }
  return false;
}

CreativeEditorWorldLayoutElevationPoint previewElevationHandlePosition(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    const CreativeEditorWorldLayoutElevationHandle& handle) {
  CreativeEditorWorldLayoutElevationPoint position = handle.position;
  if ((handle.kind ==
           CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow ||
       handle.kind ==
           CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh) &&
      handle.sourceIndex < state.source.verticalConnectors.size() &&
      state.verticalConnectorManipulation.active &&
      state.verticalConnectorManipulation.target.connectorIndex ==
          handle.sourceIndex &&
      state.verticalConnectorManipulation.previewValid) {
    const CreativeEditorWorldLayoutVerticalConnectorTarget target =
        creativeEditorWorldLayoutElevationConnectorTarget(state, handle);
    const cr::CreativeWorldLayoutRect footprint =
        state.verticalConnectorManipulation.previewFootprint;
    if (target.handle == CreativeEditorWorldLayoutRectHandle::East) {
      position.horizontal = footprint.maximum.x;
    } else if (target.handle == CreativeEditorWorldLayoutRectHandle::West) {
      position.horizontal = footprint.minimum.x;
    } else if (target.handle == CreativeEditorWorldLayoutRectHandle::North) {
      position.horizontal = footprint.minimum.z;
    } else if (target.handle == CreativeEditorWorldLayoutRectHandle::South) {
      position.horizontal = footprint.maximum.z;
    }
    return position;
  }
  const CreativeEditorWorldLayoutElevationManipulationState& manipulation =
      state.elevationManipulation;
  if (!manipulation.active || !manipulation.preview.accepted ||
      manipulation.handle.kind != handle.kind ||
      manipulation.handle.sourceKind != handle.sourceKind ||
      manipulation.handle.sourceIndex != handle.sourceIndex) {
    return position;
  }
  const CreativeEditorWorldLayoutElevationEditResult& edit =
      manipulation.preview;
  switch (handle.kind) {
    case CreativeEditorWorldLayoutElevationHandleKind::LevelFloor:
      position.vertical = edit.floorTopLayer;
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::WallTop:
      if (handle.sourceKind ==
              CreativeEditorWorldLayoutElevationSourceKind::Wall &&
          handle.sourceIndex < state.source.walls.size()) {
        position.vertical =
            state.source.walls[handle.sourceIndex].baseLayer +
            static_cast<double>(edit.wallHeightCells);
      } else if (handle.levelIndex < state.source.levels.size()) {
        position.vertical =
            state.source.levels[handle.levelIndex].floorTopLayer +
            static_cast<double>(edit.wallHeightCells);
      }
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::RoofRidge:
      for (const CreativeEditorWorldLayoutElevationLine& line :
           projection.lines) {
        if (line.kind ==
                CreativeEditorWorldLayoutElevationLineKind::RoofSlope &&
            line.levelIndex == handle.levelIndex) {
          constexpr double kDegreesToRadians =
              0.01745329251994329576923690768489;
          const double run =
              std::abs(line.end.horizontal - line.start.horizontal);
          position.vertical =
              line.start.vertical +
              std::tan(edit.roofPitchDegrees * kDegreesToRadians) * run;
          break;
        }
      }
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::OpeningBottom:
      if (handle.sourceIndex < state.source.openings.size()) {
        position.vertical +=
            edit.openingSillCells -
            state.source.openings[handle.sourceIndex].cutoutBottomCells;
      }
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::OpeningTop:
      if (handle.sourceIndex < state.source.openings.size()) {
        const cr::CreativeWorldLayoutOpening& opening =
            state.source.openings[handle.sourceIndex];
        position.vertical += edit.openingSillCells + edit.openingHeightCells -
                             opening.cutoutBottomCells -
                             opening.cutoutHeightCells;
      }
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunLow:
    case CreativeEditorWorldLayoutElevationHandleKind::ConnectorRunHigh:
      break;
    case CreativeEditorWorldLayoutElevationHandleKind::None:
    case CreativeEditorWorldLayoutElevationHandleKind::Count:
      break;
  }
  return position;
}

}  // namespace

void drawCreativeEditorWorldLayoutElevationSectionControls(
    CreativeEditorWorldLayoutState& state, bool interactionEnabled) {
  constexpr std::array<CreativeEditorWorldLayoutLevelEditScope, 4U>
      kLevelEditScopes = {
          CreativeEditorWorldLayoutLevelEditScope::Selected,
          CreativeEditorWorldLayoutLevelEditScope::SelectedAndAbove,
          CreativeEditorWorldLayoutLevelEditScope::SelectedAndBelow,
          CreativeEditorWorldLayoutLevelEditScope::All,
      };
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted("Level edit");
  ImGui::SameLine();
  ImGui::BeginDisabled(!interactionEnabled ||
                       state.elevationManipulation.active);
  ImGui::PushID("section_level_edit_scope");
  for (std::size_t index = 0U; index < kLevelEditScopes.size(); ++index) {
    if (index > 0U) {
      ImGui::SameLine();
    }
    const CreativeEditorWorldLayoutLevelEditScope scope =
        kLevelEditScopes[index];
    const std::string_view label =
        creativeEditorWorldLayoutLevelEditScopeLabel(scope);
    if (ImGui::RadioButton(label.data(),
                           state.elevationLevelEditScope == scope)) {
      state.elevationLevelEditScope = scope;
    }
  }
  ImGui::PopID();
  ImGui::EndDisabled();
}

void drawCreativeEditorWorldLayoutElevationProjection(
    ImDrawList& drawList, ImVec2 minimum, ImVec2 maximum,
    const CreativeEditorWorldLayoutElevationCanvasTransform& transform,
    const CreativeEditorWorldLayoutElevationProjection& projection,
    const CreativeEditorWorldLayoutState& state,
    const cr::CreativeGridSettings& grid,
    const cr::CreativeMeasurementAnnotationStore& measurementAnnotations,
    const cr::CreativeMeasurementState& measurement) {
  drawList.PushClipRect(minimum, maximum, true);
  drawList.AddRectFilled(minimum, maximum,
                         color({0.105F, 0.12F, 0.135F, 1.0F}));
  drawElevationGrid(drawList, minimum, maximum, transform);
  if (projection.accepted) {
    for (const CreativeEditorWorldLayoutElevationItem& item :
         projection.items) {
      ImVec2 itemMinimum = toElevationScreen(
          transform, {item.minimumHorizontal, item.maximumVertical});
      ImVec2 itemMaximum = toElevationScreen(
          transform, {item.maximumHorizontal, item.minimumVertical});
      if (itemMaximum.x - itemMinimum.x < 3.0F) {
        const float center = (itemMinimum.x + itemMaximum.x) * 0.5F;
        itemMinimum.x = center - 1.5F;
        itemMaximum.x = center + 1.5F;
      }
      if (itemMaximum.y - itemMinimum.y < 3.0F) {
        const float center = (itemMinimum.y + itemMaximum.y) * 0.5F;
        itemMinimum.y = center - 1.5F;
        itemMaximum.y = center + 1.5F;
      }
      drawList.AddRectFilled(itemMinimum, itemMaximum,
                             elevationItemColor(item.kind));
      drawList.AddRect(
          itemMinimum, itemMaximum,
          elevationItemSelected(state, item)
              ? color({0.30F, 0.95F, 0.42F, 1.0F})
              : color({0.70F, 0.74F, 0.78F, 0.92F}),
          0.0F, 0, elevationItemSelected(state, item) ? 2.4F : 1.0F);
    }
    for (const CreativeEditorWorldLayoutElevationLine& line :
         projection.lines) {
      const ImU32 lineColor =
          line.kind ==
                  CreativeEditorWorldLayoutElevationLineKind::ConnectorRise
              ? color({0.96F, 0.74F, 0.22F, 1.0F})
              : color({0.87F, 0.55F, 0.43F, 1.0F});
      drawList.AddLine(toElevationScreen(transform, line.start),
                       toElevationScreen(transform, line.end), lineColor,
                       2.4F);
    }
    for (const CreativeEditorWorldLayoutElevationHandle& handle :
         projection.handles) {
      if (!creativeEditorWorldLayoutElevationHandleVisible(state, handle)) {
        continue;
      }
      const CreativeEditorWorldLayoutElevationPoint position =
          previewElevationHandlePosition(state, projection, handle);
      const bool roofActive =
          handle.kind ==
              CreativeEditorWorldLayoutElevationHandleKind::RoofRidge &&
          state.roofManipulation.active &&
          state.roofManipulation.target.levelIndex == handle.levelIndex;
      const bool elevationActive =
          state.elevationManipulation.active &&
          state.elevationManipulation.handle.kind == handle.kind &&
          state.elevationManipulation.handle.sourceKind == handle.sourceKind &&
          state.elevationManipulation.handle.sourceIndex == handle.sourceIndex;
      const bool connectorActive =
          handle.sourceKind ==
              CreativeEditorWorldLayoutElevationSourceKind::
                  VerticalConnector &&
          state.verticalConnectorManipulation.active &&
          state.verticalConnectorManipulation.target.connectorIndex ==
              handle.sourceIndex;
      const bool active = roofActive || elevationActive || connectorActive;
      const bool valid =
          !active ||
          (roofActive
               ? state.roofManipulation.previewValid
               : connectorActive
                     ? state.verticalConnectorManipulation.previewValid
                     : state.elevationManipulation.preview.accepted);
      drawList.AddCircleFilled(
          toElevationScreen(transform, position), active ? 6.0F : 4.0F,
          valid ? color({0.30F, 0.95F, 0.42F, 1.0F})
                : color({0.95F, 0.24F, 0.20F, 1.0F}));
      drawList.AddCircle(toElevationScreen(transform, position),
                         active ? 6.0F : 4.0F,
                         color({0.06F, 0.07F, 0.08F, 1.0F}), 0, 1.2F);
    }
    drawElevationSectionAnnotations(drawList, minimum, maximum, transform,
                                    projection, state);
  }
  for (const cr::CreativeMeasurementAnnotation& annotation :
       measurementAnnotations.annotations) {
    drawMeasurementGeometry(
        drawList, transform, state.elevationAxis,
        projectCreativeEditorMeasurementGeometryToGrid(
            cr::buildCreativeMeasurementGeometry(annotation), grid),
        annotation.name, false);
  }
  const cr::CreativeMeasurementGeometry transientMeasurement =
      projectCreativeEditorMeasurementGeometryToGrid(
          cr::buildCreativeMeasurementGeometry(measurement), grid);
  drawMeasurementGeometry(
      drawList, transform, state.elevationAxis, transientMeasurement,
      transientMeasurement.visible
          ? formatCreativeEditorMeasurementReadout(measurement)
          : std::string{},
      true);
  drawList.PopClipRect();
}

}  // namespace iggy3d_creative_app
