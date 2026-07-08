# Refactor Target Backlog

Swarm scan of HEAD (5 lenses: size, duplication, dead-code, architecture, perf), ranked by **effort
descending** — biggest work first. Every target verified in code. Pick from the top; each needs its own
local preflight + owner check + focused test before Codex starts. `S<0.5d · M<2d · L<1wk · XL>1wk`.

## XL

**1. Table-drive the Receipt `*Fields.cpp` boilerplate family** · `src/app/iggy3d/receipt/*Fields.cpp` (14 files, ~2429 lines; worst: `GameplaySceneStateFields.cpp` 386, `SaveStateFields.cpp` 333)
Hand-written walls of `appendReceiptField(receipt, "literal_key", window.deep.path)` — key + path duplicated, order-pinned. **Do:** a declarative `ReceiptFieldRow {key, accessor-lambda, value-kind}` driven from a `static const std::array` per section. Prove the pattern on `GameplaySceneStateFields` first; the checked-in key-order oracle is your safety net (must stay byte-identical). Pairs with #6.

## L

**2. Split `Controller.cpp`** · `src/app/iggy3d/gameplay/Controller.cpp` (2428 lines, ~90 helpers, one export `applyProductGameplayActions`)
Biggest file in the tree; distinct concerns jammed in one anon namespace. **Do:** split along existing seams into sibling TUs behind narrow internal headers — `ControllerWallTraversal` (the ~500-line cluster), `ControllerJumpDash`, `ControllerGroundQueries`, `ControllerTargeting`, `ControllerMovementProof` (the 18 `record*/clear*/publish*`). Keep `applyProductGameplayActions` + `*Phase` orchestrators as the composition root. Extract pure kinematics (`clampHorizontalVelocity`, `moveHorizontalVelocityToward`) with unit tests. *(Active M&A lane — coordinate.)*

**3. Extract `iggy3d_creative` `main()`** · `apps/iggy3d_creative/main.cpp` (1651 lines; ~1160-line per-frame while-loop)
The entire standalone editor is one `int main()`. **Do:** a `CreativeEditorState` struct owning the free locals (camera, tool/place mode, gizmo drag anchors, key edge-latches) + named frame stages in `EditorFrame.{hpp,cpp}` (`pollFlyCameraInput`, `applyToolSwitchKeys`, `resolveAimGroundCell`, `resolveClickSelection`, `applyPlace`, `computeGizmoGeometry`, `handleGizmoHitTest`, `applyMoveDrag`, `buildEditorOverlays`). `main()` → parse → build → seed → `while(open) runCreativeEditorFrame(state, deps)`. The `--capture` path is the headless test seam.

**4. Collapse `ProductCreativeUiCommandFrameReceipt`** · `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp:17-113` (flat 71+ prefixed fields + 2 hand-written copy layers)
**Do:** replace the flat struct with composition (members `mutation/create/remove/undo/shell` of the real sub-receipt types); delete the 6 `copyXxxReceipt` fns (assign whole sub-receipts); each `*Diagnostics` built by one `toDiagnostics(sub)`. Update ~30 test refs.

**5. Encapsulate `MutationApply.hpp`** · `src/app/iggy3d/creative/mutation/MutationApply.hpp` (40 decls; 3 dead fns; 26 handlers leaked to header)
**Do:** delete the 3 dead fns; move the 26 `applyXxxMutation` handlers + `makeMutationApplyReceipt` into an anon namespace in the `.cpp`; leave ~3 real entry points in the header. Bundle with #14 as one "tighten creative header surfaces" effort.

**7. Wireframe fast-path** · `src/app/iggy3d/creative/document/DocumentWireframe.cpp:52,411,422`
Per-frame: fully voxelizes every object into a `std::vector` just to read `cells.size()`. **Do:** add `projectObjectToGridSummary()` returning `{status, profile, occupancyKind, projectedBounds, cellCount}` computed **without** allocating the cells vector (box/volume = arithmetic; line/path = analytic count). Land before/with #14 (reshapes the projectors).

## M

**6. `ProductAppReceiptContext`** · `src/app/iggy3d/ReceiptBuilder.cpp:32-93` (8-arg `buildProductAppReceipt` + 12 long-param appenders)
**Do:** a `ProductAppReceiptContext` holding the inputs + derived frames, built once; every appender takes `(receipt, ctx)`. Pairs naturally with #1.

**8. Table-drive `SaveCodec` enum serialization** · `src/runtime/save/SaveCodec.cpp` (21 `enumText()` + 22 `parseEnum()` hand-synced across ~20 enums)
**Do:** one X-macro / constexpr-table (`ENUM_STRING_TABLE`) listing each enumerator once → both `toName`/`fromName`. Delete the hand-written bodies. (string == enumerator-name holds universally.)

**9. De-dup Automation `resolveProduct*Automation`** · `src/app/iggy3d/automation/Automation.cpp` (~12× `find_if` + double-table lookup; the doubled arrays self-desync)
**Do:** one generic `lookupAutomationRow(rows, value, fallback)`; delete every duplicated lookup array (keep `rows` + a single fallback). Removes a sync foot-gun + ~300 lines.

**10. Extract Automation owner-guard skeleton** · `Automation.cpp` (16 `canonicalKey == "..."` world/ascii branches repeat the guard/report skeleton)
**Do:** `requireNewWorldOwner(context, command, key)` + `reportApplied(context, command, key, ok)`; drive the pure "set draft field from value + record" branches from a small `{key, member}` table.

**11. Extract `Facade::applyMoveDragIntent`** · `src/app/iggy3d/creative/Facade.cpp:567-768` (~200-line method)
**Do:** phases → `resolveMoveDragAnchor` / `applyAxisHolds` / `applySnap` / `buildMoveMutation`; pull the pure geometry into a free function with unit tests.

**12. Creative enum `toString()` → tables** · `ObjectDescriptor.cpp` (+ `Mutation.cpp`, `MutationApply.cpp`) — ~8 switch-return functions
**Do:** reuse #8's table pattern for `CreativeObjectCategory/Profile/ShapeKind/RuntimeAnchorSemantic/DirtyFlag/MutationCategory/...`; one `{enumerator,name}` table per enum, switch body → lookup.

**13. Halve `FramePresenter` present bodies** · `src/app/iggy3d/window/FramePresenter.cpp:779-901,903-1155`
**Do:** extract backend-agnostic `buildPresenterFrameInputs(request)->PresenterFrameInputs` (HUD formatting, top-down-map overlay, starter menu input, overlay attach); each `present*Frame` → `build + <backend>.submit + backend glue`.

**14. Encapsulate `SpatialProjection`'s 7 per-shape projectors** · `SpatialProjection.hpp`
**Do:** move the 7 internal projectors into an anon namespace in the `.cpp`, header keeps only `projectObjectToGrid`. Do with #5; **land #7 first** (may reshape these).

## S (high-impact only)

**15. Creative dead-code sweep** · 6 empty (0-byte) adapter files `creative/adapters/{Draw,ObjCat,RoomEd}.{cpp,hpp}` + 7 dead payload builders + 7 `DocumentMutation` wrappers + 5 predicate wrappers + 4 `Stats` fields
**Do:** `git rm` the 6 files; delete the dead fns/wrappers/fields from `.hpp`+`.cpp`. **Same commit:** delete the `Stats` asserts in `tests/unit/creative_core_tests.cpp:32-46`. *(If rotate/resize gizmo wiring is imminent, keep `rotateDocumentObject`/`resizeDocumentObject`.)*

**16. Stop the per-tick collider copy in `reasoningSegmentBlocked`** · `src/runtime/ai/ReasoningGraph.cpp:64-95` (`:81` copies the whole collider set every call — AI hot path)
**Do:** take `const std::vector<PhysicsAabbCollider>&` (or a span overload of `raycastPhysicsAabbs`); delete the copy.

**17. `describeObject` O(1)** · `src/app/iggy3d/creative/document/ObjectDescriptor.cpp:1896-1904` (~38-comparison linear scan over ~76 kinds, per call)
**Do:** index directly by `static_cast<size_t>(kind)` with a guard; add a `static_assert` loop pinning `kDescriptors[i].kind == (Kind)i` so reorders fail the build. Bundle with #7/#14.

---
*Source: swarm `wim4nxz90`. Full per-finding detail (incl. line-exact clusters) in that run's output.*
