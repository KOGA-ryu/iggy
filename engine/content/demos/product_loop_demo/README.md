# Product Loop Demo

This package is the first product-facing authored content proof for the current
runtime loop. It uses the same explicit package contract as test fixtures, but
lives under `engine/content/demos` so it is content first and test input second.

It proves:

- package manifest loading through `package.toml`;
- ASCII source-plan parsing and profile scenario conversion;
- target discovery for a nearby pickup;
- inventory mutation from picking up `item:key`;
- save/load and reset behavior through the acceptance test;
- required-item interaction by opening `target:door` only after the key exists;
- embedded `[expect]` facts for repeatable CLI checks.

Run the package check from the repository root:

```sh
engine/build/iggy_scenario_toml_runner --check engine/content/demos/product_loop_demo
```

Inspect the frame trace:

```sh
engine/build/iggy_scenario_toml_runner --trace engine/content/demos/product_loop_demo
```

Launch it in native play:

```sh
engine/build/iggy_native_play --play engine/content/demos/product_loop_demo
```
