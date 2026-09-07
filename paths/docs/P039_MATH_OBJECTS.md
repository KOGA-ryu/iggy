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

For text-only verification, configure a separate build with the native host
excluded. These two tests calculate geometry and camera matrices on the CPU;
they do not create a window, render frames, capture, or load images.

```sh
cmake -S . -B b-math-headless -DCMAKE_BUILD_TYPE=Release -DPATHS_BUILD_NATIVE=OFF
cmake --build b-math-headless -t paths_math_object_tests paths_scene_tests -j4
ctest --test-dir b-math-headless -R '^(paths_math_object_tests|paths_scene_tests)$' --output-on-failure
```

`--object` accepts `algebra`, `trig`, `calculus`, `linear` and `discrete`.
`--set key=value` follows the selected object's control schema. Disk sampling
is 0=left, 1=midpoint, 2=right. `--route` contains moves after the initial A.
The CLI uses the same semantic model route as the controls. `math_lab --help`
prints its options and exits before creating the native host.

The user's continuation instruction prohibits taking or viewing any images.
Do not run captures, image viewers, browser previews, or rendering as part of
this work. Native build verification compiles the app without launching it.

## Representation limits

The native mesh shader uses vertex colour, so the linear map is shown as a
coloured cage and lattice with basis arrows. Its analytical volume and rank
come from the mathematical model. Negative determinants do not become negative
geometric volume. Sphere/rod thickness is a presentation aid. The calculus
readout compares analytic disk volumes with the integral, independent of its
24-sided rendered circles. Separating blocks/disks changes placement only.
The optional A-H shortcut changes the graph; moving its layers does not.

## Verification

Build and text-only verification completed on 2026-09-07:

- The existing Release `math_lab` target builds successfully; its `--help`
  command exits successfully before native host creation.
- A fresh Release configuration with `PATHS_BUILD_NATIVE=OFF` builds and passes
  `paths_math_object_tests` and `paths_scene_tests` (2/2 tests).
- The maths test covers cube volume and winding, the unit-circle identity,
  disk-sum bounds and midpoint convergence, signed determinant and rank,
  shortest graph paths, invalid actions, mesh bounds and camera navigation.
- The tested parameter extremes use at most 7,179 of 8,192 vertices and 22,800
  of 65,536 indices. The pure test executable links only the system C++ and
  system libraries, with no SDL/Vulkan dependency.

The completion pass used source inspection and text-only commands. No images
were captured, generated, rendered or viewed, and no app window was opened.
Visual layout, interactive pointer feel and swapchain acceptance remain
unverified. Changes remain uncommitted; existing study/sorter work is preserved.

References: [Khronos vertex input](https://docs.vulkan.org/guide/latest/vertex_input_data_processing.html),
[OpenStax unit circle](https://openstax.org/books/precalculus-2e/pages/5-2-unit-circle-sine-and-cosine-functions),
[OpenStax volume by slicing](https://openstax.org/books/calculus-volume-1/pages/6-2-determining-volumes-by-slicing).
