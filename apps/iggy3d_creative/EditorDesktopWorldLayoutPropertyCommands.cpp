#include "EditorDesktopWorldLayoutCommandsInternal.hpp"

#include <type_traits>
#include <variant>

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;

namespace {

[[nodiscard]] creative::CreativeWorldLayoutTable worldLayoutPropertyTable(
    const CreativeDesktopWorldLayoutPropertySettings& settings) noexcept {
  return std::visit(
      []<typename Settings>(const Settings&) {
        return creativeDesktopWorldLayoutPropertyTable<Settings>();
      },
      settings);
}

CreativeEditorWorldLayoutEditReceipt applyWorldLayoutPropertySettings(
    CreativeEditorWorldLayoutState& state,
    std::size_t index,
    const CreativeDesktopWorldLayoutPropertySettings& settings,
    creative::CreativeGridSettings grid) {
  return std::visit(
      [&]<typename Settings>(const Settings& value) {
        if constexpr (std::is_same_v<
                          Settings,
                          CreativeEditorWorldLayoutLevelSettings>) {
          return setCreativeEditorWorldLayoutLevelSettings(state, index,
                                                           value);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutRoomMetadata>) {
          return setCreativeEditorWorldLayoutRoomMetadata(state, index,
                                                          value);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutRoomSettings>) {
          return setCreativeEditorWorldLayoutRoomSettings(state, index,
                                                          value);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutTopologyEdgeSettings>) {
          return setCreativeEditorWorldLayoutRoomEdgeSettings(
              state,
              {index, value.fixedEndpoint, value.lengthCells,
               value.wallThicknessCells, value.wallHeightCells,
               value.profile, value.material, value.joinStyle});
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutVerticalConnectorSettings>) {
          return setCreativeEditorWorldLayoutVerticalConnectorSettings(
              state, index, value, grid);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutBoxSettings>) {
          return setCreativeEditorWorldLayoutBoxSettings(state, index, value);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutWallSettings>) {
          return setCreativeEditorWorldLayoutWallSettings(state, index,
                                                          value);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutOpeningSettings>) {
          return setCreativeEditorWorldLayoutOpeningSettings(state, index,
                                                             value);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutRoofApertureSettings>) {
          return setCreativeEditorWorldLayoutRoofApertureSettings(
              state, index, value, grid);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutTerrainProfileSettings>) {
          return setCreativeEditorWorldLayoutTerrainProfileSettings(
              state, index, value);
        } else if constexpr (std::is_same_v<
                                 Settings,
                                 CreativeEditorWorldLayoutTerrainPathSettings>) {
          return setCreativeEditorWorldLayoutTerrainPathSettings(state, index,
                                                                 value);
        } else {
          static_assert(std::is_same_v<
                        Settings,
                        CreativeEditorWorldLayoutObjectSettings>);
          return setCreativeEditorWorldLayoutObjectSettings(state, index,
                                                            value);
        }
      },
      settings);
}

}  // namespace

bool dispatchCreativeDesktopWorldLayoutPropertyCommand(
    const CreativeDesktopCommand& command,
    const CreativeDesktopCommandContext& context,
    CreativeDesktopCommandResult& result) {
  creative::CreativeAppState& appState = context.appState;
  CreativeEditorState& editor = context.editor;
  switch (command.id) {
    case CreativeDesktopCommandId::WorldLayoutEditSourceProperty: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutPropertyEditPayload>(command);
      if (payload == nullptr ||
          payload->phase >= CreativeDesktopWorldLayoutPropertyEditPhase::Count) {
        result.message = "layout property edit: payload mismatch";
        break;
      }
      if (payload->table != worldLayoutPropertyTable(payload->settings)) {
        result.message = "layout property edit: settings type mismatch";
        break;
      }
      if (payload->phase ==
          CreativeDesktopWorldLayoutPropertyEditPhase::Cancel) {
        const bool ownsPreview =
            editor.worldLayout.propertyPreviewKey.active &&
            editor.worldLayout.propertyPreviewKey.table == payload->table &&
            editor.worldLayout.propertyPreviewKey.index == payload->index;
        result.sceneChanged =
            ownsPreview && clearCreativeEditorWorldLayoutLiveEditPreview(
                               editor.worldLayout);
        result.accepted = true;
        result.changed = result.sceneChanged;
        editor.worldLayout.statusMessage =
            result.changed ? "layout property preview canceled"
                           : "layout property preview already clear";
        result.message = editor.worldLayout.statusMessage;
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, payload->table, payload->index,
              payload->stableKey)) {
        result.message = "layout property edit: stale target";
        break;
      }
      if (payload->phase ==
              CreativeDesktopWorldLayoutPropertyEditPhase::Preview &&
          editor.worldLayout.liveEditPreviewVisible &&
          editor.worldLayout.propertyPreviewKey.active &&
          editor.worldLayout.propertyPreviewKey.sourceRevision ==
              editor.worldLayout.revision &&
          editor.worldLayout.propertyPreviewKey.table == payload->table &&
          editor.worldLayout.propertyPreviewKey.index == payload->index &&
          editor.worldLayout.propertyPreviewKey.settings == payload->settings) {
        result.accepted = true;
        editor.worldLayout.statusMessage =
            "property edit preview already current";
        result.message = editor.worldLayout.statusMessage;
        break;
      }

      const auto applyProperty = [&](CreativeEditorWorldLayoutState& target) {
        return applyWorldLayoutPropertySettings(target, payload->index,
                                                payload->settings,
                                                appState.facade.document()
                                                    .gridSettings());
      };
      CreativeDesktopWorldLayoutLiveEditResult propertyEdit;
      if (payload->phase ==
          CreativeDesktopWorldLayoutPropertyEditPhase::Preview) {
        propertyEdit = dispatchCreativeDesktopWorldLayoutPreviewEdit(
            editor.worldLayout, appState, applyProperty,
            "property edit preview ready in 3D");
        if (propertyEdit.accepted &&
            editor.worldLayout.liveEditPreviewVisible) {
          editor.worldLayout.propertyPreviewKey = {
              true, editor.worldLayout.revision, payload->table,
              payload->index, payload->settings};
        }
      } else {
        propertyEdit = dispatchCreativeDesktopWorldLayoutImmediateEdit(
            editor.worldLayout, appState, applyProperty,
            "desktop_world_layout_property_edit");
      }
      result.accepted = propertyEdit.accepted;
      result.changed = propertyEdit.changed;
      result.worldLayoutChanged = propertyEdit.worldLayoutChanged;
      result.sceneChanged = propertyEdit.sceneChanged;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    case CreativeDesktopCommandId::WorldLayoutSetObjectSettings: {
      const auto* payload =
          payloadAs<CreativeDesktopWorldLayoutObjectSettingsPayload>(command);
      if (payload == nullptr) {
        result.message = "layout object settings: payload mismatch";
        break;
      }
      if (!creativeEditorWorldLayoutSourceStableKeyMatches(
              editor.worldLayout, cr::CreativeWorldLayoutTable::Object,
              payload->objectIndex, payload->stableKey)) {
        result.message = "layout object settings: stale target";
        break;
      }
      const bool previewWasActive =
          creativeEditorWorldLayoutPreviewActive(editor.worldLayout);
      const CreativeEditorWorldLayoutEditReceipt receipt =
          setCreativeEditorWorldLayoutObjectSettings(
              editor.worldLayout, payload->objectIndex, payload->settings);
      result.accepted = receipt.accepted;
      result.changed = receipt.changed;
      result.worldLayoutChanged = receipt.changed;
      result.sceneChanged = previewWasActive && receipt.changed;
      result.message = editor.worldLayout.statusMessage;
      break;
    }
    default:
      return false;
  }
  return true;
}

}  // namespace iggy3d_creative_app
