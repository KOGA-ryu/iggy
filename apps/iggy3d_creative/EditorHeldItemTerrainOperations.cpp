#include "EditorHeldItemWorldOperationsInternal.hpp"

#include "EditorState.hpp"
#include "EditorTerrain.hpp"
#include "EditorVolume.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void upsertTerrainControl(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorTerrainEditWithHistory(
      context.request.appState, context.request.editor,
      CreativeEditorTerrainEditKind::Upsert,
      "minecraft_terrain_rod_upsert"));
}

void removeTerrainControl(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorTerrainEditWithHistory(
      context.request.appState, context.request.editor,
      CreativeEditorTerrainEditKind::Remove,
      "minecraft_terrain_rod_remove"));
}

void sampleTerrainControl(CreativeHeldItemWorldOperationContext& context) {
  const CreativeEditorTerrainEditReceipt receipt =
      applyCreativeEditorTerrainEditWithHistory(
          context.request.appState, context.request.editor,
          CreativeEditorTerrainEditKind::Sample,
          "minecraft_terrain_rod_sample");
  if (receipt.accepted &&
      context.request.editor.toolSettings.terrainRodStampMode ==
          cr::CreativeTerrainRodStampMode::Seed) {
    context.request.editor.terrain.selectionValid = false;
  }
}

void beginTerrainGrade(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(beginCreativeEditorTerrainGrade(
      context.request.appState, context.request.editor));
}

void applyTerrainGrade(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorTerrainGradeWithHistory(
      context.request.appState, context.request.editor,
      "minecraft_terrain_grade_apply"));
}

void cancelTerrainGrade(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(
      cancelCreativeEditorTerrainGrade(context.request.editor));
}

void applyTerrainProfile(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorTerrainProfileWithHistory(
      context.request.appState, context.request.editor,
      "minecraft_terrain_profile_apply"));
}

void lockTerrainProfileBase(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(lockCreativeEditorTerrainProfileBase(
      context.request.appState.facade.document(), context.request.editor));
}

void unlockTerrainProfileBase(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(
      unlockCreativeEditorTerrainProfileBase(context.request.editor));
}

void addTerrainPathPoint(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(addCreativeEditorTerrainPathPoint(
      context.request.appState.facade.document(), context.request.editor));
}

void applyTerrainPath(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(applyCreativeEditorTerrainPathWithHistory(
      context.request.appState, context.request.editor,
      "minecraft_terrain_path_apply"));
}

void removeTerrainPathPoint(CreativeHeldItemWorldOperationContext& context) {
  static_cast<void>(
      removeCreativeEditorTerrainPathPoint(context.request.editor));
}

void applyTerrainRegion(CreativeHeldItemWorldOperationContext& context) {
  if (context.request.editor.terrain.region.stamp.active) {
    static_cast<void>(applyCreativeEditorTerrainStampWithHistory(
        context.request.appState, context.request.editor,
        "minecraft_terrain_stamp_apply"));
    return;
  }
  static_cast<void>(applyCreativeEditorTerrainRegionWithHistory(
      context.request.appState, context.request.editor,
      "minecraft_terrain_region_apply"));
}

void advanceTerrainRegion(CreativeHeldItemWorldOperationContext& context) {
  CreativeEditorState& editor = context.request.editor;
  if (cr::creativeVolumeSelectionComplete(editor.volume.selection)) {
    applyTerrainRegion(context);
    return;
  }
  advanceCreativeHeldItemVolumeSelection(context);
}

void sampleTerrainRegionHeight(
    CreativeHeldItemWorldOperationContext& context) {
  if (context.request.editor.terrain.region.stamp.active) {
    return;
  }
  const CreativeEditorTerrainRegionReceipt selected =
      selectCreativeEditorTerrainRegionOperationAtPointer(
          context.request.appState.facade.document(),
          context.request.editor);
  if (selected.accepted) {
    return;
  }
  static_cast<void>(sampleCreativeEditorTerrainRegionHeight(
      context.request.appState.facade.document(),
      context.request.editor));
}

void cancelTerrainRegion(CreativeHeldItemWorldOperationContext& context) {
  if (context.request.editor.terrain.region.stamp.active) {
    static_cast<void>(
        cancelCreativeEditorTerrainStamp(context.request.editor));
    return;
  }
  static_cast<void>(
      cancelCreativeEditorTerrainRegion(context.request.editor));
}

}  // namespace

void executeCreativeHeldItemTerrainOperation(
    cr::CreativeHeldItemWorldOperation operation,
    CreativeHeldItemWorldOperationContext& context) {
  switch (operation) {
    case cr::CreativeHeldItemWorldOperation::UpsertTerrainControl:
      upsertTerrainControl(context);
      return;
    case cr::CreativeHeldItemWorldOperation::RemoveTerrainControl:
      removeTerrainControl(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SampleTerrainControl:
      sampleTerrainControl(context);
      return;
    case cr::CreativeHeldItemWorldOperation::BeginTerrainGrade:
      beginTerrainGrade(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyTerrainGrade:
      applyTerrainGrade(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CancelTerrainGrade:
      cancelTerrainGrade(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyTerrainProfile:
      applyTerrainProfile(context);
      return;
    case cr::CreativeHeldItemWorldOperation::LockTerrainProfileBase:
      lockTerrainProfileBase(context);
      return;
    case cr::CreativeHeldItemWorldOperation::UnlockTerrainProfileBase:
      unlockTerrainProfileBase(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AddTerrainPathPoint:
      addTerrainPathPoint(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyTerrainPath:
      applyTerrainPath(context);
      return;
    case cr::CreativeHeldItemWorldOperation::RemoveTerrainPathPoint:
      removeTerrainPathPoint(context);
      return;
    case cr::CreativeHeldItemWorldOperation::ApplyTerrainRegion:
      applyTerrainRegion(context);
      return;
    case cr::CreativeHeldItemWorldOperation::AdvanceTerrainRegion:
      advanceTerrainRegion(context);
      return;
    case cr::CreativeHeldItemWorldOperation::SampleTerrainRegionHeight:
      sampleTerrainRegionHeight(context);
      return;
    case cr::CreativeHeldItemWorldOperation::CancelTerrainRegion:
      cancelTerrainRegion(context);
      return;
    default:
      return;
  }
}

}  // namespace iggy3d_creative_app
