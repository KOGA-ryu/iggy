#include "EditorDesktopModel.hpp"
#include "app/iggy3d/creative/document/Hierarchy.hpp"

#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <utility>

namespace iggy3d_creative_app {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegreesPerRadian = 180.0 / kPi;
constexpr double kRadiansPerDegree = kPi / 180.0;

[[nodiscard]] char asciiLower(char c) noexcept {
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// ASCII case-insensitive substring test. An empty needle matches everything.
[[nodiscard]] bool asciiContains(std::string_view haystack,
                                 std::string_view needle) noexcept {
  if (needle.empty()) {
    return true;
  }
  if (needle.size() > haystack.size()) {
    return false;
  }
  const std::size_t last = haystack.size() - needle.size();
  for (std::size_t start = 0U; start <= last; ++start) {
    std::size_t offset = 0U;
    while (offset < needle.size() &&
           asciiLower(haystack[start + offset]) == asciiLower(needle[offset])) {
      ++offset;
    }
    if (offset == needle.size()) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::unordered_map<cr::CreativeObjectId, std::size_t> indexById(
    std::span<const cr::CreativeObject> objects) {
  std::unordered_map<cr::CreativeObjectId, std::size_t> index;
  index.reserve(objects.size());
  for (std::size_t i = 0U; i < objects.size(); ++i) {
    index.emplace(objects[i].id, i);  // first occurrence wins; ids are unique.
  }
  return index;
}

[[nodiscard]] bool allFinite(cr::CreativeVec3 v) noexcept {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

[[nodiscard]] bool rowMatchesQuery(const CreativeDesktopOutlinerRow& row,
                                   std::string_view query) {
  char idText[24];
  std::snprintf(idText, sizeof(idText), "%llu",
                static_cast<unsigned long long>(row.objectId));
  return asciiContains(row.name, query) ||
         asciiContains(cr::toString(row.kind), query) ||
         asciiContains(idText, query);
}

void includeGeneratedScopeObject(
    CreativeDesktopGeneratedSourceScopeSummary& summary,
    const cr::CreativeObject& object,
    bool effectivelyVisible) noexcept {
  ++summary.objectCount;
  if (effectivelyVisible) {
    ++summary.visibleObjectCount;
  } else {
    ++summary.hiddenObjectCount;
  }
  const cr::CreativeTransformedBounds bounds =
      cr::resolveCreativeObjectBounds(object);
  if (!bounds.valid) {
    return;
  }
  if (!summary.hasBounds) {
    summary.worldBounds = bounds.worldBounds;
    summary.hasBounds = true;
    return;
  }
  summary.worldBounds.min.x =
      std::min(summary.worldBounds.min.x, bounds.worldBounds.min.x);
  summary.worldBounds.min.y =
      std::min(summary.worldBounds.min.y, bounds.worldBounds.min.y);
  summary.worldBounds.min.z =
      std::min(summary.worldBounds.min.z, bounds.worldBounds.min.z);
  summary.worldBounds.max.x =
      std::max(summary.worldBounds.max.x, bounds.worldBounds.max.x);
  summary.worldBounds.max.y =
      std::max(summary.worldBounds.max.y, bounds.worldBounds.max.y);
  summary.worldBounds.max.z =
      std::max(summary.worldBounds.max.z, bounds.worldBounds.max.z);
}

}  // namespace

CreativeDesktopOutlinerModel buildCreativeDesktopOutlinerModel(
    cr::CreativeDocumentId documentId,
    std::uint64_t documentRevision,
    std::span<const cr::CreativeObject> objects) {
  CreativeDesktopOutlinerModel model;
  model.documentId = documentId;
  model.documentRevision = documentRevision;
  const std::size_t count = objects.size();
  if (count == 0U) {
    return model;
  }

  // 1. objectId -> source index, once.
  const std::unordered_map<cr::CreativeObjectId, std::size_t> idIndex =
      indexById(objects);

  // 2. Child adjacency in source order, once. A root is an object with no
  //    parentId, or one whose parentId names an absent object (recovered).
  struct RootEntry {
    std::size_t index;
    CreativeDesktopHierarchyRecovery recovery;
  };
  std::vector<std::vector<std::size_t>> children(count);
  std::vector<RootEntry> roots;
  roots.reserve(count);
  for (std::size_t i = 0U; i < count; ++i) {
    const cr::CreativeObject& object = objects[i];
    if (!object.parentId.has_value()) {
      roots.push_back({i, CreativeDesktopHierarchyRecovery::None});
      continue;
    }
    const auto parent = idIndex.find(*object.parentId);
    if (parent == idIndex.end()) {
      roots.push_back({i, CreativeDesktopHierarchyRecovery::MissingParent});
      continue;
    }
    // A self-parent resolves here and so is never a root; it is recovered as a
    // cycle in step 5 instead.
    children[parent->second].push_back(i);
  }

  std::vector<bool> visited(count, false);
  model.rows.reserve(count);

  struct Frame {
    std::size_t index;
    std::uint32_t depth;
  };
  std::vector<Frame> stack;

  // 3/4. Iterative parent-before-child traversal; visit state guarantees each
  //      object is emitted at most once, so cycles terminate.
  const auto traverse = [&](std::size_t rootIndex,
                            CreativeDesktopHierarchyRecovery rootRecovery) {
    stack.clear();
    stack.push_back({rootIndex, 0U});
    while (!stack.empty()) {
      const Frame frame = stack.back();
      stack.pop_back();
      if (visited[frame.index]) {
        continue;
      }
      visited[frame.index] = true;
      const cr::CreativeObject& object = objects[frame.index];

      CreativeDesktopHierarchyRecovery recovery =
          CreativeDesktopHierarchyRecovery::None;
      if (frame.depth == 0U) {
        recovery = rootRecovery;
      }
      // 6. Clamp displayed depth; a deeper descendant stays a row and carries
      //    the depth warning rather than being dropped.
      if (frame.depth > kCreativeDesktopMaxOutlinerDepth) {
        recovery = CreativeDesktopHierarchyRecovery::DepthLimit;
      }

      CreativeDesktopOutlinerRow row;
      row.objectId = object.id;
      row.parentObjectId = object.parentId.has_value() ? *object.parentId
                                                       : cr::kInvalidObjectId;
      row.kind = object.kind;
      row.name = object.name;
      row.depth = std::min(frame.depth, kCreativeDesktopMaxOutlinerDepth);
      row.hasChildren = !children[frame.index].empty();
      row.visible = object.visible;
      row.locked = object.locked;
      const cr::CreativeObjectHierarchyState hierarchyState =
          cr::resolveCreativeObjectHierarchyState(objects, object.id);
      row.effectivelyVisible =
          hierarchyState.resolved && hierarchyState.effectivelyVisible;
      row.effectivelyLocked =
          !hierarchyState.resolved || hierarchyState.effectivelyLocked;
      row.recovery = recovery;
      if (recovery != CreativeDesktopHierarchyRecovery::None) {
        ++model.recoveredRowCount;
      }
      model.rows.push_back(std::move(row));

      // Push children reversed so they pop in source order.
      const std::vector<std::size_t>& kids = children[frame.index];
      for (std::size_t k = kids.size(); k-- > 0U;) {
        stack.push_back({kids[k], frame.depth + 1U});
      }
    }
  };

  for (const RootEntry& root : roots) {
    traverse(root.index, root.recovery);
  }
  // 5. Anything still unvisited belongs to a parent cycle (no valid root
  //    reaches it). Append each once as a recovered root, in source order.
  for (std::size_t i = 0U; i < count; ++i) {
    if (!visited[i]) {
      traverse(i, CreativeDesktopHierarchyRecovery::Cycle);
    }
  }
  return model;
}

CreativeDesktopOutlinerModel buildCreativeDesktopOutlinerModel(
    const cr::CreativeDocument& document) {
  return buildCreativeDesktopOutlinerModel(document.id(), document.revision(),
                                           document.objects());
}

std::vector<std::size_t> filterCreativeDesktopOutlinerRows(
    const CreativeDesktopOutlinerModel& model,
    std::string_view query) {
  std::vector<std::size_t> indices;
  const std::size_t rowCount = model.rows.size();
  if (query.empty()) {
    indices.resize(rowCount);
    for (std::size_t i = 0U; i < rowCount; ++i) {
      indices[i] = i;
    }
    return indices;
  }

  std::unordered_map<cr::CreativeObjectId, std::size_t> rowById;
  rowById.reserve(rowCount);
  for (std::size_t i = 0U; i < rowCount; ++i) {
    rowById.emplace(model.rows[i].objectId, i);
  }

  // A match keeps its whole ancestor path so the hit retains hierarchy context.
  std::vector<bool> keep(rowCount, false);
  for (std::size_t i = 0U; i < rowCount; ++i) {
    if (!rowMatchesQuery(model.rows[i], query)) {
      continue;
    }
    keep[i] = true;
    cr::CreativeObjectId parent = model.rows[i].parentObjectId;
    while (parent != cr::kInvalidObjectId) {
      const auto it = rowById.find(parent);
      if (it == rowById.end() || keep[it->second]) {
        break;  // absent parent, or already kept (also breaks parent cycles).
      }
      keep[it->second] = true;
      parent = model.rows[it->second].parentObjectId;
    }
  }

  indices.reserve(rowCount);
  for (std::size_t i = 0U; i < rowCount; ++i) {
    if (keep[i]) {
      indices.push_back(i);
    }
  }
  return indices;
}

CreativeDesktopSelectionGesture creativeDesktopSelectionGestureFor(
    bool rangeModifier,
    bool toggleModifier) noexcept {
  if (rangeModifier) {
    return CreativeDesktopSelectionGesture::VisibleRange;  // Shift wins.
  }
  if (toggleModifier) {
    return CreativeDesktopSelectionGesture::Toggle;
  }
  return CreativeDesktopSelectionGesture::Replace;
}

CreativeDesktopSelectionPlan planCreativeDesktopSelection(
    std::span<const cr::CreativeObjectId> visibleObjectIds,
    std::span<const cr::CreativeObjectId> currentSelectedIds,
    cr::CreativeObjectId primaryObjectId,
    cr::CreativeObjectId clickedObjectId,
    cr::CreativeObjectId anchorObjectId,
    CreativeDesktopSelectionGesture gesture) {
  CreativeDesktopSelectionPlan plan;
  if (clickedObjectId == cr::kInvalidObjectId) {
    return plan;
  }

  const auto visibleIndexOf = [&](cr::CreativeObjectId id) {
    return static_cast<std::size_t>(
        std::find(visibleObjectIds.begin(), visibleObjectIds.end(), id) -
        visibleObjectIds.begin());
  };

  const auto planReplace = [&]() {
    plan.objectIds = {clickedObjectId};
    plan.primaryObjectId = clickedObjectId;
    plan.nextAnchorObjectId = clickedObjectId;  // plain click moves the anchor.
    plan.accepted = true;
  };

  switch (gesture) {
    case CreativeDesktopSelectionGesture::Replace:
      planReplace();
      return plan;

    case CreativeDesktopSelectionGesture::Toggle: {
      plan.objectIds.assign(currentSelectedIds.begin(),
                            currentSelectedIds.end());
      const auto it = std::find(plan.objectIds.begin(), plan.objectIds.end(),
                                clickedObjectId);
      if (it != plan.objectIds.end()) {
        plan.objectIds.erase(it);
        // Removing the primary hands it to the last remaining id.
        plan.primaryObjectId =
            (primaryObjectId == clickedObjectId)
                ? (plan.objectIds.empty() ? cr::kInvalidObjectId
                                          : plan.objectIds.back())
                : primaryObjectId;
      } else {
        if (plan.objectIds.size() >= cr::kCreativeSelectionTargetCapacity) {
          return {};
        }
        plan.objectIds.push_back(clickedObjectId);
        plan.primaryObjectId = clickedObjectId;  // an added object is primary.
      }
      plan.nextAnchorObjectId = clickedObjectId;  // toggle moves the anchor.
      plan.accepted = true;
      return plan;
    }

    case CreativeDesktopSelectionGesture::VisibleRange: {
      const std::size_t anchorIndex = visibleIndexOf(anchorObjectId);
      const std::size_t clickedIndex = visibleIndexOf(clickedObjectId);
      if (anchorObjectId == cr::kInvalidObjectId ||
          anchorIndex == visibleObjectIds.size() ||
          clickedIndex == visibleObjectIds.size()) {
        planReplace();  // absent anchor -> plain selection (and moves anchor).
        return plan;
      }
      const std::size_t low = std::min(anchorIndex, clickedIndex);
      const std::size_t high = std::max(anchorIndex, clickedIndex);
      if (high - low + 1U > cr::kCreativeSelectionTargetCapacity) {
        return {};
      }
      plan.objectIds.reserve(high - low + 1U);
      for (std::size_t i = low; i <= high; ++i) {
        plan.objectIds.push_back(visibleObjectIds[i]);
      }
      plan.primaryObjectId = clickedObjectId;
      plan.nextAnchorObjectId = anchorObjectId;  // a range retains the anchor.
      plan.accepted = true;
      return plan;
    }
  }
  return plan;
}

CreativeDesktopSelectionPlan planCreativeDesktopHierarchySelection(
    const CreativeDesktopOutlinerModel& model,
    cr::CreativeObjectId objectId,
    CreativeDesktopHierarchySelectionScope scope) {
  CreativeDesktopSelectionPlan plan;
  if (objectId == cr::kInvalidObjectId || model.rows.empty()) {
    return plan;
  }
  const auto target = std::find_if(
      model.rows.begin(), model.rows.end(),
      [objectId](const CreativeDesktopOutlinerRow& row) {
        return row.objectId == objectId;
      });
  if (target == model.rows.end()) {
    return plan;
  }

  const auto accept = [&](cr::CreativeObjectId primary) {
    if (plan.objectIds.empty() ||
        plan.objectIds.size() > cr::kCreativeSelectionTargetCapacity) {
      plan = {};
      return;
    }
    plan.primaryObjectId = primary;
    plan.nextAnchorObjectId = primary;
    plan.accepted = true;
  };

  switch (scope) {
    case CreativeDesktopHierarchySelectionScope::Parent:
      if (target->parentObjectId == cr::kInvalidObjectId ||
          std::none_of(model.rows.begin(), model.rows.end(),
                       [&](const CreativeDesktopOutlinerRow& row) {
                         return row.objectId == target->parentObjectId;
                       })) {
        return plan;
      }
      plan.objectIds.push_back(target->parentObjectId);
      accept(target->parentObjectId);
      return plan;

    case CreativeDesktopHierarchySelectionScope::DirectChildren:
      for (const CreativeDesktopOutlinerRow& row : model.rows) {
        if (row.parentObjectId == objectId) {
          plan.objectIds.push_back(row.objectId);
          if (plan.objectIds.size() > cr::kCreativeSelectionTargetCapacity) {
            return {};
          }
        }
      }
      accept(plan.objectIds.empty() ? cr::kInvalidObjectId
                                    : plan.objectIds.front());
      return plan;

    case CreativeDesktopHierarchySelectionScope::Subtree: {
      std::unordered_map<cr::CreativeObjectId, cr::CreativeObjectId> parents;
      parents.reserve(model.rows.size());
      for (const CreativeDesktopOutlinerRow& row : model.rows) {
        parents.emplace(row.objectId, row.parentObjectId);
      }
      plan.objectIds.push_back(objectId);
      for (const CreativeDesktopOutlinerRow& row : model.rows) {
        if (row.objectId == objectId) {
          continue;
        }
        cr::CreativeObjectId cursor = row.parentObjectId;
        std::size_t hopCount = 0U;
        while (cursor != cr::kInvalidObjectId &&
               hopCount++ < model.rows.size()) {
          if (cursor == objectId) {
            plan.objectIds.push_back(row.objectId);
            break;
          }
          const auto parent = parents.find(cursor);
          cursor = parent == parents.end() ? cr::kInvalidObjectId
                                           : parent->second;
        }
        if (plan.objectIds.size() > cr::kCreativeSelectionTargetCapacity) {
          return {};
        }
      }
      accept(objectId);
      return plan;
    }
  }
  return plan;
}

CreativeDesktopSelectionResolution resolveCreativeDesktopSelection(
    std::span<const cr::CreativeObject> objects,
    std::span<const cr::CreativeObjectId> selectedObjectIds,
    cr::CreativeObjectId primaryObjectId) {
  CreativeDesktopSelectionResolution resolution;
  if (selectedObjectIds.empty()) {
    return resolution;
  }
  const std::unordered_map<cr::CreativeObjectId, std::size_t> idIndex =
      indexById(objects);
  resolution.objectIds.reserve(selectedObjectIds.size());
  for (const cr::CreativeObjectId id : selectedObjectIds) {
    if (idIndex.find(id) != idIndex.end()) {
      resolution.objectIds.push_back(id);  // stale ids are discarded.
    }
  }
  if (resolution.objectIds.empty()) {
    return resolution;
  }
  const bool primaryValid =
      primaryObjectId != cr::kInvalidObjectId &&
      std::find(resolution.objectIds.begin(), resolution.objectIds.end(),
                primaryObjectId) != resolution.objectIds.end();
  resolution.primaryObjectId =
      primaryValid ? primaryObjectId : resolution.objectIds.front();
  return resolution;
}

CreativeDesktopSelectionResolution resolveCreativeDesktopSelection(
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> selectedObjectIds,
    cr::CreativeObjectId primaryObjectId) {
  return resolveCreativeDesktopSelection(document.objects(), selectedObjectIds,
                                         primaryObjectId);
}

cr::CreativeVec3 creativeDesktopRadiansToDegrees(
    cr::CreativeVec3 radians) noexcept {
  return {radians.x * kDegreesPerRadian, radians.y * kDegreesPerRadian,
          radians.z * kDegreesPerRadian};
}

cr::CreativeVec3 creativeDesktopDegreesToRadians(
    cr::CreativeVec3 degrees) noexcept {
  return {degrees.x * kRadiansPerDegree, degrees.y * kRadiansPerDegree,
          degrees.z * kRadiansPerDegree};
}

CreativeDesktopTransformDraft validateCreativeDesktopTransformDraft(
    cr::CreativeVec3 position,
    cr::CreativeVec3 rotationDegrees,
    cr::CreativeVec3 scale) noexcept {
  CreativeDesktopTransformDraft draft;
  if (!allFinite(position) || !allFinite(rotationDegrees) ||
      !allFinite(scale)) {
    draft.message = "value must be a finite number";
    return draft;
  }
  if (scale.x <= 0.0 || scale.y <= 0.0 || scale.z <= 0.0) {
    draft.message = "scale must be greater than zero";
    return draft;
  }
  draft.transform.position = position;
  draft.transform.rotationEulerRadians =
      creativeDesktopDegreesToRadians(rotationDegrees);
  draft.transform.scale = scale;
  draft.valid = true;
  return draft;
}

bool creativeDesktopGeneratedSourceSupportsAdoption(
    const cr::CreativeWorldLayoutObjectProvenance& provenance) noexcept {
  return provenance.owned && provenance.contributorCount == 1U &&
         (provenance.table == cr::CreativeWorldLayoutTable::Object ||
          provenance.table == cr::CreativeWorldLayoutTable::Box);
}

CreativeDesktopGeneratedSourceScopeModel
buildCreativeDesktopGeneratedSourceScopeModel(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutObjectProvenance& provenance) noexcept {
  CreativeDesktopGeneratedSourceScopeModel model;
  const cr::CreativeWorldLayoutSourceAncestry ancestry =
      cr::buildCreativeWorldLayoutSourceAncestry(layout, provenance);
  for (std::size_t sourceIndex = 0U; sourceIndex < ancestry.count;
       ++sourceIndex) {
    const cr::CreativeWorldLayoutSourceRef source =
        ancestry.entries[sourceIndex];
    CreativeDesktopGeneratedSourceScopeEntry entry;
    entry.table = source.table;
    entry.index = source.index;
    switch (source.table) {
      case cr::CreativeWorldLayoutTable::Building:
        if (source.index >= layout.buildings.size()) continue;
        entry.stableKey = layout.buildings[source.index].stableKey;
        entry.name = layout.buildings[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::Level:
        if (source.index >= layout.levels.size()) continue;
        entry.stableKey = layout.levels[source.index].stableKey;
        entry.name = layout.levels[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::Room:
        if (source.index >= layout.rooms.size()) continue;
        entry.stableKey = layout.rooms[source.index].stableKey;
        entry.name = layout.rooms[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::TopologyEdge:
        if (source.index >= layout.topologyEdges.size()) continue;
        entry.stableKey = layout.topologyEdges[source.index].stableKey;
        entry.name = "Wall";
        break;
      case cr::CreativeWorldLayoutTable::VerticalConnector:
        if (source.index >= layout.verticalConnectors.size()) continue;
        entry.stableKey = layout.verticalConnectors[source.index].stableKey;
        entry.name = layout.verticalConnectors[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::Box:
        if (source.index >= layout.boxes.size()) continue;
        entry.stableKey = layout.boxes[source.index].stableKey;
        entry.name = layout.boxes[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::Wall:
        if (source.index >= layout.walls.size()) continue;
        entry.stableKey = layout.walls[source.index].stableKey;
        entry.name = layout.walls[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::Opening:
        if (source.index >= layout.openings.size()) continue;
        entry.stableKey = layout.openings[source.index].stableKey;
        entry.name = layout.openings[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::RoofAperture:
        if (source.index >= layout.roofApertures.size()) continue;
        entry.stableKey = layout.roofApertures[source.index].stableKey;
        entry.name = layout.roofApertures[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::Object:
        if (source.index >= layout.objects.size()) continue;
        entry.stableKey = layout.objects[source.index].stableKey;
        entry.name = layout.objects[source.index].name;
        break;
      case cr::CreativeWorldLayoutTable::TerrainProfile:
        if (source.index >= layout.terrainProfiles.size()) continue;
        entry.stableKey = layout.terrainProfiles[source.index].stableKey;
        entry.name = layout.terrainProfiles[source.index].stableKey;
        break;
      case cr::CreativeWorldLayoutTable::TerrainPath:
        if (source.index >= layout.terrainPaths.size()) continue;
        entry.stableKey = layout.terrainPaths[source.index].stableKey;
        entry.name = layout.terrainPaths[source.index].stableKey;
        break;
      case cr::CreativeWorldLayoutTable::None:
      case cr::CreativeWorldLayoutTable::TerrainPathPoint:
        continue;
    }
    model.entries[model.count++] = entry;
  }
  model.directEntryIndex =
      ancestry.directEntryIndex < model.count ? ancestry.directEntryIndex : 0U;
  return model;
}

std::size_t findCreativeDesktopGeneratedSourceScope(
    const CreativeDesktopGeneratedSourceScopeModel& model,
    cr::CreativeWorldLayoutTable table,
    std::size_t index) noexcept {
  for (std::size_t scopeIndex = 0U; scopeIndex < model.count; ++scopeIndex) {
    const CreativeDesktopGeneratedSourceScopeEntry& entry =
        model.entries[scopeIndex];
    if (entry.table == table && entry.index == index) {
      return scopeIndex;
    }
  }
  return model.count;
}

std::size_t resolveCreativeDesktopGeneratedSourceActiveScope(
    const CreativeDesktopGeneratedSourceScopeModel& model,
    cr::CreativeWorldLayoutTable selectedTable,
    std::size_t selectedIndex) noexcept {
  const std::size_t selected = findCreativeDesktopGeneratedSourceScope(
      model, selectedTable, selectedIndex);
  return selected < model.count ? selected : model.directEntryIndex;
}

bool creativeDesktopGeneratedObjectBelongsToSourceScope(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeObject& object,
    cr::CreativeWorldLayoutTable table,
    std::size_t index) {
  return cr::creativeWorldLayoutObjectBelongsToSource(layout, object, table,
                                                      index);
}

CreativeDesktopGeneratedSourceScopeSummary
buildCreativeDesktopGeneratedSourceScopeSummary(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout,
    cr::CreativeWorldLayoutTable table,
    std::size_t index) {
  CreativeDesktopGeneratedSourceScopeSummary summary;
  summary.table = table;
  summary.index = index;
  for (const cr::CreativeObject& object : document.objects()) {
    if (!creativeDesktopGeneratedObjectBelongsToSourceScope(
            layout, object, table, index)) {
      continue;
    }
    const cr::CreativeObjectHierarchyState hierarchyState =
        cr::resolveCreativeObjectHierarchyState(document, object.id);
    includeGeneratedScopeObject(
        summary, object,
        hierarchyState.resolved && hierarchyState.effectivelyVisible);
  }
  summary.valid = summary.objectCount > 0U;
  return summary;
}

bool refreshCreativeDesktopGeneratedSourceScopeCache(
    CreativeDesktopGeneratedSourceScopeCache& cache,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout,
    std::uint64_t sourceEpoch,
    std::uint64_t sourceRevision,
    std::uint64_t generatedRevision,
    cr::CreativeWorldLayoutTable table,
    std::size_t index) {
  if (cache.populated && cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.sourceEpoch == sourceEpoch &&
      cache.sourceRevision == sourceRevision &&
      cache.generatedRevision == generatedRevision && cache.table == table &&
      cache.index == index) {
    return false;
  }

  cache.populated = true;
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.sourceEpoch = sourceEpoch;
  cache.sourceRevision = sourceRevision;
  cache.generatedRevision = generatedRevision;
  cache.table = table;
  cache.index = index;
  cache.summary = {};
  cache.summary.table = table;
  cache.summary.index = index;
  cache.objectIds.clear();
  cache.objectIds.reserve(document.objects().size());
  for (const cr::CreativeObject& object : document.objects()) {
    if (!creativeDesktopGeneratedObjectBelongsToSourceScope(
            layout, object, table, index)) {
      continue;
    }
    cache.objectIds.push_back(object.id);
    const cr::CreativeObjectHierarchyState hierarchyState =
        cr::resolveCreativeObjectHierarchyState(document, object.id);
    includeGeneratedScopeObject(
        cache.summary, object,
        hierarchyState.resolved && hierarchyState.effectivelyVisible);
  }
  std::sort(cache.objectIds.begin(), cache.objectIds.end());
  cache.summary.valid = cache.summary.objectCount > 0U;
  return true;
}

bool creativeDesktopGeneratedSourceScopeCacheContains(
    const CreativeDesktopGeneratedSourceScopeCache& cache,
    cr::CreativeObjectId objectId) noexcept {
  return cache.populated &&
         std::binary_search(cache.objectIds.begin(), cache.objectIds.end(),
                            objectId);
}

CreativeDesktopGeneratedSourceScopeTint
creativeDesktopGeneratedSourceScopeTint(
    cr::CreativeWorldLayoutTable table) noexcept {
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
      return {0.18F, 0.82F, 1.0F, 1.0F};
    case cr::CreativeWorldLayoutTable::Level:
      return {0.24F, 0.90F, 0.48F, 1.0F};
    case cr::CreativeWorldLayoutTable::Room:
      return {1.0F, 0.58F, 0.18F, 1.0F};
    case cr::CreativeWorldLayoutTable::TopologyEdge:
      return {1.0F, 0.78F, 0.24F, 1.0F};
    case cr::CreativeWorldLayoutTable::RoofAperture:
      return {0.35F, 0.76F, 0.94F, 1.0F};
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::VerticalConnector:
    case cr::CreativeWorldLayoutTable::Box:
    case cr::CreativeWorldLayoutTable::Wall:
    case cr::CreativeWorldLayoutTable::Opening:
    case cr::CreativeWorldLayoutTable::Object:
    case cr::CreativeWorldLayoutTable::TerrainProfile:
    case cr::CreativeWorldLayoutTable::TerrainPath:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return {};
  }
  return {};
}

}  // namespace iggy3d_creative_app
