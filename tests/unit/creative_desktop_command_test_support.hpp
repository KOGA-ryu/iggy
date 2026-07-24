#pragma once

#include "EditorDesktopCommands.hpp"

#include "EditorEdits.hpp"
#include "EditorFrame.hpp"
#include "EditorObjectActions.hpp"
#include "EditorPersistence.hpp"
#include "EditorPlaytestProcess.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutDiagnostics.hpp"
#include "EditorWorldLayoutHistory.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "app/iggy3d/creative/world/MapTemplate.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "content/assets/StaticMeshAsset.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <numbers>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

// UI-2a (docs/creative_desktop_ui_plan.md DD-7): the desktop UI emits semantic
// command IDs into a bounded frame; EditorDesktopCommands.cpp is the sole
// dispatcher, reusing the existing kernels with the same history discipline as
// the keyboard path. Fully headless — no ImGui, no window.

namespace {
namespace cr = iggy3d::creative;
namespace app = iggy3d_creative_app;

[[maybe_unused]] bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

[[maybe_unused]] bool vecNear(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return std::fabs(lhs.x - rhs.x) < 1.0e-8 &&
         std::fabs(lhs.y - rhs.y) < 1.0e-8 &&
         std::fabs(lhs.z - rhs.z) < 1.0e-8;
}

[[maybe_unused]] bool near(double lhs, double rhs) {
  return std::fabs(lhs - rhs) <= 1.0e-9;
}

class FakePlaytestProcessControl final : public app::PlaytestProcessControl {
 public:
  [[nodiscard]] bool running() override { return running_; }
  void stopRunning() override {
    ++stopCount;
    operations.emplace_back("stop");
    std::error_code error;
    snapshotPresentAtStop =
        !observedSnapshotPath.empty() &&
        std::filesystem::exists(observedSnapshotPath, error);
    running_ = false;
  }
  [[nodiscard]] bool launch(const app::PlaytestLaunchPlan& plan,
                            std::string& reasonCode) override {
    ++launchCount;
    operations.emplace_back("launch");
    lastPlan = plan;
    std::error_code error;
    snapshotPresentAtLaunch =
        !observedSnapshotPath.empty() &&
        std::filesystem::exists(observedSnapshotPath, error);
    if (!launchSucceeds) {
      reasonCode = launchReason;
      return false;
    }
    running_ = true;
    reasonCode = "fake_playtest_launched";
    return true;
  }
  void setSnapshotEntityNames(app::PlaytestEntityNameMap names) override {
    ++nameInstallCount;
    operations.emplace_back("names");
    snapshotEntityNames = std::move(names);
  }
  bool sendPlaytestCommand(
      std::string_view verb,
      const std::vector<std::pair<std::string, std::string>>&,
      std::string& reasonCode) override {
    ++sendCount;
    lastVerb = std::string(verb);
    reasonCode = sendReason;
    return sendSucceeds;
  }

  bool running_ = false;
  bool launchSucceeds = true;
  bool sendSucceeds = true;
  bool snapshotPresentAtStop = false;
  bool snapshotPresentAtLaunch = false;
  std::size_t stopCount = 0U;
  std::size_t launchCount = 0U;
  std::size_t nameInstallCount = 0U;
  std::size_t sendCount = 0U;
  std::filesystem::path observedSnapshotPath;
  app::PlaytestLaunchPlan lastPlan;
  app::PlaytestEntityNameMap snapshotEntityNames;
  std::vector<std::string> operations;
  std::string launchReason = "fake_playtest_launch_failed";
  std::string sendReason;
  std::string lastVerb;
};

[[maybe_unused]] bool rectEquals(cr::CreativeWorldLayoutRect lhs,
                                cr::CreativeWorldLayoutRect rhs) {
  return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum;
}

[[maybe_unused]] cr::CreativeObjectId createCrate(cr::Facade& facade,
                                                  double x) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.transform.position = {x, 0, 0};
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

[[maybe_unused]] cr::CreativeObjectId createMovingPlatform(
    cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::MovingPlatform;
  request.name = "Command Lift";
  request.transform.position = {1.0, 0.5, 2.0};
  request.hasTransformOverride = true;
  request.bounds = {{0.0, 0.25, 1.0}, {2.0, 0.75, 3.0}};
  request.hasBoundsOverride = true;
  request.pathPoints = {{{1.0, 0.5, 2.0}}, {{1.0, 3.5, 2.0}}};
  request.hasPathOverride = true;
  return facade.createDocumentObject(request).objectId;
}

[[maybe_unused]] cr::CreativeObjectId createPlayerSpawn(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::SpawnPoint;
  request.name = "Command Player Spawn";
  request.transform.position = {1.0, 0.0, 2.0};
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

[[maybe_unused]] cr::CreativeObjectId createNpcSpawn(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::NpcSpawn;
  request.name = "Command NPC Spawn";
  request.transform.position = {2.0, 0.0, 3.0};
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request).objectId;
}

struct AttachedPair {
  cr::CreativeObjectId parentId = cr::kInvalidObjectId;
  cr::CreativeObjectId childId = cr::kInvalidObjectId;
};

[[maybe_unused]] AttachedPair createAttachedPair(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest parent;
  parent.kind = cr::CreativeObjectKind::Prop;
  parent.name = "Attachment Parent";
  parent.transform.position = {2.0, 0.0, 3.0};
  parent.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt parentReceipt =
      facade.createDocumentObject(parent);

  cr::CreativeDocumentCreateRequest child;
  child.kind = cr::CreativeObjectKind::Door;
  child.name = "Attachment Child";
  child.transform.position = {2.5, 0.0, 3.0};
  child.hasTransformOverride = true;
  child.parentId = parentReceipt.objectId;
  child.attachmentSocket = "door_frame";
  const cr::CreativeDocumentCreateReceipt childReceipt =
      facade.createDocumentObject(child);
  return {parentReceipt.objectId, childReceipt.objectId};
}

[[maybe_unused]] void selectPrimary(cr::Facade& facade,
                                    cr::CreativeObjectId objectId) {
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  input.pointer.modifiers = cr::kCreativeToolModifierNone;
  static_cast<void>(facade.dispatchToolInput(input));
}

[[maybe_unused]] void completeDistanceMeasurement(cr::Facade& facade) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.x = 1.0;
  input.pointer.y = 2.0;
  static_cast<void>(facade.dispatchToolInput(input));
  input.pointer.x = 4.0;
  input.pointer.y = 6.0;
  static_cast<void>(facade.dispatchToolInput(input));
}

[[maybe_unused]] app::CreativeDesktopCommandResult dispatchOne(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id);
  return app::dispatchCreativeDesktopCommands(frame, context);
}

[[maybe_unused]] app::CreativeDesktopCommandResult dispatchOne(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context,
    std::string arg) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(arg));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

// Step 3: push a single typed-payload command and dispatch it.
[[maybe_unused]] app::CreativeDesktopCommandResult dispatchPayload(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context,
    app::CreativeDesktopCommandPayload payload) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(payload));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

[[maybe_unused]] const cr::CreativeObject* findGeneratedObject(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout,
    cr::CreativeWorldLayoutTable table,
    std::size_t index,
    cr::CreativeObjectKind kind) {
  for (const cr::CreativeObject& object : document.objects()) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(layout, object);
    if (provenance.owned && provenance.table == table &&
        provenance.index == index && object.kind == kind) {
      return &object;
    }
  }
  return nullptr;
}

[[maybe_unused]] std::vector<cr::CreativeObjectId> generatedSourceObjectIds(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout,
    cr::CreativeWorldLayoutTable table,
    std::size_t index) {
  std::vector<cr::CreativeObjectId> ids;
  for (const cr::CreativeObject& object : document.objects()) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(layout, object);
    if (provenance.owned && provenance.table == table &&
        provenance.index == index) {
      ids.push_back(object.id);
    }
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

[[maybe_unused]] std::vector<cr::CreativeObjectId> documentObjectIds(
    const cr::CreativeDocument& document) {
  std::vector<cr::CreativeObjectId> ids;
  ids.reserve(document.objects().size());
  for (const cr::CreativeObject& object : document.objects()) {
    ids.push_back(object.id);
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

[[maybe_unused]] cr::CreativeObjectId addSynchronizedGeneratedWorldObject(
    cr::CreativeAppState& appState,
    app::CreativeEditorWorldLayoutState& worldLayout,
    std::string_view layoutKey,
    cr::CreativeObjectKind kind = cr::CreativeObjectKind::Crate) {
  app::resetCreativeEditorWorldLayout(worldLayout, std::string(layoutKey));
  cr::CreativeWorldLayoutObject source;
  source.kind = kind;
  source.stableKey = "object_crate";
  source.name = "Source Crate";
  source.assetId = "crate";
  source.boundsCells = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  worldLayout.source.objects.push_back(source);
  worldLayout.generatedBaseline =
      app::captureCreativeEditorWorldLayoutSnapshot(worldLayout);
  worldLayout.sourceHistory.current.snapshot =
      app::captureCreativeEditorWorldLayoutSnapshot(worldLayout);

  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = "Compiled Crate";
  request.tags = {
      cr::creativeWorldLayoutTag(worldLayout.source.stableKey),
      cr::creativeWorldLayoutProvenanceTag(
          worldLayout.source, cr::CreativeWorldLayoutTable::Object, 0U)};
  if (kind == cr::CreativeObjectKind::MovingPlatform) {
    request.pathPoints = {{{0.5, 0.5, 0.5}}, {{0.5, 2.5, 0.5}}};
    request.hasPathOverride = true;
  }
  return appState.facade.createDocumentObject(request).objectId;
}

[[maybe_unused]] bool generatedBounds(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout,
    cr::CreativeWorldLayoutTable table,
    std::size_t index,
    cr::CreativeObjectKind kind,
    cr::CreativeBounds& output) {
  bool found = false;
  for (const cr::CreativeObject& object : document.objects()) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(layout, object);
    if (!provenance.owned || provenance.table != table ||
        provenance.index != index || object.kind != kind) {
      continue;
    }
    const cr::CreativeTransformedBounds bounds =
        cr::resolveCreativeObjectBounds(object);
    if (!bounds.valid) {
      continue;
    }
    if (!found) {
      output = bounds.worldBounds;
      found = true;
      continue;
    }
    output.min.x = std::min(output.min.x, bounds.worldBounds.min.x);
    output.min.y = std::min(output.min.y, bounds.worldBounds.min.y);
    output.min.z = std::min(output.min.z, bounds.worldBounds.min.z);
    output.max.x = std::max(output.max.x, bounds.worldBounds.max.x);
    output.max.y = std::max(output.max.y, bounds.worldBounds.max.y);
    output.max.z = std::max(output.max.z, bounds.worldBounds.max.z);
  }
  return found;
}

[[maybe_unused]] bool generatedRoomContributorBounds(
    const cr::CreativeDocument& document, const cr::CreativeWorldLayout& layout,
    std::size_t roomIndex, cr::CreativeObjectKind kind,
    cr::CreativeBounds& output) {
  if (roomIndex >= layout.rooms.size()) {
    return false;
  }
  std::array<std::string, 4U> edgeTags;
  for (std::size_t edgeIndex = 0U; edgeIndex < edgeTags.size(); ++edgeIndex) {
    edgeTags[edgeIndex] = cr::creativeWorldLayoutRoomEdgeProvenanceTag(
        layout, roomIndex,
        static_cast<cr::CreativeWorldLayoutRoomEdge>(edgeIndex));
  }

  bool found = false;
  for (const cr::CreativeObject& object : document.objects()) {
    const bool contributes =
        object.kind == kind &&
        std::any_of(edgeTags.begin(), edgeTags.end(),
                    [&](const std::string& tag) {
                      return std::find(object.tags.begin(), object.tags.end(),
                                       tag) != object.tags.end();
                    });
    if (!contributes) {
      continue;
    }
    const cr::CreativeTransformedBounds bounds =
        cr::resolveCreativeObjectBounds(object);
    if (!bounds.valid) {
      continue;
    }
    if (!found) {
      output = bounds.worldBounds;
      found = true;
      continue;
    }
    output.min.x = std::min(output.min.x, bounds.worldBounds.min.x);
    output.min.y = std::min(output.min.y, bounds.worldBounds.min.y);
    output.min.z = std::min(output.min.z, bounds.worldBounds.min.z);
    output.max.x = std::max(output.max.x, bounds.worldBounds.max.x);
    output.max.y = std::max(output.max.y, bounds.worldBounds.max.y);
    output.max.z = std::max(output.max.z, bounds.worldBounds.max.z);
  }
  return found;
}

}  // namespace
