#include "EditorDesktopHistoryModel.hpp"
#include "EditorDesktopModel.hpp"
#include "EditorWorldLayoutState.hpp"

#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// UI-4A pure desktop model (docs/creative_desktop_ui_work_order.md). The
// hierarchy projection, search, selection planning, selection resolution, and
// Inspector transform drafts are pinned here with no ImGui and no live
// document — malformed parent graphs are fed directly so recovery is provable.

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double a, double b, double tol = 1.0e-9) {
  const double d = a - b;
  return (d < 0.0 ? -d : d) <= tol;
}

bool propertyEditIntentSeparatesPreviewCommitAndCancel() {
  app::CreativeDesktopPropertyEditActivity continuous;
  app::observeCreativeDesktopContinuousPropertyEdit(continuous, true, false);
  const app::CreativeDesktopPropertyEditIntent preview =
      app::resolveCreativeDesktopPropertyEditIntent(
          continuous, true, true, false);

  app::CreativeDesktopPropertyEditActivity released;
  app::observeCreativeDesktopContinuousPropertyEdit(released, false, true);
  const app::CreativeDesktopPropertyEditIntent commit =
      app::resolveCreativeDesktopPropertyEditIntent(
          released, true, true, true);

  app::CreativeDesktopPropertyEditActivity discrete;
  app::observeCreativeDesktopDiscretePropertyEdit(discrete, true);
  const app::CreativeDesktopPropertyEditIntent immediateCommit =
      app::resolveCreativeDesktopPropertyEditIntent(
          discrete, true, true, false);
  const app::CreativeDesktopPropertyEditIntent invalid =
      app::resolveCreativeDesktopPropertyEditIntent(
          continuous, true, false, true);
  const app::CreativeDesktopPropertyEditIntent reset =
      app::resolveCreativeDesktopPropertyEditIntent(
          {}, false, true, true, true);
  const app::CreativeDesktopPropertyEditIntent idle =
      app::resolveCreativeDesktopPropertyEditIntent(
          {}, true, true, true);

  return expect(
             preview == app::CreativeDesktopPropertyEditIntent::Preview,
             "an active continuous edit requests an exact preview") &&
         expect(commit == app::CreativeDesktopPropertyEditIntent::Commit,
                "deactivation commits one dirty valid draft") &&
         expect(immediateCommit ==
                    app::CreativeDesktopPropertyEditIntent::Commit,
                "a discrete property choice commits immediately") &&
         expect(invalid == app::CreativeDesktopPropertyEditIntent::Cancel &&
                    reset == app::CreativeDesktopPropertyEditIntent::Cancel,
                "invalid and reset drafts clear an owned preview") &&
         expect(idle == app::CreativeDesktopPropertyEditIntent::None,
                "idle frames do not replay property work");
}

cr::CreativeObject makeObject(cr::CreativeObjectId id,
                              std::optional<cr::CreativeObjectId> parent,
                              cr::CreativeObjectKind kind, std::string name) {
  cr::CreativeObject object;
  object.id = id;
  object.parentId = parent;
  object.kind = kind;
  object.name = std::move(name);
  return object;
}

std::vector<cr::CreativeObjectId> rowIds(
    const app::CreativeDesktopOutlinerModel& model) {
  std::vector<cr::CreativeObjectId> ids;
  ids.reserve(model.rows.size());
  for (const app::CreativeDesktopOutlinerRow& row : model.rows) {
    ids.push_back(row.objectId);
  }
  return ids;
}

// 1. Empty document.
bool emptyDocumentProjectsNoRows() {
  const app::CreativeDesktopOutlinerModel model =
      app::buildCreativeDesktopOutlinerModel(9U, 2U, {});
  return expect(model.rows.empty() && model.recoveredRowCount == 0U,
                "an empty document projects no rows") &&
         expect(model.documentId == 9U && model.documentRevision == 2U,
                "the model carries the document id and revision cache key");
}

// 2. Parent-before-child flattening, sibling source order preserved.
bool flattenKeepsParentBeforeChildAndSourceOrder() {
  const std::vector<cr::CreativeObject> objects{
      makeObject(10U, std::nullopt, cr::CreativeObjectKind::Crate, "A"),
      makeObject(11U, 10U, cr::CreativeObjectKind::Crate, "A1"),
      makeObject(12U, 10U, cr::CreativeObjectKind::Crate, "A2"),
      makeObject(13U, std::nullopt, cr::CreativeObjectKind::Crate, "B"),
      makeObject(14U, 13U, cr::CreativeObjectKind::Crate, "B1")};
  const app::CreativeDesktopOutlinerModel model =
      app::buildCreativeDesktopOutlinerModel(1U, 1U, objects);
  const std::vector<cr::CreativeObjectId> ids = rowIds(model);
  const std::vector<cr::CreativeObjectId> expected{10U, 11U, 12U, 13U, 14U};
  const auto& rows = model.rows;
  return expect(ids == expected,
                "roots and siblings flatten in source order, children after "
                "their parent") &&
         expect(rows[0].depth == 0U && rows[1].depth == 1U &&
                    rows[2].depth == 1U && rows[3].depth == 0U &&
                    rows[4].depth == 1U,
                "child depth is parent depth + 1") &&
         expect(rows[0].hasChildren && !rows[1].hasChildren &&
                    rows[3].hasChildren,
                "hasChildren reflects the adjacency") &&
         expect(rows[1].parentObjectId == 10U &&
                    rows[0].parentObjectId == cr::kInvalidObjectId,
                "parentObjectId is kInvalidObjectId for a true root") &&
         expect(model.recoveredRowCount == 0U,
                "a well-formed hierarchy recovers nothing");
}

// 3. Missing-parent recovery.
bool missingParentBecomesRecoveredRoot() {
  const std::vector<cr::CreativeObject> objects{
      makeObject(1U, std::nullopt, cr::CreativeObjectKind::Crate, "A"),
      makeObject(2U, 999U, cr::CreativeObjectKind::Crate, "Orphan")};
  const app::CreativeDesktopOutlinerModel model =
      app::buildCreativeDesktopOutlinerModel(1U, 1U, objects);
  return expect(model.rows.size() == 2U, "every object is projected") &&
         expect(model.rows[1].objectId == 2U && model.rows[1].depth == 0U &&
                    model.rows[1].recovery ==
                        app::CreativeDesktopHierarchyRecovery::MissingParent,
                "a missing parent yields a recovered root") &&
         expect(model.recoveredRowCount == 1U, "recovered rows are counted");
}

// 4. Self-cycle and multi-object cycle: every object emitted exactly once.
bool cyclesRecoverAndEmitEachObjectOnce() {
  const std::vector<cr::CreativeObject> selfCycle{
      makeObject(1U, 1U, cr::CreativeObjectKind::Crate, "Self")};
  const app::CreativeDesktopOutlinerModel selfModel =
      app::buildCreativeDesktopOutlinerModel(1U, 1U, selfCycle);

  // A -> B -> A, plus an unrelated valid root that must stay ahead of them.
  const std::vector<cr::CreativeObject> pairCycle{
      makeObject(5U, std::nullopt, cr::CreativeObjectKind::Crate, "Root"),
      makeObject(6U, 7U, cr::CreativeObjectKind::Crate, "A"),
      makeObject(7U, 6U, cr::CreativeObjectKind::Crate, "B")};
  const app::CreativeDesktopOutlinerModel pairModel =
      app::buildCreativeDesktopOutlinerModel(1U, 1U, pairCycle);
  const std::vector<cr::CreativeObjectId> ids = rowIds(pairModel);

  return expect(selfModel.rows.size() == 1U &&
                    selfModel.rows[0].recovery ==
                        app::CreativeDesktopHierarchyRecovery::Cycle,
                "a self-cycle is emitted once as a recovered root") &&
         expect(pairModel.rows.size() == 3U,
                "a two-object cycle emits each object exactly once") &&
         expect(ids[0] == 5U && ids[1] == 6U && ids[2] == 7U,
                "valid roots come first; the cycle is appended in source "
                "order") &&
         expect(pairModel.rows[1].recovery ==
                    app::CreativeDesktopHierarchyRecovery::Cycle,
                "the cycle entry point is the recovered root") &&
         expect(pairModel.recoveredRowCount == 1U,
                "only the cycle's entry row is counted as recovered");
}

// 5. Depth-limit recovery: deeper rows survive, clamped and marked.
bool depthLimitClampsRatherThanDropping() {
  const std::uint32_t chain = app::kCreativeDesktopMaxOutlinerDepth + 3U;
  std::vector<cr::CreativeObject> objects;
  objects.push_back(
      makeObject(1U, std::nullopt, cr::CreativeObjectKind::Crate, "root"));
  for (std::uint32_t i = 1U; i < chain; ++i) {
    objects.push_back(
        makeObject(i + 1U, i, cr::CreativeObjectKind::Crate, "link"));
  }
  const app::CreativeDesktopOutlinerModel model =
      app::buildCreativeDesktopOutlinerModel(1U, 1U, objects);
  const app::CreativeDesktopOutlinerRow& deepest = model.rows.back();
  return expect(model.rows.size() == objects.size(),
                "a chain deeper than the cap still projects every object") &&
         expect(deepest.depth == app::kCreativeDesktopMaxOutlinerDepth,
                "displayed depth clamps at the cap") &&
         expect(deepest.recovery ==
                    app::CreativeDesktopHierarchyRecovery::DepthLimit,
                "a clamped row carries the depth warning") &&
         expect(model.recoveredRowCount == 2U,
                "only rows past the cap are counted as depth-recovered");
}

// 6. ASCII case-insensitive name/kind/id filtering.
bool searchMatchesNameKindAndId() {
  const std::vector<cr::CreativeObject> objects{
      makeObject(42U, std::nullopt, cr::CreativeObjectKind::Crate, "Pillar"),
      makeObject(43U, std::nullopt, cr::CreativeObjectKind::Wall, "Barrier")};
  const app::CreativeDesktopOutlinerModel model =
      app::buildCreativeDesktopOutlinerModel(1U, 1U, objects);
  const auto all = app::filterCreativeDesktopOutlinerRows(model, "");
  const auto byName = app::filterCreativeDesktopOutlinerRows(model, "PILL");
  const auto byKind = app::filterCreativeDesktopOutlinerRows(model, "wall");
  const auto byId = app::filterCreativeDesktopOutlinerRows(model, "42");
  const auto none = app::filterCreativeDesktopOutlinerRows(model, "zzz");
  return expect(all.size() == 2U, "an empty query shows every object") &&
         expect(byName.size() == 1U && model.rows[byName[0]].objectId == 42U,
                "name search is ASCII case-insensitive") &&
         expect(byKind.size() == 1U && model.rows[byKind[0]].objectId == 43U,
                "object-kind label search matches") &&
         expect(byId.size() == 1U && model.rows[byId[0]].objectId == 42U,
                "decimal object-id search matches") &&
         expect(none.empty(), "a non-matching query returns no rows");
}

// 7. Matching descendants retain their ancestor path.
bool searchRetainsAncestorPath() {
  const std::vector<cr::CreativeObject> objects{
      makeObject(1U, std::nullopt, cr::CreativeObjectKind::Crate, "grand"),
      makeObject(2U, 1U, cr::CreativeObjectKind::Crate, "parent"),
      makeObject(3U, 2U, cr::CreativeObjectKind::Crate, "needle"),
      makeObject(4U, std::nullopt, cr::CreativeObjectKind::Crate, "unrelated")};
  const app::CreativeDesktopOutlinerModel model =
      app::buildCreativeDesktopOutlinerModel(1U, 1U, objects);
  const auto filtered = app::filterCreativeDesktopOutlinerRows(model, "needle");
  std::vector<cr::CreativeObjectId> ids;
  for (const std::size_t index : filtered) {
    ids.push_back(model.rows[index].objectId);
  }
  const std::vector<cr::CreativeObjectId> expected{1U, 2U, 3U};
  return expect(ids == expected,
                "a matching descendant keeps its ancestors, in row order, and "
                "drops unrelated roots");
}

// 8. Plain selection replaces and sets the primary.
bool plainSelectionReplaces() {
  const std::vector<cr::CreativeObjectId> visible{10U, 11U, 12U};
  const std::vector<cr::CreativeObjectId> current{10U, 11U};
  const app::CreativeDesktopSelectionPlan plan =
      app::planCreativeDesktopSelection(
          visible, current, 10U, 12U, 10U,
          app::CreativeDesktopSelectionGesture::Replace);
  return expect(plan.accepted && plan.objectIds.size() == 1U &&
                    plan.objectIds[0] == 12U && plan.primaryObjectId == 12U,
                "plain click replaces the selection and becomes primary") &&
         expect(plan.nextAnchorObjectId == 12U,
                "a plain click updates the anchor");
}

// 9. Toggle add/remove and primary behavior.
bool toggleAddsAndRemoves() {
  const std::vector<cr::CreativeObjectId> visible{10U, 11U, 12U};
  const app::CreativeDesktopSelectionPlan added =
      app::planCreativeDesktopSelection(
          visible, std::vector<cr::CreativeObjectId>{10U}, 10U, 11U, 10U,
          app::CreativeDesktopSelectionGesture::Toggle);
  const app::CreativeDesktopSelectionPlan removed =
      app::planCreativeDesktopSelection(
          visible, std::vector<cr::CreativeObjectId>{10U, 11U}, 11U, 11U, 11U,
          app::CreativeDesktopSelectionGesture::Toggle);
  return expect(added.objectIds.size() == 2U && added.objectIds[1] == 11U &&
                    added.primaryObjectId == 11U &&
                    added.nextAnchorObjectId == 11U,
                "toggle adds the clicked object and makes it primary") &&
         expect(removed.objectIds.size() == 1U &&
                    removed.objectIds[0] == 10U &&
                    removed.primaryObjectId == 10U,
                "toggling off the primary re-homes the primary");
}

// 10. Shift range selection works in both directions.
bool shiftRangeSelectsBothDirections() {
  const std::vector<cr::CreativeObjectId> visible{10U, 11U, 12U, 13U};
  const app::CreativeDesktopSelectionPlan forward =
      app::planCreativeDesktopSelection(
          visible, std::vector<cr::CreativeObjectId>{10U}, 10U, 12U, 10U,
          app::CreativeDesktopSelectionGesture::VisibleRange);
  const app::CreativeDesktopSelectionPlan backward =
      app::planCreativeDesktopSelection(
          visible, std::vector<cr::CreativeObjectId>{13U}, 13U, 11U, 13U,
          app::CreativeDesktopSelectionGesture::VisibleRange);
  const std::vector<cr::CreativeObjectId> forwardExpected{10U, 11U, 12U};
  const std::vector<cr::CreativeObjectId> backwardExpected{11U, 12U, 13U};
  return expect(forward.objectIds == forwardExpected &&
                    forward.primaryObjectId == 12U,
                "a downward range spans anchor..clicked inclusively") &&
         expect(backward.objectIds == backwardExpected &&
                    backward.primaryObjectId == 11U,
                "an upward range spans clicked..anchor inclusively") &&
         expect(forward.nextAnchorObjectId == 10U &&
                    backward.nextAnchorObjectId == 13U,
                "a range retains the stored anchor");
}

// 11. Shift takes precedence over toggle.
bool shiftWinsOverToggle() {
  return expect(app::creativeDesktopSelectionGestureFor(true, true) ==
                    app::CreativeDesktopSelectionGesture::VisibleRange,
                "range wins when both modifiers are down") &&
         expect(app::creativeDesktopSelectionGestureFor(false, true) ==
                    app::CreativeDesktopSelectionGesture::Toggle,
                "the toggle modifier alone toggles") &&
         expect(app::creativeDesktopSelectionGestureFor(false, false) ==
                    app::CreativeDesktopSelectionGesture::Replace,
                "no modifier is a plain replace");
}

// 12. A range whose anchor is not in the filtered rows falls back to plain.
bool missingAnchorFallsBackToPlain() {
  const std::vector<cr::CreativeObjectId> visible{10U, 11U, 12U};
  // Anchor 99 was filtered out of the visible rows.
  const app::CreativeDesktopSelectionPlan filteredOut =
      app::planCreativeDesktopSelection(
          visible, std::vector<cr::CreativeObjectId>{10U}, 10U, 12U, 99U,
          app::CreativeDesktopSelectionGesture::VisibleRange);
  const app::CreativeDesktopSelectionPlan noAnchor =
      app::planCreativeDesktopSelection(
          visible, std::vector<cr::CreativeObjectId>{}, cr::kInvalidObjectId,
          12U, cr::kInvalidObjectId,
          app::CreativeDesktopSelectionGesture::VisibleRange);
  return expect(filteredOut.objectIds.size() == 1U &&
                    filteredOut.objectIds[0] == 12U &&
                    filteredOut.nextAnchorObjectId == 12U,
                "an anchor absent from the filtered rows falls back to plain") &&
         expect(noAnchor.objectIds.size() == 1U && noAnchor.objectIds[0] == 12U,
                "no anchor at all falls back to plain");
}

bool hierarchySelectionPlansParentChildrenAndSubtree() {
  const std::vector<cr::CreativeObject> objects{
      makeObject(1U, std::nullopt, cr::CreativeObjectKind::Group, "root"),
      makeObject(2U, 1U, cr::CreativeObjectKind::Group, "branch"),
      makeObject(3U, 2U, cr::CreativeObjectKind::Crate, "leaf"),
      makeObject(4U, 1U, cr::CreativeObjectKind::Crate, "sibling")};
  const app::CreativeDesktopOutlinerModel model =
      app::buildCreativeDesktopOutlinerModel(1U, 1U, objects);
  const app::CreativeDesktopSelectionPlan parent =
      app::planCreativeDesktopHierarchySelection(
          model, 3U,
          app::CreativeDesktopHierarchySelectionScope::Parent);
  const app::CreativeDesktopSelectionPlan children =
      app::planCreativeDesktopHierarchySelection(
          model, 1U,
          app::CreativeDesktopHierarchySelectionScope::DirectChildren);
  const app::CreativeDesktopSelectionPlan subtree =
      app::planCreativeDesktopHierarchySelection(
          model, 1U,
          app::CreativeDesktopHierarchySelectionScope::Subtree);
  const app::CreativeDesktopSelectionPlan leafChildren =
      app::planCreativeDesktopHierarchySelection(
          model, 3U,
          app::CreativeDesktopHierarchySelectionScope::DirectChildren);

  return expect(parent.accepted && parent.objectIds ==
                                        std::vector<cr::CreativeObjectId>{2U} &&
                    parent.primaryObjectId == 2U,
                "parent selection resolves one immediate parent") &&
         expect(children.accepted &&
                    children.objectIds ==
                        std::vector<cr::CreativeObjectId>{2U, 4U} &&
                    children.primaryObjectId == 2U,
                "child selection preserves deterministic sibling order") &&
         expect(subtree.accepted &&
                    subtree.objectIds ==
                        std::vector<cr::CreativeObjectId>{1U, 2U, 3U, 4U} &&
                    subtree.primaryObjectId == 1U,
                "hierarchy selection includes root and every descendant") &&
         expect(!leafChildren.accepted && leafChildren.objectIds.empty(),
                "a leaf invents no child selection");
}

bool selectionPlannersRejectOverCapacityAtomically() {
  std::vector<cr::CreativeObjectId> ids;
  ids.reserve(cr::kCreativeSelectionTargetCapacity + 1U);
  for (std::size_t index = 0U;
       index <= cr::kCreativeSelectionTargetCapacity; ++index) {
    ids.push_back(static_cast<cr::CreativeObjectId>(index + 1U));
  }
  const app::CreativeDesktopSelectionPlan range =
      app::planCreativeDesktopSelection(
          ids, {}, cr::kInvalidObjectId, ids.back(), ids.front(),
          app::CreativeDesktopSelectionGesture::VisibleRange);
  const app::CreativeDesktopSelectionPlan toggle =
      app::planCreativeDesktopSelection(
          {}, std::span<const cr::CreativeObjectId>{
                  ids.data(), cr::kCreativeSelectionTargetCapacity},
          ids.front(), ids.back(), ids.front(),
          app::CreativeDesktopSelectionGesture::Toggle);

  app::CreativeDesktopOutlinerModel model;
  model.rows.reserve(ids.size());
  app::CreativeDesktopOutlinerRow root;
  root.objectId = ids.front();
  root.parentObjectId = cr::kInvalidObjectId;
  model.rows.push_back(root);
  for (std::size_t index = 1U; index < ids.size(); ++index) {
    app::CreativeDesktopOutlinerRow child;
    child.objectId = ids[index];
    child.parentObjectId = ids.front();
    model.rows.push_back(child);
  }
  const app::CreativeDesktopSelectionPlan subtree =
      app::planCreativeDesktopHierarchySelection(
          model, ids.front(),
          app::CreativeDesktopHierarchySelectionScope::Subtree);
  const app::CreativeDesktopSelectionPlan children =
      app::planCreativeDesktopHierarchySelection(
          model, ids.front(),
          app::CreativeDesktopHierarchySelectionScope::DirectChildren);

  return expect(!range.accepted && range.objectIds.empty(),
                "visible range rejects an over-capacity span") &&
         expect(!toggle.accepted && toggle.objectIds.empty(),
                "toggle rejects an addition at capacity") &&
         expect(!subtree.accepted && subtree.objectIds.empty(),
                "over-capacity hierarchy rejects without a partial set") &&
         expect(children.accepted &&
                    children.objectIds.size() ==
                        cr::kCreativeSelectionTargetCapacity,
                "the exact hierarchy capacity remains accepted");
}

// 13. Stale selected ids are discarded when resolving against document truth.
bool staleSelectionIdsAreDiscarded() {
  const std::vector<cr::CreativeObject> objects{
      makeObject(1U, std::nullopt, cr::CreativeObjectKind::Crate, "A"),
      makeObject(2U, std::nullopt, cr::CreativeObjectKind::Crate, "B")};
  const auto zero = app::resolveCreativeDesktopSelection(
      objects, std::vector<cr::CreativeObjectId>{404U}, 404U);
  const auto single = app::resolveCreativeDesktopSelection(
      objects, std::vector<cr::CreativeObjectId>{404U, 2U}, 404U);
  const auto multi = app::resolveCreativeDesktopSelection(
      objects, std::vector<cr::CreativeObjectId>{1U, 404U, 2U}, 2U);
  return expect(zero.objectIds.empty() &&
                    zero.primaryObjectId == cr::kInvalidObjectId,
                "an all-stale selection resolves to zero") &&
         expect(single.objectIds.size() == 1U && single.objectIds[0] == 2U &&
                    single.primaryObjectId == 2U,
                "a stale primary re-homes onto the first valid id") &&
         expect(multi.objectIds.size() == 2U && multi.primaryObjectId == 2U,
                "valid ids keep their order and a valid primary is kept");
}

// 14. Degree/radian conversion parity.
bool degreeRadianParity() {
  const cr::CreativeVec3 radians{0.5, -1.25, 2.0};
  const cr::CreativeVec3 degrees = app::creativeDesktopRadiansToDegrees(radians);
  const cr::CreativeVec3 back = app::creativeDesktopDegreesToRadians(degrees);
  return expect(near(degrees.x, 0.5 * 180.0 / 3.14159265358979323846),
                "radians convert to degrees at the boundary") &&
         expect(near(back.x, radians.x) && near(back.y, radians.y) &&
                    near(back.z, radians.z),
                "degrees convert back to the original radians");
}

// 15. Non-finite and non-positive-scale drafts are rejected.
bool draftRejectsNonFiniteAndNonPositiveScale() {
  const cr::CreativeVec3 zero{0.0, 0.0, 0.0};
  const cr::CreativeVec3 unit{1.0, 1.0, 1.0};
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();

  const auto valid =
      app::validateCreativeDesktopTransformDraft({1.0, 2.0, 3.0}, {0.0, 90.0,
                                                                   0.0}, unit);
  const auto infinite =
      app::validateCreativeDesktopTransformDraft({inf, 0.0, 0.0}, zero, unit);
  const auto notANumber =
      app::validateCreativeDesktopTransformDraft(zero, {nan, 0.0, 0.0}, unit);
  const auto zeroScale =
      app::validateCreativeDesktopTransformDraft(zero, zero, {1.0, 0.0, 1.0});
  const auto negativeScale =
      app::validateCreativeDesktopTransformDraft(zero, zero, {1.0, 1.0, -2.0});

  return expect(valid.valid && near(valid.transform.position.x, 1.0) &&
                    near(valid.transform.rotationEulerRadians.y,
                         90.0 * 3.14159265358979323846 / 180.0),
                "a finite, positive-scale draft converts degrees to radians") &&
         expect(!infinite.valid && !infinite.message.empty(),
                "a non-finite position is rejected with a message") &&
         expect(!notANumber.valid, "a NaN rotation is rejected") &&
         expect(!zeroScale.valid, "a zero scale component is rejected") &&
         expect(!negativeScale.valid, "a negative scale component is rejected");
}

bool generatedSourceScopesAreOrderedAndDoNotInventConnectorOwnership() {
  cr::CreativeWorldLayout layout;
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  layout.buildings.push_back(building);
  layout.levels.push_back({0U, "ground", "Ground"});
  layout.levels.push_back({0U, "upper", "Upper", 3.0});
  layout.rooms.push_back(
      {0U, 0U, "ground_room", "Ground Room", {{0, 0}, {8, 6}}, 0.25});
  layout.rooms.push_back(
      {0U, 1U, "upper_room", "Upper Room", {{0, 0}, {8, 6}}, 0.25});
  layout.walls.push_back(
      {0U, "partition", "Partition", {4, 0}, {4, 6}, 0.0, 3U, 0.25});
  cr::CreativeWorldLayoutOpening roomOpening;
  roomOpening.hostKind =
      cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  roomOpening.roomIndex = 1U;
  roomOpening.stableKey = "room_door";
  roomOpening.name = "Room Door";
  layout.openings.push_back(roomOpening);
  cr::CreativeWorldLayoutOpening wallOpening;
  wallOpening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
  wallOpening.wallIndex = 0U;
  wallOpening.stableKey = "wall_door";
  wallOpening.name = "Wall Door";
  layout.openings.push_back(wallOpening);
  layout.verticalConnectors.push_back(
      {0U,
       0U,
       1U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "stair",
       "Stair",
       {{1, 1}, {5, 3}}});

  const auto provenance = [](cr::CreativeWorldLayoutTable table,
                             std::size_t index) {
    return cr::CreativeWorldLayoutObjectProvenance{
        true, table, index, cr::CreativeWorldLayoutRoomEdge::Count, 1U};
  };
  const app::CreativeDesktopGeneratedSourceScopeModel room =
      app::buildCreativeDesktopGeneratedSourceScopeModel(
          layout, provenance(cr::CreativeWorldLayoutTable::Room, 1U));
  const app::CreativeDesktopGeneratedSourceScopeModel roomOpeningScopes =
      app::buildCreativeDesktopGeneratedSourceScopeModel(
          layout, provenance(cr::CreativeWorldLayoutTable::Opening, 0U));
  const app::CreativeDesktopGeneratedSourceScopeModel wallOpeningScopes =
      app::buildCreativeDesktopGeneratedSourceScopeModel(
          layout, provenance(cr::CreativeWorldLayoutTable::Opening, 1U));
  const app::CreativeDesktopGeneratedSourceScopeModel connector =
      app::buildCreativeDesktopGeneratedSourceScopeModel(
          layout,
          provenance(cr::CreativeWorldLayoutTable::VerticalConnector, 0U));
  const app::CreativeDesktopGeneratedSourceScopeModel unowned =
      app::buildCreativeDesktopGeneratedSourceScopeModel(layout, {});

  const bool roomPath =
      room.count == 3U &&
      room.entries[0].table == cr::CreativeWorldLayoutTable::Building &&
      room.entries[1].table == cr::CreativeWorldLayoutTable::Level &&
      room.entries[2].table == cr::CreativeWorldLayoutTable::Room &&
      room.directEntryIndex == 2U && room.entries[1].name == "Upper";
  const bool openingPath =
      roomOpeningScopes.count == 4U &&
      roomOpeningScopes.entries[0].table ==
          cr::CreativeWorldLayoutTable::Building &&
      roomOpeningScopes.entries[1].table ==
          cr::CreativeWorldLayoutTable::Level &&
      roomOpeningScopes.entries[2].table ==
          cr::CreativeWorldLayoutTable::Room &&
      roomOpeningScopes.entries[3].table ==
          cr::CreativeWorldLayoutTable::Opening &&
      roomOpeningScopes.directEntryIndex == 3U;
  const bool ambiguousPathsStayShort =
      wallOpeningScopes.count == 2U &&
      wallOpeningScopes.entries[0].table ==
          cr::CreativeWorldLayoutTable::Building &&
      wallOpeningScopes.entries[1].table ==
          cr::CreativeWorldLayoutTable::Opening &&
      connector.count == 2U &&
      connector.entries[0].table ==
          cr::CreativeWorldLayoutTable::Building &&
      connector.entries[1].table ==
          cr::CreativeWorldLayoutTable::VerticalConnector;

  return expect(roomPath,
                "room scope orders building level and direct room") &&
         expect(openingPath,
                "room-hosted opening retains every unambiguous owner") &&
         expect(ambiguousPathsStayShort,
                "wall openings and cross-level connectors omit guessed owners") &&
         expect(unowned.count == 0U,
                "unowned objects expose no generated source scopes");
}

cr::CreativeWorldLayout generatedScopeFixture() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "scope_layout";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "house";
  building.name = "House";
  layout.buildings.push_back(std::move(building));
  layout.levels.push_back({0U, "ground", "Ground"});
  layout.levels.push_back({0U, "upper", "Upper", 3.0});
  layout.rooms.push_back(
      {0U, 0U, "west", "West", {{0, 0}, {4, 4}}, 0.25});
  layout.rooms.push_back(
      {0U, 0U, "east", "East", {{4, 0}, {8, 4}}, 0.25});
  layout.rooms.push_back(
      {0U, 1U, "upper", "Upper", {{0, 0}, {8, 4}}, 0.25});
  layout.verticalConnectors.push_back(
      {0U,
       0U,
       2U,
       cr::CreativeWorldLayoutVerticalConnectorKind::Stair,
       cr::CreativeWorldLayoutVerticalDirection::PositiveX,
       "stair",
       "Stair",
       {{1, 1}, {3, 3}}});
  return layout;
}

cr::CreativeObject taggedObject(
    cr::CreativeObjectId id,
    std::string name,
    cr::CreativeBounds bounds,
    std::vector<std::string> tags,
    bool visible = true) {
  cr::CreativeObject object;
  object.id = id;
  object.kind = cr::CreativeObjectKind::Crate;
  object.name = std::move(name);
  object.bounds = bounds;
  object.tags = std::move(tags);
  object.visible = visible;
  return object;
}

std::vector<std::string> generatedTags(
    const cr::CreativeWorldLayout& layout,
    std::string sourceTag) {
  return {cr::creativeWorldLayoutTag(layout.stableKey),
          std::move(sourceTag)};
}

bool generatedScopeMembershipHonorsSharedEdgesAndNoInventedAncestry() {
  const cr::CreativeWorldLayout layout = generatedScopeFixture();
  const cr::CreativeObject west = taggedObject(
      1U, "West floor", {{0.0, 0.0, 0.0}, {4.0, 0.2, 4.0}},
      generatedTags(layout, cr::creativeWorldLayoutProvenanceTag(
                                layout, cr::CreativeWorldLayoutTable::Room,
                                0U)));
  const cr::CreativeObject east = taggedObject(
      2U, "East floor", {{4.0, 0.0, 0.0}, {8.0, 0.2, 4.0}},
      generatedTags(layout, cr::creativeWorldLayoutProvenanceTag(
                                layout, cr::CreativeWorldLayoutTable::Room,
                                1U)));
  const cr::CreativeObject shared = taggedObject(
      3U, "Shared wall", {{3.8, 0.0, 0.0}, {4.2, 3.0, 4.0}},
      {cr::creativeWorldLayoutTag(layout.stableKey),
       cr::creativeWorldLayoutRoomEdgeProvenanceTag(
           layout, 0U, cr::CreativeWorldLayoutRoomEdge::East),
       cr::creativeWorldLayoutRoomEdgeProvenanceTag(
           layout, 1U, cr::CreativeWorldLayoutRoomEdge::West)});
  const cr::CreativeObject connector = taggedObject(
      4U, "Stair", {{1.0, 0.0, 1.0}, {3.0, 3.0, 3.0}},
      generatedTags(
          layout, cr::creativeWorldLayoutProvenanceTag(
                      layout,
                      cr::CreativeWorldLayoutTable::VerticalConnector, 0U)));

  const auto belongs = [&](const cr::CreativeObject& object,
                           cr::CreativeWorldLayoutTable table,
                           std::size_t index) {
    return app::creativeDesktopGeneratedObjectBelongsToSourceScope(
        layout, object, table, index);
  };
  return expect(belongs(west, cr::CreativeWorldLayoutTable::Room, 0U) &&
                    belongs(shared, cr::CreativeWorldLayoutTable::Room, 0U) &&
                    !belongs(east, cr::CreativeWorldLayoutTable::Room, 0U),
                "west room includes its floor and shared wall only") &&
         expect(belongs(east, cr::CreativeWorldLayoutTable::Room, 1U) &&
                    belongs(shared, cr::CreativeWorldLayoutTable::Room, 1U),
                "shared wall belongs to every contributing room") &&
         expect(belongs(west, cr::CreativeWorldLayoutTable::Level, 0U) &&
                    belongs(east, cr::CreativeWorldLayoutTable::Level, 0U) &&
                    belongs(shared, cr::CreativeWorldLayoutTable::Level, 0U) &&
                    !belongs(connector, cr::CreativeWorldLayoutTable::Level,
                             0U),
                "level includes room geometry but not a cross-level connector") &&
         expect(belongs(connector, cr::CreativeWorldLayoutTable::Building,
                        0U) &&
                    belongs(connector,
                            cr::CreativeWorldLayoutTable::VerticalConnector,
                            0U) &&
                    !belongs(connector, cr::CreativeWorldLayoutTable::Room,
                             0U),
                "connector belongs to its building and direct source only");
}

bool generatedScopeSummaryCountsVisibilityAndMergesWorldBounds() {
  const cr::CreativeWorldLayout layout = generatedScopeFixture();
  cr::CreativeDocument document = cr::CreativeDocument::create("Scope");
  static_cast<void>(document.assignId(77U));
  const auto append = [&](std::string name, cr::CreativeBounds bounds,
                          std::vector<std::string> tags, bool visible) {
    cr::CreativeDocumentCreateRequest request;
    request.kind = cr::CreativeObjectKind::Crate;
    request.name = std::move(name);
    request.bounds = bounds;
    request.hasBoundsOverride = true;
    request.tags = std::move(tags);
    request.visible = visible;
    request.hasVisibleOverride = true;
    const cr::CreativeDocumentCreateReceipt receipt =
        document.createObject(request);
    return receipt.accepted ? receipt.objectId : cr::kInvalidObjectId;
  };
  const cr::CreativeObjectId westId = append(
      "West floor", {{0.0, 0.0, 0.0}, {4.0, 0.2, 4.0}},
      generatedTags(layout, cr::creativeWorldLayoutProvenanceTag(
                                layout, cr::CreativeWorldLayoutTable::Room,
                                0U)),
      true);
  const cr::CreativeObjectId sharedId = append(
      "Shared wall", {{3.8, 0.0, -1.0}, {4.2, 3.0, 5.0}},
      {cr::creativeWorldLayoutTag(layout.stableKey),
       cr::creativeWorldLayoutRoomEdgeProvenanceTag(
           layout, 0U, cr::CreativeWorldLayoutRoomEdge::East),
       cr::creativeWorldLayoutRoomEdgeProvenanceTag(
           layout, 1U, cr::CreativeWorldLayoutRoomEdge::West)},
      false);
  const cr::CreativeObjectId eastId = append(
      "East floor", {{4.0, 0.0, 0.0}, {8.0, 0.2, 4.0}},
      generatedTags(layout, cr::creativeWorldLayoutProvenanceTag(
                                layout, cr::CreativeWorldLayoutTable::Room,
                                1U)),
      true);
  const app::CreativeDesktopGeneratedSourceScopeSummary west =
      app::buildCreativeDesktopGeneratedSourceScopeSummary(
          document, layout, cr::CreativeWorldLayoutTable::Room, 0U);

  const auto provenance = cr::CreativeWorldLayoutObjectProvenance{
      true, cr::CreativeWorldLayoutTable::Room, 1U,
      cr::CreativeWorldLayoutRoomEdge::Count, 1U};
  const app::CreativeDesktopGeneratedSourceScopeModel scopes =
      app::buildCreativeDesktopGeneratedSourceScopeModel(layout, provenance);
  const std::size_t direct =
      app::resolveCreativeDesktopGeneratedSourceActiveScope(
          scopes, cr::CreativeWorldLayoutTable::Level, 99U);
  const app::CreativeDesktopGeneratedSourceScopeTint buildingTint =
      app::creativeDesktopGeneratedSourceScopeTint(
          cr::CreativeWorldLayoutTable::Building);
  app::CreativeDesktopGeneratedSourceScopeCache cache;
  const bool cacheBuilt =
      app::refreshCreativeDesktopGeneratedSourceScopeCache(
          cache, document, layout, 5U, 8U, 8U,
          cr::CreativeWorldLayoutTable::Room, 0U);
  bool idleCacheReused = true;
  for (std::size_t frame = 0U; frame < 300U; ++frame) {
    idleCacheReused =
        !app::refreshCreativeDesktopGeneratedSourceScopeCache(
            cache, document, layout, 5U, 8U, 8U,
            cr::CreativeWorldLayoutTable::Room, 0U) &&
        idleCacheReused;
  }
  const bool sourceReplacementRebuilt =
      app::refreshCreativeDesktopGeneratedSourceScopeCache(
          cache, document, layout, 6U, 8U, 8U,
          cr::CreativeWorldLayoutTable::Room, 0U);

  return expect(westId != cr::kInvalidObjectId &&
                    sharedId != cr::kInvalidObjectId &&
                    eastId != cr::kInvalidObjectId,
                "summary fixture objects are valid document objects") &&
         expect(west.valid && west.objectCount == 2U &&
                    west.visibleObjectCount == 1U &&
                    west.hiddenObjectCount == 1U,
                "scope summary distinguishes visible and hidden members") &&
         expect(west.hasBounds && near(west.worldBounds.min.x, 0.0) &&
                    near(west.worldBounds.min.y, 0.0) &&
                    near(west.worldBounds.min.z, -1.0) &&
                    near(west.worldBounds.max.x, 4.2) &&
                    near(west.worldBounds.max.y, 3.0) &&
                    near(west.worldBounds.max.z, 5.0),
                "scope summary merges exact transformed world bounds") &&
         expect(direct == scopes.directEntryIndex &&
                    scopes.entries[direct].table ==
                        cr::CreativeWorldLayoutTable::Room,
                "an unrelated 2D selection falls back to direct source") &&
         expect(near(buildingTint.r, 0.18, 1.0e-6) &&
                    near(buildingTint.g, 0.82, 1.0e-6) &&
                    near(buildingTint.b, 1.0, 1.0e-6),
                "shared scope tint policy pins the Building color") &&
         expect(cacheBuilt && idleCacheReused && sourceReplacementRebuilt &&
                    app::creativeDesktopGeneratedSourceScopeCacheContains(
                        cache, westId) &&
                    app::creativeDesktopGeneratedSourceScopeCacheContains(
                        cache, sharedId) &&
                    !app::creativeDesktopGeneratedSourceScopeCacheContains(
                        cache, eastId),
                "scope cache rebuilds only when its complete revision key changes");
}

bool historyModelPinsPriorityLabelsAndUnsynchronizedBlocking() {
  cr::CreativeDocument document = cr::CreativeDocument::create("History");
  static_cast<void>(document.assignId(991U));
  cr::CreativeDocumentHistory history;
  history.maxDepth = 4U;
  history.undoSnapshots.push_back({document, "desktop_duplicate_selection", {}});
  history.undoSnapshots.push_back({document, "keyboard_delete", {}});
  history.redoSnapshots.push_back({document, "desktop_paste", {}});

  app::CreativeEditorWorldLayoutState worldLayout;
  worldLayout.revision = 7U;
  worldLayout.generatedRevision = 6U;
  worldLayout.sourceHistory.maxDepth = 3U;
  app::CreativeEditorWorldLayoutSourceHistoryEntry older;
  older.source = "room resized";
  app::CreativeEditorWorldLayoutSourceHistoryEntry newer;
  newer.source = "layout symbol deleted";
  worldLayout.sourceHistory.undoEntries = {older, newer};
  app::CreativeEditorWorldLayoutSourceHistoryEntry redo;
  redo.source = "wall restored";
  worldLayout.sourceHistory.redoEntries = {redo};

  const app::CreativeDesktopHistoryModel unsynchronized =
      app::buildCreativeDesktopHistoryModel(history, &worldLayout);
  const bool sourcePriority =
      !unsynchronized.sourceSynchronized && unsynchronized.canUndo &&
      unsynchronized.canRedo && unsynchronized.undoEntries.size() == 4U &&
      unsynchronized.undoEntries[0].domain ==
          app::CreativeDesktopHistoryDomain::WorldLayout &&
      unsynchronized.undoEntries[0].label == "Layout symbol deleted" &&
      unsynchronized.undoEntries[0].nextAction &&
      unsynchronized.undoEntries[1].label == "Room resized" &&
      unsynchronized.undoEntries[2].label == "Delete" &&
      unsynchronized.undoEntries[2].blockedByUnsynchronizedSource &&
      !unsynchronized.undoEntries[2].nextAction &&
      unsynchronized.redoEntries[0].label == "Wall restored" &&
      unsynchronized.redoEntries[0].nextAction;

  worldLayout.revision = worldLayout.generatedRevision;
  worldLayout.sourceHistory.undoEntries.clear();
  worldLayout.sourceHistory.redoEntries.clear();
  const app::CreativeDesktopHistoryModel synchronized =
      app::buildCreativeDesktopHistoryModel(history, &worldLayout);
  return expect(sourcePriority,
                "history projects source edits first and blocks stale document history") &&
         expect(synchronized.sourceSynchronized && synchronized.canUndo &&
                    synchronized.canRedo &&
                    synchronized.undoEntries[0].label == "Delete" &&
                    synchronized.undoEntries[0].nextAction &&
                    synchronized.undoEntries[1].label ==
                        "Duplicate selection" &&
                    synchronized.redoEntries[0].label == "Paste" &&
                    synchronized.redoEntries[0].nextAction &&
                    synchronized.sourceMaxDepth == 3U &&
                    synchronized.documentMaxDepth == 4U,
                "history resumes newest-first document actions after synchronization");
}

}  // namespace

int main() {
  bool ok = true;
  ok = propertyEditIntentSeparatesPreviewCommitAndCancel() && ok;
  ok = emptyDocumentProjectsNoRows() && ok;
  ok = flattenKeepsParentBeforeChildAndSourceOrder() && ok;
  ok = missingParentBecomesRecoveredRoot() && ok;
  ok = cyclesRecoverAndEmitEachObjectOnce() && ok;
  ok = depthLimitClampsRatherThanDropping() && ok;
  ok = searchMatchesNameKindAndId() && ok;
  ok = searchRetainsAncestorPath() && ok;
  ok = plainSelectionReplaces() && ok;
  ok = toggleAddsAndRemoves() && ok;
  ok = shiftRangeSelectsBothDirections() && ok;
  ok = shiftWinsOverToggle() && ok;
  ok = missingAnchorFallsBackToPlain() && ok;
  ok = hierarchySelectionPlansParentChildrenAndSubtree() && ok;
  ok = selectionPlannersRejectOverCapacityAtomically() && ok;
  ok = staleSelectionIdsAreDiscarded() && ok;
  ok = degreeRadianParity() && ok;
  ok = draftRejectsNonFiniteAndNonPositiveScale() && ok;
  ok = generatedSourceScopesAreOrderedAndDoNotInventConnectorOwnership() && ok;
  ok = generatedScopeMembershipHonorsSharedEdgesAndNoInventedAncestry() && ok;
  ok = generatedScopeSummaryCountsVisibilityAndMergesWorldBounds() && ok;
  ok = historyModelPinsPriorityLabelsAndUnsynchronizedBlocking() && ok;
  return ok ? 0 : 1;
}
