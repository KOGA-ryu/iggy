# P039: native interactive maths objects

The user requested 3D objects across algebra, trigonometry, calculus, linear
algebra and discrete maths, with a school-to-introductory-university progression,
and identified `/Users/kogaryu/iggy3d/paths` as the C++ Vulkan destination.

The `math_lab` startup presents five parameterised objects using the existing
native renderer. The reusable `paths_math_objects` library contains the pure
mathematical owner and its SceneFrame adapter. This is one object-exploration
capability; it does not alter scored questions or saved study progress.

| Object | Controls | Observable mathematical fact |
| --- | --- | --- |
| Algebra expansion cube | x, separation | Eight component volumes sum to `(x+1)^3` |
| Unit circle and wave | angle | Signed sine/cosine, radians and unit-radius identity |
| Integration disks | count, sampling rule, separation | Disk sums converge to `8*pi/3` |
| Transforming cube | shear k, vertical scale s | `det A=s`, volume `abs(s)`, rank 2 at s=0 |
| Binary graph | layer spacing, shortcut, next vertex | Unweighted shortest paths and bit-flip adjacency |

`MathObjects::dispatch` is the sole parameter/route/challenge owner. Invalid,
nonfinite and wrong-subject requests are rejected without changing its revision
or state. A snapshot holds at most 192 primitive placements, 16 labels, six
metrics and a 64-vertex route. A deterministic eight-vertex BFS supplies graph
distances. UI widgets and CLI options forward semantic actions to this owner.

`MathObjectScene` uses cached unit primitives and reserved buffers to produce
the existing 32-byte `SceneVertex` layout and 16-bit triangle indices. It checks
the existing 8,192-vertex / 65,536-index GPU capacities before appending geometry.
It owns no Vulkan resources. It rebuilds geometry only when the model revision
changes; camera-only movement reuses it. Both gallery and mathematical scenes
call `publishSceneCamera`, the former gallery camera function now shared, and
use the existing camera-navigation kernel and native host.

The supplied mathematical geometry and UI are original work for this request.
They adapt the five concepts already prototyped in `maths-object-lab` in the
conversation's visualization output. Existing attributed camera, vector,
matrix, Vulkan, SDL and ImGui code is reused in place; no new third-party code
or parent-repository source is copied into Paths.

## Build and run

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b -t math_lab paths_math_object_tests paths_scene_tests -j4
ctest --test-dir b -R '^(paths_math_object_tests|paths_scene_tests)$' --output-on-failure
./b/math_lab
```

The pure library and tests are also available with `PATHS_BUILD_NATIVE=OFF`.
The native app uses only the SDL3/Vulkan dependencies already declared by Paths.
Orbit with left drag, pan with right drag, and zoom with the wheel. The sidebar
holds the parameter controls, measurements, challenge and learning progression.

For a bounded offscreen run:

```sh
./b/math_lab --offscreen --object linear --set scale=0 --check \
  --capture /private/tmp/paths-math-objects/linear-flat.png
./b/math_lab --offscreen --object discrete --route BDH --check
```

`--object` accepts `algebra`, `trig`, `calculus`, `linear` and `discrete`.
`--set key=value` follows the selected object's control schema. Disk sampling
is 0=left, 1=midpoint, 2=right. `--route` contains moves after the initial A.
The CLI uses the same semantic model route as the controls. Captures use the
existing native capture implementation. Offscreen mode creates no visible window.

## Representation limits

The native mesh shader uses vertex colour, so the linear map is shown as a
coloured cage and lattice with basis arrows. Its analytical volume and rank
come from the mathematical model. Negative determinants do not become negative
geometric volume. Sphere/rod thickness is a presentation aid. The calculus
readout compares analytic disk volumes with the integral, independent of its
24-sided rendered circles. Separating blocks/disks changes placement only.
The optional A-H shortcut changes the graph; moving its layers does not.

## Verification

Implementation is ready for the focused pure-model, scene and native offscreen
checks. Interactive pointer feel and swapchain acceptance require a user test;
offscreen evidence must retain that distinction. Changes remain uncommitted.

References: [Khronos vertex input](https://docs.vulkan.org/guide/latest/vertex_input_data_processing.html),
[OpenStax unit circle](https://openstax.org/books/precalculus-2e/pages/5-2-unit-circle-sine-and-cosine-functions),
[OpenStax volume by slicing](https://openstax.org/books/calculus-volume-1/pages/6-2-determining-volumes-by-slicing).
