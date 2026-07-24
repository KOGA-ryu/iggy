#pragma once

#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"

#include "app/iggy3d/creative/Geometry.hpp"

#include <variant>

namespace iggy3d::creative {

struct CreativeTerrainGenerationResult;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

template <typename Payload>
[[nodiscard]] const Payload* payloadAs(
    const CreativeDesktopCommand& command) {
  return std::get_if<Payload>(&command.payload);
}

[[nodiscard]] bool focusEditorCameraOnBounds(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeBounds bounds) noexcept;

[[nodiscard]] bool focusEditorCameraOnObject(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeObject& object) noexcept;

[[nodiscard]] bool focusEditorCameraOnSelection(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeSelectionState& selection) noexcept;

[[nodiscard]] bool focusEditorCameraOnDocument(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeDocument& document) noexcept;

[[nodiscard]] bool focusEditorCameraOnTerrainGeneration(
    CreativeEditorState& editor,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeTerrainGenerationResult& generation)
    noexcept;

bool dispatchCreativeDesktopDocumentCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopObjectCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopAssetCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopTerrainCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

bool dispatchCreativeDesktopPlayCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result);

}  // namespace iggy3d_creative_app
