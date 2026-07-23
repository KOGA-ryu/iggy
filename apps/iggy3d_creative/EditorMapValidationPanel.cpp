#include "EditorMapValidationPanel.hpp"

#include <string>
#include <string_view>

#include "imgui.h"

#include "EditorDesktopWidgets.hpp"

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

namespace {

ImVec4 severityColor(cr::CreativeMapDiagnosticSeverity severity) noexcept {
  switch (severity) {
    case cr::CreativeMapDiagnosticSeverity::Info:
      return {0.52F, 0.78F, 1.0F, 1.0F};
    case cr::CreativeMapDiagnosticSeverity::Warning:
      return {1.0F, 0.82F, 0.25F, 1.0F};
    case cr::CreativeMapDiagnosticSeverity::Error:
      return {1.0F, 0.34F, 0.30F, 1.0F};
  }
  return {0.68F, 0.72F, 0.75F, 1.0F};
}

void appendValidationState(
    CreativeEditorMapValidationCacheStatus status,
    const CreativeEditorMapValidationCache& cache) {
  switch (status) {
    case CreativeEditorMapValidationCacheStatus::NotRun:
      ImGui::TextDisabled("NOT RUN");
      return;
    case CreativeEditorMapValidationCacheStatus::Stale:
      ImGui::TextColored({1.0F, 0.72F, 0.20F, 1.0F}, "STALE");
      return;
    case CreativeEditorMapValidationCacheStatus::Current:
      ImGui::TextColored(cache.result.passed
                             ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                             : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                         "%s", cache.result.passed ? "PASSED" : "FAILED");
      return;
  }
}

std::string diagnosticSubject(const cr::CreativeDocument& document,
                              const cr::CreativeMapDiagnostic& diagnostic) {
  if (const cr::CreativeObject* object =
          document.findObject(diagnostic.objectId);
      object != nullptr) {
    return object->name;
  }
  if (!diagnostic.subject.empty()) {
    return diagnostic.subject;
  }
  return "Map";
}

void appendDiagnosticRows(
    const cr::CreativeDocument& document,
    const cr::CreativeMapValidationResult& result,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands) {
  if (result.diagnostics.empty()) {
    ImGui::TextDisabled("No map diagnostics");
    return;
  }

  constexpr ImGuiTableFlags kFlags =
      ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
  if (!ImGui::BeginTable("##creative_map_diagnostics", 6, kFlags,
                         ImVec2{0.0F, 0.0F})) {
    return;
  }
  ImGui::TableSetupScrollFreeze(0, 1);
  ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 72.0F);
  ImGui::TableSetupColumn("Area", ImGuiTableColumnFlags_WidthFixed, 96.0F);
  ImGui::TableSetupColumn("Subject", ImGuiTableColumnFlags_WidthStretch, 0.8F);
  ImGui::TableSetupColumn("Finding", ImGuiTableColumnFlags_WidthStretch, 1.2F);
  ImGui::TableSetupColumn("Repair", ImGuiTableColumnFlags_WidthStretch, 1.4F);
  ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 52.0F);
  ImGui::TableHeadersRow();

  for (std::size_t index = 0U; index < result.diagnostics.size(); ++index) {
    const cr::CreativeMapDiagnostic& diagnostic = result.diagnostics[index];
    const CreativeEditorMapDiagnosticDescriptor& descriptor =
        creativeEditorMapDiagnosticDescriptor(diagnostic.code);
    const std::string subject = diagnosticSubject(document, diagnostic);
    ImGui::PushID(static_cast<int>(index));
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextColored(severityColor(diagnostic.severity), "%s",
                       std::string(cr::toString(diagnostic.severity)).c_str());
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(
        creativeEditorMapDiagnosticAreaLabel(descriptor.area).data());
    ImGui::TableSetColumnIndex(2);
    ImGui::TextWrapped("%s", subject.c_str());
    ImGui::TableSetColumnIndex(3);
    ImGui::TextWrapped("%.*s", static_cast<int>(descriptor.title.size()),
                       descriptor.title.data());
    if (!diagnostic.detail.empty()) {
      ImGui::TextDisabled("%s", diagnostic.detail.c_str());
    }
    if (diagnostic.fact != 0U) {
      ImGui::TextDisabled("fact %llu",
                          static_cast<unsigned long long>(diagnostic.fact));
    }
    ImGui::TableSetColumnIndex(4);
    ImGui::TextWrapped("%.*s", static_cast<int>(descriptor.remediation.size()),
                       descriptor.remediation.data());
    ImGui::TableSetColumnIndex(5);
    ImGui::BeginDisabled(
        document.findObject(diagnostic.objectId) == nullptr);
    if (ImGui::SmallButton("Focus")) {
      queueCreativeDesktopObjectNavigation(commands, diagnostic.objectId,
                                            playModeActive);
    }
    ImGui::EndDisabled();
    ImGui::PopID();
  }
  ImGui::EndTable();
}

}  // namespace

void buildCreativeEditorMapValidationPanel(
    CreativeEditorMapValidationCache& cache,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog,
    bool playModeActive,
    CreativeDesktopCommandFrame& commands) {
  CreativeEditorMapValidationCacheStatus status =
      creativeEditorMapValidationCacheStatus(cache, document, assetCatalog);
  const bool force = status == CreativeEditorMapValidationCacheStatus::Current;
  if (ImGui::Button(force ? "Re-run validation" : "Run validation")) {
    static_cast<void>(refreshCreativeEditorMapValidation(
        cache, document, assetCatalog, force));
    status = CreativeEditorMapValidationCacheStatus::Current;
  }
  ImGui::SameLine();
  appendValidationState(status, cache);

  if (status == CreativeEditorMapValidationCacheStatus::NotRun) {
    ImGui::TextDisabled("No whole-map validation result");
    return;
  }

  ImGui::SameLine();
  ImGui::TextDisabled("rev %llu  run %llu",
                      static_cast<unsigned long long>(cache.documentRevision),
                      static_cast<unsigned long long>(cache.buildCount));
  ImGui::SameLine();
  ImGui::TextColored(cache.result.summary.errorCount == 0U
                         ? ImVec4{0.20F, 1.0F, 0.35F, 1.0F}
                         : ImVec4{1.0F, 0.34F, 0.30F, 1.0F},
                     "%u errors", cache.result.summary.errorCount);
  ImGui::SameLine();
  ImGui::TextColored({1.0F, 0.82F, 0.25F, 1.0F}, "%u warnings",
                     cache.result.summary.warningCount);
  ImGui::SameLine();
  ImGui::TextColored({0.52F, 0.78F, 1.0F, 1.0F}, "%u info",
                     cache.result.summary.infoCount);
  if (status == CreativeEditorMapValidationCacheStatus::Stale) {
    ImGui::TextColored({1.0F, 0.72F, 0.20F, 1.0F},
                       "Document or assets changed after this result");
  }
  if (cache.result.summary.diagnosticsTruncated) {
    ImGui::TextColored(
        {1.0F, 0.34F, 0.30F, 1.0F}, "%llu diagnostics omitted",
        static_cast<unsigned long long>(
            cache.result.summary.droppedDiagnosticCount));
  }
  ImGui::Separator();
  appendDiagnosticRows(document, cache.result, playModeActive, commands);
}

void buildCreativeEditorMapValidationPassStatus(
    const CreativeEditorMapValidationCache& cache,
    const cr::CreativeDocument& document,
    const iggy3d::StaticMeshAssetCatalog* assetCatalog) {
  const CreativeEditorMapValidationCacheStatus status =
      creativeEditorMapValidationCacheStatus(cache, document, assetCatalog);
  appendValidationState(status, cache);
  if (status == CreativeEditorMapValidationCacheStatus::NotRun) {
    ImGui::TextDisabled("Whole-map validation has not run");
    return;
  }
  if (status == CreativeEditorMapValidationCacheStatus::Stale) {
    ImGui::TextColored({1.0F, 0.72F, 0.20F, 1.0F},
                       "The displayed result is stale");
  }
  const cr::CreativeMapValidationResult& result = cache.result;
  ImGui::Text("Document %llu  revision %llu",
              static_cast<unsigned long long>(result.documentId),
              static_cast<unsigned long long>(result.documentRevision));
  ImGui::Text("Runtime objects %llu  player spawns %llu  assets %llu",
              static_cast<unsigned long long>(
                  result.summary.runtimeObjectCount),
              static_cast<unsigned long long>(
                  result.summary.playerSpawnCount),
              static_cast<unsigned long long>(
                  result.summary.referencedStaticMeshAssetCount));
  ImGui::Text("Room bake: %s", result.roomBake.reasonCode.c_str());
  ImGui::Text("Player spawn: %s",
              std::string(cr::toString(result.playerSpawn.status)).c_str());
  ImGui::TextDisabled("Warnings do not block validation; errors do.");
}

}  // namespace iggy3d_creative_app
