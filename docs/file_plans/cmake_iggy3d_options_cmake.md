# `cmake/iggy3d_options.cmake`

Updated: 2026-06-20

Exact purpose: declare build options for tests, warnings, and tools.

## Build Position

- priority rank: 6
- tier: Tier 0: Repo Contract And Build Shell
- module: `build system`
- file kind: `build`

## Ownership

This file owns:

- `IGGY3D_BUILD_TESTS`
- `IGGY3D_BUILD_TOOLS`
- `IGGY3D_WARNINGS_AS_ERRORS`

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- CMake only
- owned source/test paths in `/Users/kogaryu/iggy3d`
- no old iggy targets or include directories

## Data Contract

- defaults favor internal demo development: tests and tools on in dev builds
- renderer option is absent or off until runtime acceptance is green

## Semantics

- options do not import old iggy targets
- option names use `IGGY3D_` prefix only

## Implementation Plan

1. create target/config behavior explicitly;
2. fail early when required owned files are missing;
3. keep old iggy paths out of include/link lists;
4. make test/tool options visible and documented.

## Compute Cost

- O(3) cache option declarations at configure time.

## Diagnostics And Errors

- option declaration does not fail for normal ON/OFF values;
- downstream CMake files fail when an enabled feature's required local files are
  missing;
- runtime `Diagnostic` types are not used by this CMake helper.

## Save Replay Multiplayer Notes

- build options are not save truth and are not replay input.
- disabling tests or tools must not alter the `iggy3d` library's runtime
  semantics.
- no option must introduce renderer, socket, multiplayer, or old iggy
  dependencies in the foundation build.

## Tests And Verification

- configure output must show the three option values;
- `IGGY3D_BUILD_TESTS=OFF` still configures and builds `iggy3d`;
- `IGGY3D_BUILD_TOOLS=OFF` still configures and builds `iggy3d`;
- `IGGY3D_WARNINGS_AS_ERRORS=ON` is consumed only by the warnings helper.

## Completion Criteria

- `cmake/iggy3d_options.cmake` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Options
Declare only project-owned options:
- `IGGY3D_BUILD_TESTS` default `ON`.
- `IGGY3D_BUILD_TOOLS` default `ON`.
- `IGGY3D_WARNINGS_AS_ERRORS` default `OFF` for local iteration.

Do not declare sanitizer, renderer, SDL, Vulkan, networking, install, package,
or old iggy compatibility options in the foundation pass.

### Semantics
- Options must be ordinary CMake cache options.
- Options must not inspect old repo paths.
- Disabling tests must not remove the `iggy3d` library target.
- Disabling tools must not affect unit-test targets except acceptance tests that intentionally execute app tools.

### Diagnostics
The helper prints exactly these configure status labels:

```text
IGGY3D_BUILD_TESTS=<ON|OFF>
IGGY3D_BUILD_TOOLS=<ON|OFF>
IGGY3D_WARNINGS_AS_ERRORS=<ON|OFF>
```

It must not hide failed tool/test registration behind status-only messages.

### Completion Criteria
`CMakeLists.txt` can include this file first, and all other CMake files can rely on the three option variables being defined.
