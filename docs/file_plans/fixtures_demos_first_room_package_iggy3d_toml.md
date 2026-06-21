# `fixtures/demos/first_room/package.iggy3d.toml`

Updated: 2026-06-20

Exact purpose: define the first-room package manifest fixture consumed by
`iggy3d_validate_package`, `iggy3d_headless_demo`, package loader tests, and
acceptance tests.

## Build Position

- priority rank: 37
- tier: Tier 2: Content And Configuration
- module: `fixtures`
- file kind: `fixture`

## Exact Repo Path

```text
fixtures/demos/first_room/package.iggy3d.toml
```

## Required TOML Content

The fixture file must contain exactly these required package facts:

```toml
[package]
id = "iggy3d.first_room"
schema_version = 1
required_runtime_schema = 1
scenario = "scenario.iggy3d.toml"
```

The first complete build fixture contains no `[[assets]]` tables. Asset-table
parser support is documented in `PackageManifest.hpp`, but this fixture must
remain exactly the package table above for acceptance tests.

Fixture edits that add assets must update package-loader tests and acceptance
facts in the same patch.

## Schema Rules

- `package.id` must be `iggy3d.first_room`.
- `schema_version` must be integer `1`.
- `required_runtime_schema` must be integer `1`.
- `scenario` must be the relative path `scenario.iggy3d.toml`.
- No absolute path is allowed.
- No path containing `..` is allowed.
- No old `/Users/kogaryu/iggy` path or dependency text is allowed.
- Unknown keys are validation/parse failures, not ignored metadata.

## Ownership

This fixture is seed data. It is loaded and validated by content tools. It is
not compiled into `iggy3d` and does not own runtime state after session
creation.

## Tests

`tests/unit/package_loader_tests.cpp` must load this file successfully and
assert all required fields.

## Completion Criteria

The fixture gives package identity and scenario path for the first-room demo
without renderer or old-iggy dependencies.
