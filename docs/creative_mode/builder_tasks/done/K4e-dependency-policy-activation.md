# K4e - Dependency Policy Activation

## Status

READY. K4 batch step 6 of 6. Claim only after K4d2 is committed and in `done/`,
and Builder's K4d2 report independently shows the 320/16/16, zero-SCC graph.
Authority:
docs/creative_mode/builder_tasks/blocked/K4-department-dependency-dag-plan.md.

This is the batch closeout. Reviewer receives one aggregate K4 code-review
brief only after this card is committed, unless an earlier card STOPs.

## Goal

Publish the machine policy and activate live repository enforcement using the
already-landed K4 tool. This is the only K4 implementation card permitted to
edit architecture policy documentation.

## Scope

Allowed files:

- add docs/architecture_dependency_policy.json
- update docs/architecture.md dependency section only
- update cmake/iggy3d_tests.cmake
- update CMakeLists.txt only if required by existing Python discovery wiring

No production C++ include change, no exception-based cycle preservation, and no
unrelated documentation edits. The graph tool and its synthetic tests are
frozen in this card.

## Required Policy

The JSON encodes the seven department prefixes, recognized suffixes, allowed
include-direction adjacency, exact restricted target-header allowlists, and final
ratchet ceilings from the parent K4 plan. allowed_exceptions is an empty array.

Allowed directions:

- core: none
- config -> core
- content -> config|core
- runtime -> content|config|core
- projection -> runtime|content|core
- render -> projection|core
- app -> render|projection|runtime|content|config|core

Restrict runtime -> content to RoomAsset.hpp, TraversalTag.hpp, ScenarioSeed.hpp,
and NpcBehaviorProfileId.hpp. Restrict projection -> content to RoomAsset.hpp.
These are exact target policies, not wildcard exceptions.

Register dependency_direction_tests, invoking the live repository tool with
--check-policy, CMake-discovered Python, repository-root working directory, and
labels architecture;dependency;oracle;iggy3d.

## Required Final Gate

    python3 tests/tools/dependency_graph_tests.py
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json
    cmake -S . -B build
    ctest --test-dir build -R '^(dependency_graph_tool_tests|dependency_direction_tests)$' --output-on-failure
    rg -n '#include[[:space:]]+[<"]runtime/' src/content
    rg -n '#include[[:space:]]+[<"]render/' src/projection
    git diff --check

The live policy report must be exactly 320 edges, 16 directed pairs, 16 unordered
pairs, zero SCCs, zero violations, and zero exceptions. Both greps return no
match.

## Stop Conditions

- The final graph differs from 320/16/16, has an SCC, or needs a non-empty
  exception list.
- Policy activation discovers a production boundary change; stop for Reviewer
  and do not change C++ in this card.
- The manifest requires wildcard exemptions or changes source classification.
- The graph tool or its synthetic tests need repair. STOP and create a separate
  repair card; do not modify the verifier while activating its policy.

## Completion Brief

Append manifest facts, architecture-doc rule, exact live policy output, both
dependency CTest results, and zero-exception evidence. Commit the card, then
send Reviewer one aggregate K4 brief listing all six commits and request the
milestone code review.
