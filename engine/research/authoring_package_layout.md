# Authored Scenario Package Layout

This is the proposed package boundary for authored TOML scenarios. It is a
developer/content organization shape around the existing one-file
ASCII source-plan TOML path, not a second scenario model.

## Minimal Shape

```text
example_package/
  package.toml
  scenario.toml
  README.md
```

`scenario.toml` remains a normal self-contained ASCII source-plan TOML file.
It must continue to run through the existing path:

```text
RuntimeGameplayTomlScenarioFacade -> TOML file reader -> authoring adapter ->
profile scenario runner -> summary/final-row projection
```

`package.toml` is package metadata and indirection only. The package runner
reads this file from one explicit package path and then delegates to the
existing facade using the declared main scenario file.

Initial package manifest shape:

```toml
format_id = "iggy:authored-scenario-package"
version = 1
main = "scenario.toml"
```

Run an explicit package directory from a configured build with:

```sh
engine/build/iggy_scenario_toml_runner engine/tests/fixtures/runtime/ascii_source_plan_packages/moving_guard_room_package
```

Passing the manifest file directly is also explicit:

```sh
engine/build/iggy_scenario_toml_runner engine/tests/fixtures/runtime/ascii_source_plan_packages/moving_guard_room_package/package.toml
```

## Package Rules

- The package path must be explicit. There is no discovery, registry, or
  recursive scan.
- `main` names exactly one TOML scenario file inside the package directory.
- `main` must be relative, must not be empty, and must not escape the package
  directory.
- The main TOML file owns gameplay authoring facts, expectations, and source
  plan version policy.
- `README.md` is documentation only.
- Optional expected-output text files may be useful later for humans, but they
  must not replace embedded `[expect]` source-plan facts unless a future packet
  explicitly changes that contract.

## Non-Goals

- Multiple scenarios per package.
- Package dependencies or imports.
- Asset catalogs, tilesets, or external resource resolution.
- Save/load state.
- UI/Edi ownership.
- Runtime autorun or game-loop integration.
- A new TOML parser or broader TOML compliance claim.
- New gameplay semantics.

## Implementation Boundary

- Package reading belongs in a narrow package facade/runner layer, not in the
  source-plan parser or profile converter;
- successful package execution delegates to `RuntimeGameplayTomlScenarioFacade`
  for the main TOML file;
- CLI output and facade result shapes stay aligned with one-file execution;
- package metadata is display/read-only and cannot alter gameplay execution.
