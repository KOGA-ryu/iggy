export const meta = {
  name: 'ui-murder-screen',
  description: 'Rebuild one bespoke starter screen from the L1 UI widget layer, byte-identical, and adversarially verify (Sonnet 5 agents)',
  whenToUse: 'Continue the iggy3d UI widget migration. Pass a screen key as args: "settings", "dev_tools", or "new_world". The workflow migrates that emitter onto the widget layer output-preservingly, gates it, and verifies. It does NOT commit — you review the diff and commit.',
  phases: [
    { title: 'Implement', detail: 'rebuild the target emitter through the widgets + run the receipt gate', model: 'sonnet' },
    { title: 'Verify', detail: 'adversarial byte-identity + gate check by independent skeptics', model: 'sonnet' },
  ],
}

const REPO = '/Users/kogaryu/iggy3d'

// args may be a bare string ("settings") or an object ({screen:"settings"}).
const target =
  (typeof args === 'string' && args) ||
  (args && typeof args === 'object' && args.screen) ||
  'settings'

const SCREENS = {
  settings: {
    fn: 'emitSettingsContent',
    idPrefix: 'starter.content.settings',
    tests: 'settingsAndDevToolsChildScreensAreModeled',
  },
  dev_tools: {
    fn: 'emitDevToolsContent',
    idPrefix: 'starter.content.dev_tools',
    tests: 'settingsAndDevToolsChildScreensAreModeled',
  },
  new_world: {
    fn: 'emitNewWorldContent',
    idPrefix: 'starter.content.new_world',
    tests: 'newWorldChildScreenBuildsReadySelectorSurface + newWorldEditModeShowsCursorPaletteAndLastGlyph',
  },
}

const screen = SCREENS[target]
if (!screen) {
  log(`unknown screen "${target}" — choose one of: ${Object.keys(SCREENS).join(', ')}`)
  return { error: `unknown screen "${target}"`, valid: Object.keys(SCREENS) }
}

const PATTERN = `
MIGRATION PATTERN (proven twice — emitDeleteConfirmContent and emitLoadSaveContent in
src/app/iggy3d/menu/DrawList.cpp are the reference; read them first):
1. Rewrite each emitText(list, TONE, RECT, id, text, action, selected, enabled) as
   emit(UiText{...}, out), and each emitRect(list, KIND, TONE, RECT, id, ...) as
   emit(UiPanel{...}, out), accumulating into a local "WidgetOutput out;" then
   "appendWidgetOutput(list, out);" at the end of the function.
2. MANDATORY: designated initializers, in field order, omitting trailing defaults:
   UiText{.rect=..., .tone=..., .semanticId=..., .text=..., .action=..., .selected=..., .enabled=...}
   UiPanel{.rect=..., .kind=..., .tone=..., .semanticId=..., .action=..., .selected=..., .enabled=...}
3. ARG-ORDER FLIP: old emitText is (list, TONE, RECT, ...); UiText is {RECT, TONE, ...}.
   Tone and rect are SWAPPED. Do not transpose them.
4. EMISSION-PRESERVING: identical primitives, identical ORDER, identical
   tone/rect/semanticId/text/action/selected/enabled, identical textCount/rectCount.
   Do NOT change any coordinate, tone, id, or ordering. No auto-layout. This is an
   output-preserving refactor — the draw-list receipt must stay byte-identical with
   ZERO edits to the test file.
5. string_view -> string needs an explicit std::string(...) wrap (e.g. action.label).
6. Include is already present (DrawList.cpp includes app/iggy3d/ui/Widget.hpp).
`

const CONTEXT = `
Repo: ${REPO} (C++20 engine, branch iggy3d-main). Read docs/ui/ui_widget_handoff.md and
docs/ui/ui_architecture.md for full context. The L1 widget layer lives in
src/app/iggy3d/ui/Widget.hpp. Suite baseline is 172/172 green and MUST stay green.
TARGET this run: the bespoke emitter "${screen.fn}" in src/app/iggy3d/menu/DrawList.cpp
(semanticId prefix "${screen.idPrefix}"), guarded by the receipt test(s): ${screen.tests}.
${PATTERN}
`

const VERDICT_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  properties: {
    gatePassed: { type: 'boolean', description: 'did the full ctest suite pass 172+/all green' },
    byteIdentical: { type: 'boolean', description: 'is the new emit provably output-identical to the old' },
    testFileUnchanged: { type: 'boolean', description: 'was tests/unit/product_ui_draw_list_tests.cpp left unedited (a rebuild must not touch its own receipt)' },
    issues: { type: 'array', items: { type: 'string' } },
    reasoning: { type: 'string' },
  },
  required: ['gatePassed', 'byteIdentical', 'testFileUnchanged', 'issues', 'reasoning'],
}

phase('Implement')
const impl = await agent(
  `${CONTEXT}

TASK: Migrate ${screen.fn} onto the widget layer following the pattern EXACTLY.
Steps:
1. Read the two reference functions (emitDeleteConfirmContent, emitLoadSaveContent) and
   the current ${screen.fn} in src/app/iggy3d/menu/DrawList.cpp.
2. Edit ${screen.fn} in place — rewrite its emit* calls through the widgets, designated
   initializers, output-preserving. Change ONLY that function. Do NOT edit any test file.
3. Build and gate:
   make -C ${REPO}/build -j8 iggy3d product_ui_draw_list_tests
   ctest --test-dir ${REPO}/build -R product_ui_draw_list_tests --output-on-failure
   then the full suite: make -C ${REPO}/build -j8 && ctest --test-dir ${REPO}/build
4. If the receipt or suite is RED, your rewrite diverged — fix it until byte-identical
   and green. Do NOT weaken or edit the tests to make them pass.
5. Do NOT git commit. Leave the change in the working tree.

Report: what you changed, the final ctest summary line (e.g. "X tests passed, 0 failed"),
and paste the output of: git -C ${REPO} diff --stat. Be honest if it is not green.`,
  { label: `murder:${target}`, phase: 'Implement', model: 'sonnet' }
)

phase('Verify')
const verdicts = await parallel(
  [1, 2].map((n) => () =>
    agent(
      `${CONTEXT}

An implementer just rewrote ${screen.fn} through the widget layer (uncommitted, in the
working tree). ADVERSARIALLY VERIFY it. Read the diff with: git -C ${REPO} diff -- src/app/iggy3d/menu/DrawList.cpp
and, if needed, git -C ${REPO} diff. Then:
- Confirm the change is OUTPUT-IDENTICAL to the original hand-emit: every primitive in
  the same order, with identical tone, rect, semanticId, text, action, selected, enabled,
  and identical textCount/rectCount contributions. Hunt for a swapped semanticId/text, a
  swapped tone/rect, a dropped/added primitive, a reordered emit, a changed coordinate.
- Confirm NO test file was edited (git diff must not touch tests/): a rebuild that edits
  its own receipt is invalid.
- Re-run the gate yourself: ctest --test-dir ${REPO}/build -R product_ui_draw_list_tests
  and ctest --test-dir ${REPO}/build ; report whether it is green.
Default to byteIdentical=false unless you can concretely confirm identity from the diff.
(Reviewer instance ${n}.)`,
      { label: `verify:${target}:${n}`, phase: 'Verify', model: 'sonnet', schema: VERDICT_SCHEMA }
    )
  )
)

const good = verdicts.filter(Boolean)
const green = good.length > 0 && good.every((v) => v.gatePassed && v.byteIdentical && v.testFileUnchanged)
log(`${screen.fn}: ${green ? 'VERIFIED green + byte-identical' : 'NEEDS ATTENTION — see verdicts'}`)

return {
  target,
  screen: screen.fn,
  verified: green,
  implementerReport: impl,
  verdicts: good,
  nextStep: green
    ? `Review "git diff" and commit: claude: rebuild ${screen.fn} from widgets (byte-identical). Then re-run this workflow for the next screen.`
    : `Do NOT commit. The verifiers flagged issues (see verdicts[].issues) — fix in the working tree or revert with git checkout -- src/app/iggy3d/menu/DrawList.cpp.`,
}
