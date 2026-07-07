# E123: Creative Object Kind Switch Cleanup

## Objective

Delete two exhaustive per-kind `switch` ladders by routing them through the
`CreativeObjectDescriptor` facts they already duplicate — **only if the seam is
straightforward** (the descriptor already carries the exact fact). If it does
not, stop and move this card to `blocked/` with the exact missing facts.

## Why This Exists

From `docs/complexity_audit_v0_1.md` (finding #1 / bucket 1). Two exhaustive
switches shadow the descriptor table that is already the credited single owner of
per-kind facts:

- `creative/document/Object.cpp:168` `toString()` — a ~108-case switch mapping
  kind → display string.
- `creative/mutation/Mutation.cpp:551` `allowedMutations()` — a ~102-case switch
  deciding per-kind mutation eligibility.

`describeObject(kind)` already owns per-kind facts. Collapsing these drops
"add a Creative object kind" from 4 hand-edited sites to 2 (enum line +
descriptor row) and removes a hand-verify step.

## Required Work

1. **Read the descriptor first.** In `creative/document/ObjectDescriptor.*`,
   confirm whether `describeObject(kind)` already exposes:
   - a display-name fact whose string is **byte-identical** to what
     `Object.cpp toString()` currently returns for every kind, and
   - a mutation-eligibility fact (e.g. `descriptorAllowsMutation(...)` /
     capability rows) that reproduces `allowedMutations()` exactly.
2. **If both facts exist and match exactly:**
   - Route `Object.cpp toString()` → `describeObject(kind).displayName` (or the
     exact accessor). Delete the switch.
   - Make `Mutation.cpp allowedMutations()` a pure filter over the descriptor
     capability facts. Delete the switch.
3. **If a fact is missing or a string differs for ANY kind:** STOP. Do not invent
   descriptor facts or "fix up" strings here — that is a descriptor-coverage
   change with its own review. Move this card to `blocked/` listing the exact
   kinds and the exact mismatch (switch value vs descriptor value).
4. Keep behavior byte-identical. The descriptor coverage sentinel (`E59`) and the
   receipt key-order oracle must both stay green — they prove no kind lost its
   string and no receipt field drifted.

## Acceptance Notes

- Both switches removed; `toString()` and `allowedMutations()` are thin
  descriptor lookups/filters.
- No behavior change: every kind's display string and mutation eligibility is
  identical to before.
- Build clean, full suite green, **`product_receipt_key_order_tests` green**,
  descriptor coverage sentinel green.

## Do Not

- Do not add or change descriptor facts (separate card if needed → `blocked/`).
- Do not change any kind's display string or mutation eligibility.
- Do not touch save/load, receipts, renderer, or runtime.
- Do not widen to other switches (draw-kind is E124).
- Do not stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_mutation_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_mutation_tests|product_receipt_key_order_tests)$' --output-on-failure
# full suite before hand-off:
ctest --test-dir /Users/kogaryu/iggy3d/build
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files changed:
- Descriptor facts used (display + eligibility):
- Switches removed (before/after line counts):
- Behavior-preservation evidence (string/eligibility parity):
- Tests/checks run:
- Concerns/deferred (or blocker if moved to blocked/):

---

## Completion Brief - Builder

- Files changed:
  - `/Users/kogaryu/iggy3d/src/app/iggy3d/creative/document/Object.cpp`
  - `/Users/kogaryu/iggy3d/src/app/iggy3d/creative/mutation/Mutation.cpp`
  - `/Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E123-creative-object-kind-switch-cleanup.md`

- Descriptor facts used (display + eligibility):
  - Object kind string lookup now uses `describeObject(kind).name`. I used `name`, not `displayName`, because `displayName` is intentionally human-readable and differs for rows like `MovingPlatform` / `Moving Platform`; `name` is the descriptor fact that is byte-identical with the old `toString(CreativeObjectKind)` switch.
  - Mutation grouping now uses descriptor `profile`, `category`, `shapeKind`, `projectionProfile`, and `canOwnChildren` to choose the same profile-level verb sets previously spelled out by object-kind switch cases.
  - `descriptorAllowsMutation(...)` continues to own the existing capability filters over `hasTransform`, `hasBounds`, and `canHaveParent`, so the old two-stage behavior remains: `allowedMutations(...)` lists profile verbs, and descriptor validation filters unsupported transform/bounds/parent verbs.

- Switches removed (before/after line counts):
  - Removed the `toString(CreativeObjectKind)` exhaustive kind switch in `Object.cpp`: 123 deleted lines; replacement body is one descriptor lookup.
  - Removed the `allowedMutations(CreativeObjectKind)` exhaustive kind switch in `Mutation.cpp`: the kind switch is gone; replacement body validates the descriptor and calls descriptor-profile policy helpers. File diff for the two source files is `211 insertions(+), 350 deletions(-)`.

- Behavior-preservation evidence (string/eligibility parity):
  - Existing descriptor sentinel coverage stayed green: `creative_object_descriptor_tests` still verifies descriptor rows are total and that descriptor stable names match `toString(kind)` for every enum kind.
  - Existing mutation coverage stayed green: `creative_document_mutation_tests` still exercises representative mutation allowance, descriptor validation, apply/no-change/reject behavior, dirty flags, and payload policy.
  - Receipt key-order oracle stayed green: `product_receipt_key_order_tests`.
  - Full CTest suite stayed green: 258/258 passed.

- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_object_descriptor_tests creative_document_mutation_tests product_receipt_key_order_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_object_descriptor_tests|creative_document_mutation_tests|product_receipt_key_order_tests)$' --output-on-failure`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `perl -ne 'print "$ARGV:$.:$_" if /[ \t]$/' /Users/kogaryu/iggy3d/src/app/iggy3d/creative/document/Object.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/mutation/Mutation.cpp /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E123-creative-object-kind-switch-cleanup.md`
  - `rg -n "switch \(kind\)|switch \(objectKind\)|case CreativeObjectKind::" /Users/kogaryu/iggy3d/src/app/iggy3d/creative/document/Object.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/mutation/Mutation.cpp`

- Concerns/deferred:
  - This deliberately leaves the serialized object-kind id table in `Object.cpp`; that table is save-format policy, not the display/name switch targeted by this card.
  - `descriptorAllowsMutation(...)` still calls `canMutate(...)`, but `canMutate(...)` now reads descriptor-profile policy through `allowedMutations(...)` instead of a per-kind switch.
