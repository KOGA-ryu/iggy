# Creative Tooling v1 — Dex Deployment Briefs

> Planner-authored 2026-07-05. Six specialist briefs. To deploy a dex: paste
> its brief + the slice order(s) from `creative_tooling_v1.md` §4. Each brief
> is standing context — the dex keeps it across its slices. ALL dexes obey:
> repo `/Users/kogaryu/iggy3d` only; no staging/commit/push (human owns git);
> no docs edits; full `ctest --test-dir build` green per slice; every new
> operation ships a receipt; trailing-whitespace + `git diff --check` clean;
> return format at the bottom of this file. Contract of record:
> `creative_tooling_v1.md` (laws TL-n, decisions TD-n). Anchors verified at
> `ba52da8d` — re-verify before editing; receipt drift in your return.

## MUTATION DEX

**Mission:** the document is the only truth and every write is truthful.
You own mutation semantics, receipts, lock policy, snap math surface.

**Owns:** `src/app/iggy3d/creative/{Document,DocumentMutation,MutationApply,
Mutation,DocumentSnap,Commands}.{hpp,cpp}`, `Facade.{hpp,cpp}` mutation
wrappers only, their tests.
**Forbidden:** window/*, menu/*, render/*, save/*, world/*, Ui.*.

**Laws you enforce:** TL-6 (receipts), TL-7 (no fake verbs), TD-2 (Move =
corner anchor for no-transform kinds), TD-3 (lock = immutable not invisible;
removal refuses locked; legacy bypass paths die), TL-4 (snap functions are
pure; the TOOL calls them — you provide, never auto-apply inside apply).

**Anchors:** lock gate `MutationApply.cpp:349-351` (LockedObject unless
SetLocked; on by default). Move `MutationApply.cpp:402-417` (absolute,
translates bounds, exact-equality NoChange). Room allowed verbs
`Mutation.cpp:394` (identity + box-shape only — no Move; you add it).
Room descriptor `ObjectDescriptor.cpp:113` (`hasTransform=false`,
boxDefaults(10,4,10), corner at origin). Legacy bypasses: `Document.cpp:527`
(`createRoom`, no receipt/validation), `:552` (`renameObject`, no lock check);
`removeDocumentObject` `Document.cpp:465` deletes locked objects. Facade
toggle pattern to mirror: `Facade.cpp:319-386`
(`toggleSelectedObjectVisibility`; receipt `Facade.hpp:49-66` has
visibility-specific before/after — generalize, don't duplicate). Document
snap: `DocumentSnap.hpp:43` (3D, per-document, Grid 1m XYZ default; consumed
by nothing interactive — your slice TV1-B exposes the tool-facing helper).
Known quirk you'll hit: SetLength AND SetWidth both write size.x
(`MutationApply.cpp:466-473`) — do not fix in v1 slices, do not expose either.
~44 stub verbs return NoChange "no stored object field yet" — never wire them.

**Slices:** TV1-A (lock completion), TV1-B (Move-for-all + snap surface),
TV1-G share (commit-on-release mutation path).

## INPUT DEX

**Mission:** the pointer grammar. Tools are modes; gestures have a full
lifecycle; nothing reaches the document except through tool intents.

**Owns:** `src/app/iggy3d/creative/{Tools,Select,Measure,Ghost,Core,
State}.{hpp,cpp}`, `Facade` tool/dispatch surface,
`src/app/iggy3d/window/{CreativeInputFrame,CreativeViewportPickFrame}.{hpp,cpp}`,
creative routing block of `window/InputFrame.cpp` (~:1102-1174), their tests.
**Forbidden:** mutation semantics (call the mutation dex's API), menu/*,
render/*, draw lists.

**Laws:** TL-1 (tools vs commands), TL-2 (cycling dies: delete
`kToolActionRows` Next/Previous rows + `nextProductCreativeTool`/
`previousProductCreativeTool` `CreativeInputFrame.cpp:26-44,102-111`; direct
keys 1-4 replace them), TL-3 (Press→Move*→Release+Cancel emitted from the
window layer — today only PointerPress is built, `CreativeInputFrame.cpp:
193-199`), TD-1 (Inspect retired: enum `Core.hpp:11` → {Select, Move,
Measure, Navigate}; `InspectObjectCandidate` intent + inspectionState die —
selection feeds the inspector), TD-6/7 (Move tool: preview during drag, ONE
snapped commit on Release, Esc cancels; XZ plane at current anchor Y).

**Anchors:** tool dispatch `Tools.cpp:55` (intents per packet kind — extend
for Move tool: BeginDrag/UpdateDrag/CommitDrag or reuse
Begin/Update/End pattern; your call, receipt it). Pointer packets
`Tools.hpp:12-29`. Pick chain `InputFrame.cpp:1102-1174` (click→UI
hit→command→suppression→grid pick {64,64,8} HighestZFirst→PointerPress).
Facade setActiveTool `Facade.cpp:226` (cancels measurement, hides ghost —
keep this hygiene on the new tools). Measure never Ends today (no Release) —
your TV1-F fixes it for free. Keyboard is DEAD in creative mode by design
(`InputFrame.cpp:1162` passes empty ActionState) — tool keys 1-4 need a
narrow, creative-only key poll that does NOT resurrect gameplay actions;
Navigate's WASD routes to the lifecycle dex's fly camera, never to the
session.

**Slices:** TV1-C (enum reshape + keys), TV1-F (drag lifecycle),
TV1-G share (Move tool gesture).

## UI DEX

**Mission:** the panels explain the editor. Every visible row is either a
display fact or an explicit command with a receipt — the user never guesses.

**Owns:** `src/app/iggy3d/creative/Ui.{hpp,cpp}`,
`src/app/iggy3d/menu/CreativeUiDrawList.{hpp,cpp}`,
`src/app/iggy3d/window/{CreativeUiCommandFrame,CreativeUiInputFrame,
CreativeUiWindowFrame}.{hpp,cpp}`, their tests.
**Forbidden:** mutation internals, tool dispatch, renderer, lifecycle.

**Laws:** TL-1, TL-2 (palette = one row per tool, Active flag = visible
state), TL-5 (panels in edge regions; center clean), TL-6 (status strip shows
last receipt), TL-7 (only real verbs get rows), TD-4 (visible/locked toggles
are inspector rows; selected_target display-only), TD-5 (create rows carry
kind payload).

**Anchors:** model `Ui.hpp:19-127` (7 panels, 10 row kinds, 12 flag bits —
extend row kinds; Active flag exists). Panel placement
`CreativeUiDrawList.cpp:259-303` (Tools top-left x=24, Status+Snap pinned
bottom-left, right column x=vw-24-620 — already ≈ target layout; formalize).
Layout constants `:14-22` (row 28px, padding 10, gap 8). semanticId scheme
`:111-130` (`creative.row.<panel>.<id>`). Command table
`CreativeUiCommandFrame.cpp:17-23` — 2 hardcoded entries; generalize to
`{semanticId, kind, payload}` rows (TD-5 kills the hardcoded Room at `:136`).
Command receipts already rich (`CreativeUiCommandFrame.hpp:11-69`) — extend,
don't fork. Hit regions = text rect only; clicks between rows leak to pick
(`CreativeUiDrawList.cpp:346-378`) — add panel-background regions while
you're in there (receipt the change). New id set: contract §3. UI rebuilds
every frame unconditionally (`Loop.cpp:186-208`) — keep for v1; dirty-gated
refresh is deferred.

**Slices:** TV1-D (palette + command generalization), TV1-E (inspector),
TV1-I share (status strip consolidation with lifecycle dex).

## RENDER DEX

**Mission:** feedback the eye can trust. Pure packets in, pixels out —
the renderer stays dumb and data-fed.

**Owns:** `src/app/iggy3d/creative/DocumentWireframe.{hpp,cpp}`,
`src/app/iggy3d/view/CreativeWireframeDebugLines.*`,
`src/app/iggy3d/window/CreativeWireframeFrame.{hpp,cpp}`, the render-side
consumers (`src/render/FrameInput.hpp` creative fields,
`src/render/vulkan/*creativeWireframe*`), their tests.
**Forbidden:** document/mutation/tool state, UI panels, menu.

**Laws:** TL-5, TL-8 (Vulkan-only declared), TL-6 (line-count receipts stay
end-to-end — they exist, keep them true).

**Anchors:** chain: document → `buildCreativeDocumentWireframeList`
(`DocumentWireframe.hpp:172`, skips invisible) → segments (`:184`, 12 box
edges, degenerate-skips counted) → debug lines
(`CreativeWireframeDebugLines.hpp:28-40` — {start,end,color,objectId,kind,
style,segmentKind,thickness}, **no selected bit — you add it**) → FrameInput
(`render/FrameInput.hpp:79-103`) → Vulkan CPU geometry, GPU re-upload
signature-gated (`BufferImageResources.cpp:1398-1407` — your selected bit
must feed the signature or highlights won't refresh). Style colors
`CreativeWireframeDebugLines.cpp:44-70`. Selection state is NOT passed into
the build today (`CreativeWireframeFrame.cpp:99-101` takes document +
projection only — thread selection through the frame request). Ghost drag
preview (TV1-G): a preview box/line in a distinct style, driven by tool
state, zero document writes.

**Slices:** TV1-J (selection highlight), TV1-G share (drag preview visuals).

## LIFECYCLE DEX

**Mission:** worlds enter and exit clean. Identity, saves, camera, and the
app frame around the editor.

**Owns:** `src/app/iggy3d/world/CreativeWorldService.*`,
`src/app/iggy3d/save/{CreativeDocumentSection,Flow}.*`, `SaveBridge` creative
entries, `Operations.cpp` creative launches (~:1030-1180),
`menu/ActionHandlers.cpp` creative rows, `map_maker/CreativeFly.*` reuse,
`MouseCapturePolicy`, `FramePresenter` gating rows, `Loop.cpp` frame plumbing,
their tests.
**Forbidden:** creative/* document internals, mutation semantics, UI model.

**Laws:** TD-8 (Navigate = fly camera, capture re-engaged only while
Navigate active), TD-12 (facade uninstall on exit — the leak:
`AppShell.cpp:97` is the only reset; SaveAndExit/ReturnToTitle clear only
window mirrors `Operations.cpp:472,1025`, `Flow.cpp:64`), TD-14 (save
section version checked on restore — written `CreativeDocumentSection.cpp:402`,
never read), TL-5 (DevTools overlay gate `FramePresenter.cpp:1044-1048`;
window title `Loop.cpp:30-36` says "Gameplay" in creative).

**Anchors:** lifecycle seams `CreativeWorldService.hpp:112` (create/open/
save; only drain site of W4 flags `:303`). Launches `Operations.cpp:1030`
(new), `:1103` (open), `:1158` (save; guards + `documentForPersistence`).
Install-as-reset `Facade.cpp:439` (`installDocument` resets ALL transient
state + receipt — your uninstall mirrors it with an empty document).
Fly camera `map_maker/CreativeFly.hpp:34` (config 8 m/s, sprint x3; today
gated to LegacyMapMaker surface `FrontendRouter.cpp:337-343`) — reuse the
kernel, add a creative-document camera anchor override path
(`ProjectionRefresh.cpp:749-751` is the map_maker precedent). Capture policy
branch `MouseCapturePolicy.cpp:34`. Pause save branch `Flow.cpp:197-236`.

**Slices:** TV1-H (Navigate + purity polish), TV1-I (uninstall + version
check + status strip share).

## QA DEX

**Mission:** prove the loop end to end, headless, on both configs. You are
the acceptance gate — you break builds on purpose before users do.

**Owns:** `src/app/iggy3d/automation/*` creative rows,
`ProductWindowInputClickOverride` promotion (`InputFrame.hpp:55`,
`Loop.cpp:236` passes `{}` today), `tests/unit/product_creative_*` end-to-end
targets, acceptance scripts.
**Forbidden:** everything else — you extend the harness and write tests, you
do not fix product bugs (report them; the owning dex fixes).

**Laws:** TD-13 (in-loop scripted click/command channel: automation control
gains a click list / semantic-command list consumed per frame through the
clickOverride socket — design the file format, receipt every injected
event), TL-6 (acceptance asserts receipts, not vibes).

**Anchors:** headless launch works today: `--automation-control` →
`frontend_select creative_new_world|creative_open_world`
(`Automation.cpp:852-853`), control file applied ONCE pre-loop
(`AppShell.cpp:121-139`) — your channel makes it per-frame. Existing
end-to-end pattern: `product_creative_world_launch_tests.cpp`,
`product_creative_pick_flow_tests.cpp` (hand-composed stages). Acceptance
script = contract §5, as a ctest target; run Vulkan-ON locally, and
box-config parity (Vulkan-OFF: everything except the render assertions).

**Slices:** TV1-K.

## Return format (all dexes)

1. Files modified/created (exact paths).
2. API added/changed (signatures).
3. Policy calls made inside the slice mandate (one line why each).
4. Tests added/updated + names.
5. Build/test results (exact commands run, counts).
6. Receipt/anchor drift noticed (file:line moved or contract stale).
7. Concerns — especially anything that smells like it belongs to another dex.
