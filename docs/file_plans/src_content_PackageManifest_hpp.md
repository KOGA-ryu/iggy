# `src/content/PackageManifest.hpp`

Updated: 2026-06-20

Exact purpose: declare the validated in-memory package manifest model for
`package.iggy3d.toml`.

## Build Position

- priority rank: 32
- tier: Tier 2: Content And Configuration
- module: `content`
- file kind: `header`

## Required Header Shape

Repo path:

```text
src/content/PackageManifest.hpp
```

Required includes:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
```

Required namespace:

```cpp
namespace iggy3d {
}
```

## Required Types

Declare:

```cpp
struct PackageAssetRef {
  std::string id;
  std::string path;
};

struct PackageManifest {
  std::string packageId;
  std::uint32_t schemaVersion = 1;
  std::uint32_t requiredRuntimeSchema = 1;
  std::string scenarioPath;
  std::vector<PackageAssetRef> assets;
};
```

## Field Semantics

- `packageId`: stable package id. First-room value: `iggy3d.first_room`.
- `schemaVersion`: package manifest schema. First build requires `1`.
- `requiredRuntimeSchema`: minimum runtime schema. First build requires `1`.
- `scenarioPath`: relative scenario path. First-room value:
  `scenario.iggy3d.toml`.
- `assets`: inert ids/paths only. The first-room fixture uses an empty vector.

## TOML Schema

`package.iggy3d.toml` must use:

```toml
[package]
id = "iggy3d.first_room"
schema_version = 1
required_runtime_schema = 1
scenario = "scenario.iggy3d.toml"

[[assets]]
id = "debug_marker"
path = "relative/path"
```

The `assets` table is supported by the parser but absent from the first-room
fixture. Absolute paths and paths containing `..` are invalid.

## Dependencies And Ownership

This header owns manifest values only. It does not parse files, validate
scenario contents, create sessions, allocate entities, load renderer assets, or
import old iggy schemas.

## Save Replay Multiplayer

Package id and schema fields become session identity/save compatibility facts.
Asset refs are content facts only until a renderer/content backend consumes
them.

## Completion Criteria

The header locks the manifest fields and TOML key names used by package loader
and validator.
