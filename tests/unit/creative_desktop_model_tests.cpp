#include "EditorDesktopModel.hpp"

#include "app/iggy3d/creative/document/Object.hpp"
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

// 16. Only one-to-one generated sources permit raw refinement + adoption.
bool generatedSourceAdoptionPolicyKeepsStructuresSourceOwned() {
  const auto provenance = [](bool owned, cr::CreativeWorldLayoutTable table,
                             std::size_t contributors) {
    return cr::CreativeWorldLayoutObjectProvenance{
        owned, table, 0U, cr::CreativeWorldLayoutRoomEdge::Count,
        contributors};
  };
  return expect(app::creativeDesktopGeneratedSourceSupportsAdoption(
                    provenance(true, cr::CreativeWorldLayoutTable::Object,
                               1U)) &&
                    app::creativeDesktopGeneratedSourceSupportsAdoption(
                        provenance(true, cr::CreativeWorldLayoutTable::Box,
                                   1U)),
                "one-to-one object and box outputs permit adoption") &&
         expect(!app::creativeDesktopGeneratedSourceSupportsAdoption(
                    provenance(true, cr::CreativeWorldLayoutTable::Wall,
                               1U)) &&
                    !app::creativeDesktopGeneratedSourceSupportsAdoption(
                        provenance(
                            true,
                            cr::CreativeWorldLayoutTable::VerticalConnector,
                            1U)) &&
                    !app::creativeDesktopGeneratedSourceSupportsAdoption(
                        provenance(true,
                                   cr::CreativeWorldLayoutTable::Opening,
                                   1U)) &&
                    !app::creativeDesktopGeneratedSourceSupportsAdoption(
                        provenance(true, cr::CreativeWorldLayoutTable::Object,
                                   2U)) &&
                    !app::creativeDesktopGeneratedSourceSupportsAdoption(
                        provenance(false,
                                   cr::CreativeWorldLayoutTable::Object,
                                   1U)),
                "structures condensed outputs and unowned objects stay locked");
}

}  // namespace

int main() {
  bool ok = true;
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
  ok = staleSelectionIdsAreDiscarded() && ok;
  ok = degreeRadianParity() && ok;
  ok = draftRejectsNonFiniteAndNonPositiveScale() && ok;
  ok = generatedSourceAdoptionPolicyKeepsStructuresSourceOwned() && ok;
  return ok ? 0 : 1;
}
