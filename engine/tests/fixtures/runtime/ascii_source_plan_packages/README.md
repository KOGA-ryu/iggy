# Authored Scenario Package Fixtures

These fixtures exercise the explicit package runner path. A package fixture is
a directory with `package.toml`, `scenario.toml`, and optional human notes. The
package manifest is metadata plus a single `main` TOML file; gameplay facts
remain in `scenario.toml`.

Run a package fixture from a configured build with:

```sh
engine/build/iggy_scenario_toml_runner engine/tests/fixtures/runtime/ascii_source_plan_packages/moving_guard_room_package
```

Engine callers that need data instead of CLI text can use
`RuntimeGameplayAuthoringPreviewModel` over the same explicit package path. The
preview model delegates to the package facade, preserves package metadata, and
copies the delegated scenario summary, final rows, trace frames, diagnostics,
and expectation comparison.

Checked-in package fixtures:

- `moving_guard_room_package`: positive package for basic NPC movement.
- `player_picks_up_item_package`: positive package for player movement plus
  item pickup.
- `bad_pickup_target_package`: negative package with a valid manifest and an
  invalid delegated source-plan pickup target.

Package fixtures are not a runtime package registry. Tests and tools pass an
explicit package directory or `package.toml` path; there is no recursive
discovery or dependency resolution.
