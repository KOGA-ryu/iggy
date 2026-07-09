# K4a - Dependency Graph Tool Kernel

## Status

READY. K4 batch step 1 of 6. Authority:
docs/creative_mode/builder_tasks/blocked/K4-department-dependency-dag-plan.md.

On green completion, commit this card, move it to `done/`, and continue to K4b
without waiting for Reviewer. A STOP pauses the whole K4 batch.

## Goal

Add the complete reusable dependency-graph scanner and synthetic enforcement
suite. It must derive reports and support policy checking, but this card must
not add the live policy manifest or enforce current repository policy.

## Scope

Allowed files only:

- add tools/dependency_graph.py
- add tests/tools/dependency_graph_tests.py
- update cmake/iggy3d_tests.cmake
- update CMakeLists.txt only if standard CMake Python 3 interpreter discovery
  cannot live in cmake/iggy3d_tests.cmake

No production C++ include changes, architecture policy JSON, architecture docs,
or app/window work.

## Required Tool Contract

Implement:

    python3 tools/dependency_graph.py --repo-root PATH
      [--policy PATH] [--check-policy] [--format text|json] [--output PATH]

- --repo-root is required and must contain src; text is the default format.
- Exit 0: reliable report or passing policy check. Exit 2: CLI or policy schema
  error. Exit 3: malformed/unresolved project include or unclassified
  production source/target. Exit 4: policy violation.
- Scan only .c, .cc, .cpp, .cxx, .h, .hh, .hpp, .hxx below src; exclude
  build/generated/vendor/third_party/tests/docs/tools/apps.
- Classify seven physical departments by longest prefix: core, config, content,
  runtime, projection, render, app. Creative remains part of app for K4.
- Parse quote and angle includes, ignore line/block comments, resolve
  source-relative quoted includes and src-root includes, and ignore non-project
  standard/third-party includes.
- Deduplicate by resolved (source,target), retaining a stable duplicate
  diagnostic. Normalize paths as repo-relative POSIX paths.
- Deterministic JSON keys, in order: schema, scan, departments,
  cross_department_edges, directed_pairs, fan_out, fan_in,
  transitive_closure, strongly_connected_components, diagnostics, policy.
- Implement generic policy checking for ordered departments/prefixes, allowed
  pairs, restricted target headers, ratchet ceilings, and exact exceptions.
  Report mode needs no policy and a reliable cycle still exits 0.

## Synthetic Test Contract

The Python suite must cover quote/angle and relative resolution, comment
exclusion, duplicate diagnostics, malformed/unresolved and unclassified exit 3,
graph counts/fan/closure/SCC facts, deterministic JSON, CLI and malformed-policy
exit 2, and forbidden/restricted/cycle/exception/ratchet exit 4. It must not pin
live repository counts.

Register dependency_graph_tool_tests with CMake-discovered Python 3,
repository-root working directory, and labels
architecture;dependency;oracle;iggy3d. Do not add dependency_direction_tests yet.

## Baseline Gate

The finished report must reproduce the physical seven-department baseline:

- 328 cross-department edges
- 18 directed pairs
- 16 unordered pairs
- SCCs only content/runtime and projection/render

The full pair and fan tables must match the parent K4 plan exactly.

## Verification

    python3 tests/tools/dependency_graph_tests.py
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --format text
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --format json
    cmake -S . -B build
    ctest --test-dir build -R '^dependency_graph_tool_tests$' --output-on-failure
    git diff --check

## Stop Conditions

- The live report differs from 328/18/16 or the two stated SCCs.
- A live policy manifest, production include migration, or separate Creative
  department is required to implement the generic tool.
- The parser contract cannot be implemented deterministically with the stated
  standard-library boundary.

## Completion Brief

Append changed files, JSON/text baseline output, policy-capability coverage,
synthetic and CTest results, and any stop evidence. On success, commit and
continue to K4b; do not request per-slice review.
