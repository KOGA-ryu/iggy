# File Spec

Files: `src/app/PackageRuntimeLookup.hpp`, `src/app/PackageRuntimeLookup.cpp`

Verified at: `96a43b54`

## Owns

- Runtime lookup for package/resource/shader/diagnostics roots.
- Package mode naming for headless, build-tree product, and installed product modes.
- Candidate ordering for overrides, environment variables, executable-relative paths, build-tree paths, bundle paths, and temporary diagnostics paths.
- Failure reason mapping for missing executable, resource root, shader root, diagnostics directory, and forbidden current-working-directory candidates.

## Does Not Own

- Executable path discovery.
- Renderer backend configuration or Vulkan resource loading.
- Product world template loading behavior after roots are resolved.
- Shader compilation, shader module creation, or render diagnostics formatting.
- Save, content package parsing, or asset validation.

## Reads

- `PackageLookupConfig` overrides and requirements.
- Environment variables for resource, shader, and diagnostics roots.
- Executable path and directory from `resolveExecutablePath()` when no executable override is provided.
- Filesystem directory, regular-file, current-working-directory, and temporary-directory facts.

## Writes / Mutates

- Returns `PackageLookupResult` and `PackageRuntimeLookup`.
- Creates diagnostics directories when configured to do so.
- Does not mutate app window, runtime state, renderer state, or content assets.

## Calls Out To / Wires Out To

- Calls `resolveExecutablePath()` from `ExecutablePath.*`.
- `RendererLifecycle.cpp` uses lookup output to configure shader root and diagnostics directory.
- `ProductWorldTemplateOperations.cpp` uses lookup output to locate product resources.
- Package lookup unit tests and shader lookup smoke exercise success and failure paths.

## Called By / Entry Points

- `resolvePackageRuntimeLookup(...)`.
- `packageModeName(...)`.
- Focused proof: `rg -n "PackageRuntimeLookup|resolvePackageRuntimeLookup|PackageLookupConfig|PackageLookupResult" src tests`.

## Invariants

- Explicit executable override must name an existing regular file.
- Current working directory is rejected as a chosen root for package resources or diagnostics.
- Headless mode may allow missing shader root unless shader or graphics runtime is required.
- Installed product and graphics-runtime requirements require a resource root.
- Override misses fail explicitly instead of falling through silently.
- Diagnostics directory writability is required for an overall successful lookup.

## Tests / Proof Commands

- `rg -n "package_runtime_lookup_tests|package_shader_lookup_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "resolvePackageRuntimeLookup|package_lookup_shader_root_missing|package_lookup_cwd_forbidden" tests/unit/package_runtime_lookup_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/platform/ExecutablePath.*` unless executable path discovery changes.
- `src/app/iggy3d/window/RendererLifecycle.*` unless renderer configuration consumption changes.
- `src/app/iggy3d/world/ProductWorldTemplateOperations.*` unless resource lookup consumption changes.
- `src/render/*` unless renderer config or diagnostics contracts change.

## Update When

- Package mode semantics, root candidate ordering, environment variable names, failure reasons, diagnostics directory policy, or lookup result fields change.

## Do Not Update When

- Only shader module loading, world template parsing, or executable path platform internals change without changing package runtime lookup behavior.
