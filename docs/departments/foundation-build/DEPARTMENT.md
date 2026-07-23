# Foundation and Build

## Purpose

Own low-level, cross-product foundations and the mechanisms that keep department
ownership, compilation, and testing enforceable.

## Owns

- Core math, IDs, grids, hashes, results, and spatial primitives.
- Platform and frontend foundations not owned by semantic input.
- CMake targets, warnings, shaders, third-party declarations, and build options.
- Repository-level validation and department-map tooling.
- Architectural dependency policy.

## Does Not Own

- Product capabilities merely because they are shared.
- Domain-specific UI, rendering policy, or runtime behavior.
- Work status for other departments.

## Dependency Direction

Foundation is at the bottom of the dependency graph. It must not depend on
Creative product departments. Build tooling may inspect all departments but may
not become a runtime dependency.

## Primary Owners

- `src/core/`, `src/config/`, and platform foundations
- `CMakeLists.txt`, `cmake/`, and `third_party/`
- Repository tools not owned by the asset pipeline
- `docs/departments/` and `tools/repo_departments.py`

See [FILES.md](FILES.md) for the complete generated assignment.
