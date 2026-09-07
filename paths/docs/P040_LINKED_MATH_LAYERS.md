# P040: linked function, matrix and surface layers

This implements the first three priorities authorised after P039: a function
graph laboratory, expanded matrix transformations and a surface/contour
laboratory. The native math lab now has seven objects. The other proposed
object families and extensions remain future work.

The user's no-images instruction is binding for this work. Do not capture,
render or view images, and do not open app windows or browser previews for
verification. Build the native target and use pure tests or `--validate`.

## Available layers

| Object | 0 | 1 | 2 | 3 |
| --- | --- | --- | --- | --- |
| Functions | Inputs and roots | Secants and derivative limits | Signed accumulation and bound reversal | Taylor approximation |
| Matrices | Volume and rank | Full A, composition and eigenvector probes | Column-span projection and least squares | Singular value decomposition |
| Surfaces | Height and contours | Partial derivatives and tangent planes | Gradient descent and Hessian measurements | Stationary points on a unit circle |

Function choices are x squared, x cubed minus 3x, sine, and exponential minus
one. The plots of f, its derivative, and the integral from a share the input
marker. At h=0, the secant readout uses the derivative limit; the secant
challenge requires a nonzero step. Bound reversal is an explicit action, and
its challenge requires reversing a negative integral. Taylor degrees 0–5 use
analytic derivatives and show the pointwise approximation error.

The matrix editor owns one 3x3 A. Its A[0,1] and A[1,1] entries are the original
shear and scale controls. Presets include identity, shear, xy projection,
stretch/reflection and a 90-degree rotation about z. Composition compares BA
and AB while showing v, Av and BAv. The eigenvector probe checks the residual
of Av minus lambda v and excludes zero v and zero Av. Projection uses the span
of the first two columns, including dependent or zero columns. SVD shows a
labelled, coloured sphere through V transpose, Sigma and U, alongside singular
values and a reconstruction error. Orthogonal factors may include reflections.

The three surfaces are a bowl, a saddle and sin(u)cos(v). World coordinates
are (u, height, v). The contour map and two section plots share the probe.
Partial derivatives define the tangent plane and directional derivative.
Descent takes a decreasing step using at most twelve backtracking trials,
restricted to the displayed domain. At a boundary or stationary point, failure
to find a decreasing step leaves the state unchanged. The final layer restricts
the point to the unit circle and reports its tangential derivative, Lagrange
multiplier and constraint error.

## Ownership and limits

`MathObjects::dispatch` remains the sole mathematical action owner. It validates
object, level, availability, finite values and ranges before applying input.
Returning to a lower layer restores hidden advanced parameters to defaults.
Diagram input enters that dispatcher before the following frame is drawn,
so the geometry, equations, measurements and diagrams share one revision.

The model publishes at most 192 primitive placements, three plots of three
129-point series, three 3x3 matrices, one 441-vertex surface patch and 2,048
contour segments. The scene adapter retains the existing 8,192-vertex and
65,536-index renderer capacities. It creates no Vulkan resources.

Readouts and challenge formulas are analytic except the bounded numerical SVD.
The surface uses a 21x21 grid, contours interpolate its triangles, and curves
use finite samples. The SVD uses 24 Jacobi sweeps on A transpose A, descending
singular values, a 1e-7 zero threshold and deterministic completion of null
columns. Matrix entry ranges and quantisation bound the conditioning envelope;
this is an educational laboratory, not a general numerical linear algebra API.
Challenge checks use their stated numerical tolerances and do not constitute
formal proofs. No new parent or third-party code was copied.

## Build and text-only checks

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b -t math_lab paths_math_layers_tests paths_math_object_tests paths_scene_tests -j4
ctest --test-dir b -R '^paths_math_(function|matrix|surface)_cli$' --output-on-failure

cmake -S . -B b-math-headless -DCMAKE_BUILD_TYPE=Release -DPATHS_BUILD_NATIVE=OFF
cmake --build b-math-headless -t paths_math_layers_tests paths_math_object_tests paths_scene_tests -j4
ctest --test-dir b-math-headless -R '^(paths_math_layers_tests|paths_math_object_tests|paths_scene_tests)$' --output-on-failure
```

`math_lab --validate` exits before constructing the native host. It applies the
same semantic CLI actions, computes CPU geometry and prints measurements. It
rejects capture requests. Select the object and level before its parameters;
`--help` lists keys, ranges and matrix presets. For example:

```sh
./b/math_lab --validate --object function --level 3 --set function=2 --set at=0.5 --set degree=5 --check
./b/math_lab --validate --object linear --level 3 --preset 2 --check
./b/math_lab --validate --object surface --level 3 --set surface=1 --set constraint=1 --set circle_angle=0 --check
```

## Verification

The Release native build and all six targeted checks pass on 2026-09-07.
The fresh native-disabled build passes the new layers test, the original
object regression test and the shared scene test. Its layers test links only
the system C++ and system libraries. The three native CLI tests complete their
advanced challenges using `--validate` without constructing a native host.

The new test covers 524 geometry states, independent derivative/quadrature
checks, Taylor exactness and convergence, linked markers, contour interpolation,
decreasing descent, circle constraints, noncommuting matrices, degenerate
projections and 48 matrix SVD cases with independent reconstruction and
orthogonality checks. New-layer maxima are 4,685 vertices, 14,688 indices and
290 contour segments. The original object regression reaches 7,179 vertices
and 22,800 indices, also within the existing limits. Warning-enabled C++ syntax
checks pass for the changed production code.

No images were taken, generated, rendered or viewed. Visual layout, human
pointer feel and swapchain acceptance remain unverified. All changes are
uncommitted, and existing sorter, study progress and scored-question owners
are preserved.
