#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/world/WorldLayoutVerticalConnectors.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorDesktopUiState;

[[nodiscard]] const char* creativeEditorWorldLayoutVerticalConnectorKindLabel(
    iggy3d::creative::CreativeWorldLayoutVerticalConnectorKind kind) noexcept;
[[nodiscard]] const char*
creativeEditorWorldLayoutVerticalConnectorDirectionLabel(
    iggy3d::creative::CreativeWorldLayoutVerticalDirection direction) noexcept;
[[nodiscard]] iggy3d::creative::CreativeWorldLayoutVerticalDirection
oppositeCreativeEditorWorldLayoutVerticalConnectorDirection(
    iggy3d::creative::CreativeWorldLayoutVerticalDirection direction) noexcept;
[[nodiscard]] bool drawCreativeEditorWorldLayoutVerticalConnectorMaterial(
    const char* label,
    iggy3d::creative::CreativeStructuralMaterial& material);
[[nodiscard]] iggy3d::creative::CreativeWorldLayoutVerticalConnectorPlan
planCreativeEditorWorldLayoutVerticalConnectorSettings(
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeDocument& document,
    std::size_t connectorIndex,
    const CreativeEditorWorldLayoutVerticalConnectorSettings& settings);
void drawCreativeEditorWorldLayoutVerticalConnectorPlan(
    const iggy3d::creative::CreativeWorldLayoutVerticalConnectorPlan& plan,
    iggy3d::creative::CreativeWorldLayoutVerticalConnectorKind kind);

void drawCreativeEditorWorldLayoutVerticalConnectorInspector(
    CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutStructureInspector(
    CreativeEditorDesktopUiState& desktopUi,
    CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutSourceInspector(
    CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeDocument& document,
    CreativeDesktopCommandFrame& commands);

void drawCreativeEditorWorldLayoutRoofApertureInspector(
    CreativeEditorWorldLayoutState& state,
    std::size_t apertureIndex,
    CreativeDesktopCommandFrame& commands);

}  // namespace iggy3d_creative_app
