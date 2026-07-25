#include "EditorDesktopInspectorInternal.hpp"

#include <string>

#include "EditorObjectActions.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void appendTransformFields(
    CreativeDesktopInspectorDraft& draft,
    cr::CreativeObjectId objectId,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands) {
  const auto commit =
      [&](bool setPosition, bool setRotation, bool setScale) {
        const CreativeDesktopTransformDraft validated =
            validateCreativeDesktopTransformDraft(
                {draft.position[0], draft.position[1],
                 draft.position[2]},
                {draft.rotationDegrees[0], draft.rotationDegrees[1],
                 draft.rotationDegrees[2]},
                {draft.scale[0], draft.scale[1], draft.scale[2]});
        if (!validated.valid) {
          draft.validation = validated.message;
          return;
        }
        draft.validation.clear();
        commands.enqueue(
            CreativeDesktopCommandId::SetObjectTransform,
            CreativeDesktopTransformPayload{
                objectId, validated.transform, setPosition,
                setRotation, setScale});
      };

  ImGui::SeparatorText("Transform");
  ImGui::BeginDisabled(fieldsDisabled);
  ImGui::InputScalarN(
      "Position", ImGuiDataType_Double, draft.position.data(), 3);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit(true, false, false);
  }
  ImGui::InputScalarN(
      "Rotation (deg)", ImGuiDataType_Double,
      draft.rotationDegrees.data(), 3);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit(false, true, false);
  }
  ImGui::InputScalarN(
      "Scale", ImGuiDataType_Double, draft.scale.data(), 3);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    commit(false, false, true);
  }
  ImGui::EndDisabled();
  if (!draft.validation.empty()) {
    ImGui::TextColored(
        ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%s",
        draft.validation.c_str());
  }
}

void appendGroupPivotFields(
    CreativeDesktopInspectorDraft& draft,
    cr::CreativeObjectId groupObjectId,
    bool fieldsDisabled,
    CreativeDesktopCommandFrame& commands) {
  ImGui::SeparatorText("Group Pivot");
  ImGui::TextDisabled("Moves the manipulation pivot, not the members");
  ImGui::BeginDisabled(fieldsDisabled);
  ImGui::InputScalarN(
      "Pivot", ImGuiDataType_Double, draft.position.data(), 3);
  draft.editing = draft.editing || ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    const cr::CreativeVec3 pivot{
        draft.position[0], draft.position[1], draft.position[2]};
    if (cr::isFiniteCreativeVec3(pivot)) {
      draft.validation.clear();
      commands.enqueue(
          CreativeDesktopCommandId::SetGroupPivot,
          CreativeDesktopGroupPivotPayload{groupObjectId, pivot});
    } else {
      draft.validation = "group pivot must be finite";
    }
  }
  ImGui::EndDisabled();
  if (!draft.validation.empty()) {
    ImGui::TextColored(
        ImVec4{1.0F, 0.34F, 0.30F, 1.0F}, "%s",
        draft.validation.c_str());
  }
}

}  // namespace

void appendCreativeDesktopSingleObjectInspector(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeDesktopInspectorSelectionModel& selection,
    CreativeEditorWorldLayoutState& worldLayout,
    CreativeDesktopGeneratedSourceScopeCache&
        generatedSourceScopeCache,
    const CreativeMovingPlatformPreviewState& preview,
    const CreativeMovingPlatformPathEditState& pathEdit,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands) {
  const cr::CreativeDocument& document = selection.document;
  const cr::CreativeObject& object = *selection.object;
  const cr::CreativeSemanticObjectActionFacts& actionFacts =
      selection.objectActions.facts;
  const cr::CreativeSemanticObjectActionAdmissions& actionAdmissions =
      selection.objectActions.admissions;
  CreativeDesktopInspectorDraft& draft = desktopUi.inspectorDraft;
  refreshCreativeDesktopInspectorDraft(draft, document, object);
  draft.editing = false;

  const cr::CreativeWorldLayoutObjectProvenance provenance =
      actionFacts.hasSingleSelection
          ? actionFacts.singleSelection.worldLayoutSource
          : cr::CreativeWorldLayoutObjectProvenance{};
  const cr::CreativeObjectHierarchyState hierarchyState =
      cr::resolveCreativeObjectHierarchyState(document, object.id);
  const bool sourceSynchronized =
      actionFacts.worldLayoutSynchronized;
  const CreativeEditorObjectActionCapability transformAdmission =
      creativeEditorObjectActionCapability(
          actionAdmissions,
          cr::CreativeSemanticObjectAction::SetTransform);
  const bool sourceSupportsAdoption =
      transformAdmission.route ==
      cr::CreativeSemanticObjectActionRoute::RefineThenAdopt;
  const bool sourceAdoptionAvailable =
      sourceSupportsAdoption && transformAdmission.allowed;
  const bool sourceOwnedOnly =
      provenance.owned && !sourceSupportsAdoption;
  const bool generatedSettingsSource = provenance.owned;
  if (!generatedSettingsSource &&
      (worldLayout.generatedBuildingDraft.active ||
       worldLayout.generatedLevelSettingsDraft.active ||
       worldLayout.roomSettingsDraft.active ||
       worldLayout.verticalConnectorSettingsDraft.active ||
       worldLayout.wallSettingsDraft.active ||
       worldLayout.openingSettingsDraft.active)) {
    worldLayout.generatedBuildingDraft = {};
    worldLayout.generatedLevelSettingsDraft = {};
    worldLayout.roomSettingsDraft = {};
    worldLayout.verticalConnectorSettingsDraft = {};
    worldLayout.wallSettingsDraft = {};
    worldLayout.openingSettingsDraft = {};
    if (creativeEditorWorldLayoutPreviewActive(worldLayout)) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutCancelGeneratedSettingsPreview);
    }
  }

  const auto actionAvailable =
      [&](cr::CreativeSemanticObjectAction action) {
        return creativeEditorObjectActionAvailable(
            actionAdmissions, action);
      };
  const bool rawFieldsDisabled =
      selection.playModeActive ||
      !actionAvailable(
          cr::CreativeSemanticObjectAction::StructuralMutation);
  const bool renameDisabled =
      selection.playModeActive ||
      !actionAvailable(cr::CreativeSemanticObjectAction::Rename);
  const bool transformDisabled =
      selection.playModeActive ||
      !actionAvailable(
          cr::CreativeSemanticObjectAction::SetTransform);

  ImGui::BeginDisabled(renameDisabled);
  if (creativeDesktopInputTextStdString(
          "Name", &draft.name,
          ImGuiInputTextFlags_EnterReturnsTrue) &&
      !draft.name.empty()) {
    commands.enqueue(
        CreativeDesktopCommandId::RenameObject,
        CreativeDesktopRenamePayload{object.id, draft.name});
  }
  draft.editing = draft.editing || ImGui::IsItemActive();
  ImGui::EndDisabled();
  ImGui::TextDisabled(
      "%s  |  id %llu",
      std::string(cr::toString(object.kind)).c_str(),
      static_cast<unsigned long long>(object.id));

  ImGui::BeginDisabled(
      selection.playModeActive ||
      !actionAvailable(
          cr::CreativeSemanticObjectAction::SetVisible));
  bool visible = object.visible;
  if (ImGui::Checkbox("Visible", &visible)) {
    commands.enqueue(
        CreativeDesktopCommandId::SetObjectsVisible,
        CreativeDesktopObjectFlagPayload{{object.id}, visible});
  }
  ImGui::SameLine();
  ImGui::EndDisabled();
  ImGui::BeginDisabled(
      selection.playModeActive ||
      !actionAvailable(
          cr::CreativeSemanticObjectAction::SetLocked));
  bool locked = object.locked;
  if (ImGui::Checkbox("Locked", &locked)) {
    commands.enqueue(
        CreativeDesktopCommandId::SetObjectsLocked,
        CreativeDesktopObjectFlagPayload{{object.id}, locked});
  }
  ImGui::EndDisabled();

  if (hierarchyState.hiddenByObjectId != cr::kInvalidObjectId &&
      hierarchyState.hiddenByObjectId != object.id) {
    ImGui::TextDisabled(
        "Hidden by ancestor %llu",
        static_cast<unsigned long long>(
            hierarchyState.hiddenByObjectId));
  }
  if (hierarchyState.lockedByObjectId != cr::kInvalidObjectId &&
      hierarchyState.lockedByObjectId != object.id) {
    ImGui::TextDisabled(
        "Locked by ancestor %llu",
        static_cast<unsigned long long>(
            hierarchyState.lockedByObjectId));
  }

  if (object.kind == cr::CreativeObjectKind::Group) {
    appendGroupPivotFields(
        draft, object.id, rawFieldsDisabled, commands);
  } else if (cr::creativeObjectIsHierarchyContainer(object.kind)) {
    ImGui::SeparatorText("Transform");
    ImGui::TextDisabled(
        "Use Transform Selection to move the complete instance");
  } else {
    appendTransformFields(
        draft, object.id, transformDisabled, commands);
  }
  appendCreativeDesktopPlayerSpawnFields(
      draft, object, rawFieldsDisabled, commands);
  appendCreativeDesktopNpcSpawnFields(
      draft, document, object, rawFieldsDisabled, commands);
  appendCreativeDesktopLootPointFields(
      draft, object, rawFieldsDisabled, commands);
  appendCreativeDesktopExitPointFields(
      draft, document, object, rawFieldsDisabled, commands);
  appendCreativeDesktopMovingPlatformFields(
      draft, object, preview, pathEdit, rawFieldsDisabled, commands);

  if (!object.assetId.empty()) {
    ImGui::TextDisabled("Asset: %s", object.assetId.c_str());
  }
  if (object.parentId.has_value()) {
    ImGui::TextDisabled(
        "Parent: %llu",
        static_cast<unsigned long long>(*object.parentId));
  }
  if (!object.attachmentSocket.empty()) {
    ImGui::TextDisabled(
        "Socket: %s", object.attachmentSocket.c_str());
  }
  for (const std::string& tag : object.tags) {
    ImGui::BulletText("%s", tag.c_str());
  }

  if (provenance.owned) {
    ImGui::SeparatorText("World Layout");
    ImGui::BeginDisabled(
        selection.playModeActive || !sourceAdoptionAvailable);
    if (ImGui::Button("Adopt 3D Edit")) {
      commands.enqueue(
          CreativeDesktopCommandId::WorldLayoutAdoptObjectSource,
          CreativeDesktopWorldLayoutObjectSourcePayload{object.id});
    }
    ImGui::EndDisabled();
    if (!sourceSynchronized) {
      ImGui::TextDisabled(
          "Generate pending 2D edits before adoption");
    } else if (sourceOwnedOnly) {
      ImGui::TextDisabled(
          "Source-owned geometry; raw object edits are locked");
    }
    appendCreativeDesktopGeneratedSourceSettings(
        worldLayout, generatedSourceScopeCache, document, object.id,
        provenance,
        selection.playModeActive || !sourceSynchronized, commands);
  }

  ImGui::BeginDisabled(
      selection.playModeActive ||
      !actionAvailable(
          cr::CreativeSemanticObjectAction::Duplicate));
  if (ImGui::Button("Duplicate##single")) {
    commands.enqueue(CreativeDesktopCommandId::DuplicateSelection);
  }
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(
      selection.playModeActive ||
      !actionAvailable(cr::CreativeSemanticObjectAction::Delete));
  if (ImGui::Button("Delete##single")) {
    commands.enqueue(CreativeDesktopCommandId::DeleteSelection);
  }
  ImGui::EndDisabled();

  if (draft.editing && input.cancelPressed) {
    draft.valid = false;
  }
}

}  // namespace iggy3d_creative_app
