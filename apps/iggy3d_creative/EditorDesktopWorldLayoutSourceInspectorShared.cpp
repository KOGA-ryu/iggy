#include "EditorDesktopWorldLayoutSourceInspectorInternal.hpp"

namespace iggy3d_creative_app::detail {
namespace {

int inputTextResizeCallback(ImGuiInputTextCallbackData* data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto* value = static_cast<std::string*>(data->UserData);
    value->resize(static_cast<std::size_t>(data->BufTextLen));
    data->Buf = value->data();
  }
  return 0;
}

}  // namespace

bool inputText(const char* label, std::string& value) {
  return ImGui::InputText(label, value.data(), value.capacity() + 1U,
                          ImGuiInputTextFlags_CallbackResize,
                          inputTextResizeCallback, &value);
}

void drawStableKey(std::string_view stableKey, std::string_view type) {
  ImGui::TextDisabled("%.*s  %.*s", static_cast<int>(type.size()),
                      type.data(), static_cast<int>(stableKey.size()),
                      stableKey.data());
}

void drawRetainingEdgeSettings(
    std::string_view profileKey,
    cr::CreativeTerrainLandformRecipe& landform,
    bool& enabled,
    cr::CreativeRetainingEdgeSourceRecipe& source,
    CreativeDesktopPropertyEditActivity& activity) {
  if (landform.edge != cr::CreativeTerrainLandformEdge::Retaining) {
    return;
  }

  ImGui::SeparatorText("Retaining edge kit");
  const bool enabledChanged = ImGui::Checkbox(
      "Generate retaining walls##layout_retaining_properties", &enabled);
  observeCreativeDesktopDiscretePropertyWidget(activity, enabledChanged);
  if (enabledChanged && enabled) {
    source = {};
    source.terrainProfileKey = std::string(profileKey);
  }
  if (!enabled) {
    return;
  }

  constexpr std::array selections{
      cr::CreativeRetainingEdgeSelection::All,
      cr::CreativeRetainingEdgeSelection::Internal,
      cr::CreativeRetainingEdgeSelection::Perimeter};
  constexpr std::array kits{cr::CreativeRetainingEdgeKit::Procedural,
                            cr::CreativeRetainingEdgeKit::InfrastructureStone};
  constexpr std::array materials{
      cr::CreativeStructuralMaterial::Blockout,
      cr::CreativeStructuralMaterial::Plaster,
      cr::CreativeStructuralMaterial::Timber,
      cr::CreativeStructuralMaterial::Stone,
      cr::CreativeStructuralMaterial::Brick};
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo(
          "Selection##layout_retaining_properties",
          source.settings.selection, selections,
          [](cr::CreativeRetainingEdgeSelection selection) {
            switch (selection) {
              case cr::CreativeRetainingEdgeSelection::All:
                return std::string_view{"All hard edges"};
              case cr::CreativeRetainingEdgeSelection::Internal:
                return std::string_view{"Internal risers"};
              case cr::CreativeRetainingEdgeSelection::Perimeter:
                return std::string_view{"Perimeter"};
              case cr::CreativeRetainingEdgeSelection::Count:
                break;
            }
            return std::string_view{"Invalid"};
          }));
  ImGui::SetNextItemWidth(180.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Kit##layout_retaining_properties", source.settings.kit,
                kits, [](cr::CreativeRetainingEdgeKit kit) {
                  switch (kit) {
                    case cr::CreativeRetainingEdgeKit::Procedural:
                      return std::string_view{"Procedural"};
                    case cr::CreativeRetainingEdgeKit::InfrastructureStone:
                      return std::string_view{"Infrastructure stone"};
                    case cr::CreativeRetainingEdgeKit::Count:
                      break;
                  }
                  return std::string_view{"Invalid"};
                }));
  ImGui::SetNextItemWidth(140.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Thickness##layout_retaining_properties",
                         &source.settings.thicknessMeters, 0.05, 0.25,
                         "%.2f m"));
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopContinuousPropertyWidget(
      activity,
      ImGui::InputDouble("Maximum height##layout_retaining_properties",
                         &source.settings.maximumHeightMeters, 0.5, 2.0,
                         "%.2f m"));
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Close corners##layout_retaining_properties",
                      &source.settings.closeCorners));
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      ImGui::Checkbox("Cap open ends##layout_retaining_properties",
                      &source.settings.capEnds));
  ImGui::SetNextItemWidth(160.0F);
  observeCreativeDesktopDiscretePropertyWidget(
      activity,
      enumCombo("Material##layout_retaining_properties",
                source.settings.material, materials,
                [](cr::CreativeStructuralMaterial material) {
                  return cr::toString(material);
                }));

  if (!ImGui::TreeNodeEx("Transitions##layout_retaining_properties")) {
    return;
  }
  std::size_t removeIndex = cr::kCreativeRetainingEdgeTransitionCapacity;
  constexpr std::array transitionKinds{
      cr::CreativeRetainingEdgeTransitionKind::Stair,
      cr::CreativeRetainingEdgeTransitionKind::Ramp};
  for (std::size_t index = 0U;
       index < source.settings.transitionCount; ++index) {
    cr::CreativeRetainingEdgeTransition& transition =
        source.settings.transitions[index];
    ImGui::PushID(static_cast<int>(index));
    ImGui::SeparatorText(("Transition " + std::to_string(index + 1U)).c_str());
    bool edgeChanged = false;
    const auto coord = [&](const char* label,
                           cr::CreativeTerrainCoord2& value) {
      ImGui::SetNextItemWidth(90.0F);
      edgeChanged =
          ImGui::InputScalar((std::string(label) + " X").c_str(),
                             ImGuiDataType_S32, &value.x) ||
          edgeChanged;
      ImGui::SameLine();
      ImGui::SetNextItemWidth(90.0F);
      edgeChanged =
          ImGui::InputScalar((std::string(label) + " Z").c_str(),
                             ImGuiDataType_S32, &value.z) ||
          edgeChanged;
    };
    coord("First", transition.edge.first);
    coord("Second", transition.edge.second);
    if (edgeChanged) {
      const std::int64_t deltaX =
          static_cast<std::int64_t>(transition.edge.second.x) -
          transition.edge.first.x;
      const std::int64_t deltaZ =
          static_cast<std::int64_t>(transition.edge.second.z) -
          transition.edge.first.z;
      if (std::abs(deltaX) + std::abs(deltaZ) == 1) {
        transition.edge = cr::canonicalCreativeTerrainHardEdge(
            transition.edge.first, transition.edge.second);
      }
      observeCreativeDesktopContinuousPropertyWidget(activity, true);
    }
    ImGui::SetNextItemWidth(130.0F);
    observeCreativeDesktopDiscretePropertyWidget(
        activity,
        enumCombo("Type", transition.kind, transitionKinds,
                  [](cr::CreativeRetainingEdgeTransitionKind kind) {
                    return kind ==
                                   cr::CreativeRetainingEdgeTransitionKind::Stair
                               ? std::string_view{"Stair"}
                               : std::string_view{"Ramp"};
                  }));
    ImGui::SetNextItemWidth(120.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputScalar("Run cells", ImGuiDataType_U16,
                           &transition.runCells));
    if (ImGui::SmallButton("Remove transition")) {
      removeIndex = index;
    }
    ImGui::PopID();
  }
  if (removeIndex < source.settings.transitionCount) {
    for (std::size_t index = removeIndex + 1U;
         index < source.settings.transitionCount; ++index) {
      source.settings.transitions[index - 1U] =
          source.settings.transitions[index];
    }
    --source.settings.transitionCount;
    source.settings.transitions[source.settings.transitionCount] = {};
    observeCreativeDesktopDiscretePropertyWidget(activity, true);
  }

  ImGui::BeginDisabled(
      source.settings.transitionCount >=
      cr::kCreativeRetainingEdgeTransitionCapacity);
  if (ImGui::Button("Add stair/ramp transition")) {
    cr::CreativeTerrainCoord2 first = landform.bounds.minimum;
    cr::CreativeTerrainCoord2 second = first;
    if (landform.bounds.widthCells > 1U) {
      ++second.x;
    } else {
      ++second.z;
    }
    source.settings.transitions[source.settings.transitionCount] = {
        cr::canonicalCreativeTerrainHardEdge(first, second),
        cr::CreativeRetainingEdgeTransitionKind::Stair,
        3U,
    };
    ++source.settings.transitionCount;
    observeCreativeDesktopDiscretePropertyWidget(activity, true);
  }
  ImGui::EndDisabled();
  ImGui::TextDisabled("Transition seam must match a generated hard edge");
  ImGui::TreePop();
}

void drawTerrainImpactSummary(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument& document,
    cr::CreativeWorldLayoutTable table,
    std::size_t index,
    std::string_view stableKey,
    CreativeDesktopCommandFrame& commands) {
  const CreativeEditorWorldLayoutDiagnosticCache& cache =
      state.diagnosticCache;
  const bool cacheCurrent =
      cache.valid && cache.sourceEpoch == state.sourceEpoch &&
      cache.layoutRevision == state.revision &&
      cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.terrainRevision == document.terrainField().revision() &&
      cache.materialRevision == document.terrainMaterialField().revision();
  const cr::CreativeWorldLayoutTerrainSourceImpact* impact =
      cacheCurrent
          ? cr::findCreativeWorldLayoutTerrainSourceImpact(
                cache.report.terrainImpactPlan, table, index)
          : nullptr;
  if (impact == nullptr) {
    ImGui::TextDisabled("Impact unavailable");
    return;
  }

  const bool pending = state.generatedRevision != state.revision;
  const bool current =
      !pending && impact->status ==
                      cr::CreativeWorldLayoutTerrainImpactStatus::Current;
  const bool noEffect =
      impact->status == cr::CreativeWorldLayoutTerrainImpactStatus::NoEffect;
  const ImVec4 statusColor =
      current ? ImVec4{0.45F, 0.95F, 0.48F, 1.0F}
              : noEffect ? ImVec4{0.65F, 0.68F, 0.72F, 1.0F}
                         : ImVec4{1.0F, 0.32F, 0.28F, 1.0F};
  const std::string_view status = pending ? "Pending generation"
                                          : cr::toString(impact->status);
  ImGui::TextColored(statusColor, "Impact: %.*s",
                     static_cast<int>(status.size()), status.data());
  ImGui::TextDisabled("%zu controls  %zu material cells",
                      impact->controls.size(), impact->materials.size());

  cr::CreativeBounds worldBounds{};
  const bool hasWorldBounds =
      cr::creativeWorldLayoutTerrainImpactWorldBounds(
          *impact, document.gridSettings(), worldBounds);
  if (hasWorldBounds) {
    const cr::CreativeBoundsMetrics metrics =
        cr::measureCreativeBounds(worldBounds);
    if (metrics.valid) {
      ImGui::TextDisabled("Bounds %.2f x %.2f x %.2f m", metrics.size.x,
                          metrics.size.y, metrics.size.z);
    }
  }
  ImGui::BeginDisabled(pending || !hasWorldBounds);
  if (ImGui::Button("Frame in 3D##terrain_impact")) {
    commands.enqueue(CreativeDesktopCommandId::WorldLayoutFrameSourceScope3D,
                  CreativeDesktopWorldLayoutSourcePayload{
                      table, index, std::string(stableKey)});
  }
  ImGui::EndDisabled();
}

void drawVec3Table(const char* id, const char* firstLabel,
                   cr::CreativeVec3& first,
                   CreativeDesktopPropertyEditActivity& activity,
                   const char* secondLabel,
                   cr::CreativeVec3* second) {
  if (!ImGui::BeginTable(id, 4, ImGuiTableFlags_SizingStretchSame |
                                   ImGuiTableFlags_BordersInnerV)) {
    return;
  }
  ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 42.0F);
  ImGui::TableSetupColumn("X");
  ImGui::TableSetupColumn("Y");
  ImGui::TableSetupColumn("Z");
  ImGui::TableHeadersRow();
  const auto row = [&activity](const char* label, cr::CreativeVec3& value) {
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("##x", &value.x, 0.25, 1.0, "%.3f"));
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("##y", &value.y, 0.25, 1.0, "%.3f"));
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0F);
    observeCreativeDesktopContinuousPropertyWidget(
        activity,
        ImGui::InputDouble("##z", &value.z, 0.25, 1.0, "%.3f"));
    ImGui::PopID();
  };
  row(firstLabel, first);
  if (secondLabel != nullptr && second != nullptr) {
    row(secondLabel, *second);
  }
  ImGui::EndTable();
}


}  // namespace iggy3d_creative_app::detail
