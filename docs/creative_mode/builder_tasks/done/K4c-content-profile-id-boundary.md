# K4c - Content-Owned NPC Profile Identifier Boundary

## Status

READY. K4 batch step 3 of 6. Claim only after K4b is committed and in `done/`.
Authority:
docs/creative_mode/builder_tasks/blocked/K4-department-dependency-dag-plan.md.

On green completion, commit this card, move it to `done/`, and continue to K4d1
without waiting for Reviewer. A STOP pauses the whole K4 batch.

## Goal

Move the authored NPC profile-id value and validation predicate from runtime to
content. This removes the two content implementation validation edges while
leaving scenario seed types and runtime conversion untouched for K4d1/K4d2.

## Scope

Allowed files:

- add src/content/NpcBehaviorProfileId.hpp
- update src/runtime/ai/NpcBehaviorProfile.hpp
- update src/runtime/ai/NpcBehaviorProfile.cpp
- update src/content/FixtureScenarioLoader.cpp
- update src/content/PackageValidator.cpp
- update src/app/iggy3d/world/NpcProfileAssignment.cpp
- update focused profile/package/assignment tests only as needed

No ScenarioSeed.hpp, scenario conversion, Session/RoomMarker migration, policy
JSON, or save/hash/replay work.

## Required Ownership And Behavior

- content/NpcBehaviorProfileId.hpp owns the existing public
  NpcBehaviorProfileId type and isValidNpcBehaviorProfileId predicate.
- Define the header-owned predicate as `inline bool`; do not leave a non-inline
  external definition in a multiply included header and do not add a new `.cpp`.
- Preserve the predicate exactly: non-empty id; every byte is accepted only by
  the current lowercase/digit/underscore rule. Do not change locale or accepted
  character semantics.
- runtime/ai/NpcBehaviorProfile.hpp includes the content header and no longer
  declares the type or predicate. NpcBehaviorProfile.cpp no longer implements
  the predicate.
- FixtureScenarioLoader.cpp, PackageValidator.cpp, and
  NpcProfileAssignment.cpp include the content header directly.
- Runtime profile resolution and all existing public symbol names stay intact.

## Required Graph Result

Both content validation implementation edges to runtime/ai/NpcBehaviorProfile.hpp
are gone. The six runtime state includes in FixtureScenarioLoader.hpp remain for
K4d1/K4d2; do not attempt seed-type migration here.

After this card, the graph must be exactly 327 edges, 17 directed pairs, and 16
unordered pairs, with content/runtime as the only SCC. Required pair rows:

- content -> runtime = 6
- runtime -> content = 8
- app -> content = 27
- app -> runtime = 101

## Verification

    cmake --build build --target package_loader_tests npc_behavior_profile_tests product_npc_profile_assignment_tests
    ctest --test-dir build -R '^(package_loader_tests|npc_behavior_profile_tests|product_npc_profile_assignment_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --format json
    rg -n '#include[[:space:]]+[<"]runtime/ai/NpcBehaviorProfile.hpp' src/content
    git diff --check

The grep must show no validation `.cpp` dependency. The six other runtime state
includes in FixtureScenarioLoader.hpp are outside this grep and remain for
K4d1/K4d2.

## Stop Conditions

- Profile-id validation behavior or public profile resolution changes.
- The implementation needs scenario enum/type conversion, Session changes, or
  save/hash/replay edits.
- A generic identifier cleanup expands the boundary beyond this one value type.

## Completion Brief

Append the new content owner, exact validation parity evidence, remaining
content/runtime edges reserved for K4d1/K4d2, focused tests, and graph output.
On success, commit and continue to K4d1.
