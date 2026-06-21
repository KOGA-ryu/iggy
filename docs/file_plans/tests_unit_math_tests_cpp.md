# `tests/unit/math_tests.cpp`

Updated: 2026-06-20

Exact purpose: prove core math primitives behavior for the complete `iggy3d` runtime.

## Build Position

- priority rank: 26
- tier: Tier 1: Core Contracts
- module: `unit tests`
- file kind: `test`

## Ownership

This file owns:

- test scenarios
- assertions
- fixture setup helpers local to this test file
- regression coverage for documented semantics

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- public iggy3d headers
- test framework chosen by `cmake/iggy3d_tests.cmake`
- fixtures under `/Users/kogaryu/iggy3d/fixtures`

## Data Contract

- Vec3 arithmetic and finite checks
- Aabb3 closest point/intersection
- Ray3 construction
- Plane distance/intersection
- Mat4 identity/transform
- Transform3 defaults

## Semantics

- tests must be deterministic
- tests must not require renderer or old iggy code
- tests should assert exact rejection/status codes when behavior is part of runtime contract

## Implementation Plan

1. include the test framework and only public `iggy3d` headers required for the scenario;
2. build test state through public APIs or fixture loaders;
3. assert the exact success, failure, rejection, and deterministic replay behavior named in this document;
4. keep the test independent of renderer, network services, wall-clock timing, and old `iggy` code.

## Compute Cost

- Test runtime should stay small; fixture-level tests may scan all demo entities and commands.
- Core math tests use small fixed values and no fixture loading.

## Diagnostics And Errors

- tests assert boolean validation helpers and exact numeric behavior;
- core math tests do not inspect app/content/runtime diagnostics;
- assertion names must identify the math primitive and failing contract.

## Save Replay Multiplayer Notes

- this test file owns no save truth;
- it proves value semantics that runtime save/replay/hash owners depend on;
- it must not create sessions, command logs, save envelopes, or multiplayer
  state.

## Tests And Verification

- register executable `math_tests` with labels `unit;core;iggy3d`;
- tests must be deterministic and use only public core headers;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `tests/unit/math_tests.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Purpose
Implement unit tests for all core math and stable hash primitives that runtime authority depends on.

### Required Test Cases
- `Vec3` default/zero/unit construction, add/subtract/scale, dot, distance, finite validation.
- `Vec3` stores `float x/y/z`; `sizeof(Vec3) == sizeof(float) * 3` if no
  padding is introduced by the compiler; tests assert field type through
  compile-time assignment and exact default values.
- `Transform3` identity defaults: `position=(0,0,0)`,
  `rotationEulerRadians=(0,0,0)` with `x=pitch`, `y=yaw`, `z=roll`, and
  `scale=(1,1,1)`.
- `Aabb3` valid/invalid bounds, finite rejection, inclusive contains, inclusive
  intersection including touching faces, invalid-input intersection returning
  false, and closest point clamping to min/max.
- `Ray3` validity and `pointAt(ray, 2.0F) == origin + direction * 2.0F`.
- `Plane` signed distance, invalid normal rejection, ray hit, and parallel
  ray/no-hit behavior.
- `Mat4` row-major identity, `at(row,column)`, translation, multiplication
  order where `A * B` applies `B` then `A`, and transform point homogeneous
  policy.
- `StableHasher` uses 64-bit FNV-1a constants `14695981039346656037` and
  `1099511628211`, produces deterministic repeated hashes, preserves ordered
  input differences, and documents quantized float rounding.

### Constraints
- No renderer, app, filesystem, randomness, clock, network, or old iggy dependencies.
- Tests should be plain C++ executable assertions consistent with the rest of the repo plan.

### Completion Criteria
The test executable is registered as `math_tests`, labeled `unit;core;iggy3d`, and fails on any non-finite value acceptance where runtime state would later depend on it.
