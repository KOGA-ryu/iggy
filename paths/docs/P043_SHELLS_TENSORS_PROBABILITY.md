# P043: flux shells, tensor blocks and probability networks

The user authorized these three families from the Math Terms and Definitions
TOC. They bring the native math_lab to sixteen objects, retaining P039-P042.
Each has four working layers and a challenge per layer.

| Object | Level 0 | Level 1 | Level 2 | Level 3 |
| --- | --- | --- | --- | --- |
| Flux shells | Surface orientation and normal probe | Surface integrals | Divergence theorem | Stokes and oriented boundary circulation |
| Tensor blocks | Outer products | Three-index tensors | Euclidean contraction | Orthonormal coordinate changes |
| Probability network | Sampled random walks | Transition matrices | Distribution evolution | Stationarity and convergence |

## Mathematics and controls

Flux offers a sphere, box and open disk, with radius/half-width 0.5..1.5,
rotation about x, two normal orientations and four affine vector fields.
The disk centre is (0,0,0.4); closed shells are centred at zero. Gold arrows
show normals, orange arrows show the field, and teal/coral surface colours
indicate positive/negative flux density. The Stokes disk colours show curl
flux density instead. The boundary direction follows the normal's right-hand
orientation. Play or scrub boundary time in [0,1].

Integration resolution n=2..12 is independent of the display mesh. Midpoint
quadrature uses equal-area sphere bands, box face cells and disk annuli:
2*n*n samples on sphere/disk, 6*n*n on a box. Analytic surface flux is the
reference; relative error is only reported for nonzero reference flux. The twist field (-y,x,z) gives a nontrivial sphere convergence
example; its divergence is 1. The divergence layer only reports an enclosed
volume for closed surfaces, and reverses the comparison integral for inward
normals. Stokes compares an 8*n-panel Simpson line integral with independently
summed curl flux. Constant, radial, vortex and twist fields are smooth on R^3.

Tensor inputs are three real vectors with components -2..2 in steps of 0.1.
The first layer constructs A[i,j]=u[i]*v[j]. The next two construct
T[i,j,k]=u[i]*v[j]*w[k] as three labelled slices and contract
c[i]=sum_j T[i,j,j]. This contraction uses the Euclidean metric to pair vector
indices. It can vanish while the full tensor remains nonzero. The final layer
interprets u*v^T as a Euclidean linear map and changes an orthonormal coordinate
basis by A'=Q^T*A*Q, with Q a rotation about z. World vectors stay fixed;
component blocks, matrices and coordinate rows update. Trace and Frobenius norm
are invariant. Indices are displayed as 1..3; CLI index parameters are 0..2.

Blocks have signed heights with one shared scale shown numerically. Grey cells
are zero; gold marks the selected component or contraction diagonal. Separating
slices changes placement only. Values, matrices and plots occupy separate tabs
when present, and the subject selector becomes compact below 720 pixels high.

The probability network has three states and four row-stochastic presets:
lazy directed cycle, weighted mixing toward (0.5,0.3,0.2), periodic cycle, and
an absorbing chain ending at C. A retention parameter spans 0..1 except for
the fixed periodic cycle. Row vectors evolve as p(next)=p*P. The distribution
view uses 0..64 transitions and an initial mixture of a chosen state with the
uniform distribution. Coloured node volumes are proportional to probability;
p<1e-9 uses a grey placeholder, with the numeric value retained in the table.
Edge stroke widths distinguish transition weights; heads indicate direction.

Sampled walks are separate from distribution evolution. Next walk step uses
xorshift32 and records at most 64 transitions. Seeds 0..65535 are repeatable;
zero maps to the fixed nonzero seed 0x9e3779b9. Reset, object/level selection and
changes to rule, retention, start or seed clear walk evidence. The periodic
cycle has a stationary distribution without general convergence. Retention 1
produces the identity (outside the fixed periodic preset), so stationary
distributions are nonunique; the displayed pi is explicitly just one example.
Plots show integer transitions and total-variation distance to that example.

## Ownership and validation

The existing MathObjects model owns all formulas, actions and challenges.
UI code presents snapshots and forwards actions. No scene adapter, renderer,
GPU capacities, third-party sources, scored attempts or study saves changed.
The concurrent corpus-library CMake additions were preserved during integration.
Changes are uncommitted.

Six targeted CPU tests and twelve native --validate scenarios passed:
**18 checks**. The new suite checks analytic flux and Stokes, 729 outer products,
343 three-index tensors, 1,225 basis cases, closed-form Markov distributions,
seed replay, atomic rejections, all twelve challenges and 2,429 geometry states.
Observed maxima were 3,367 vertices and 10,560 indices, within existing limits.
The wider geometry sweep caught tiny probability spheres becoming singular;
the grey-marker threshold fixes this without altering probability values.
P039-P042 mathematical and scene regressions also passed.

No native host, application window, rendering, image capture or image viewing
was used in verification. Visual layout and pointer feel remain for the user.

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b -t math_lab paths_math_shell_tensor_probability_tests paths_math_number_field_tests paths_math_exploration_tests paths_math_layers_tests paths_math_object_tests paths_scene_tests -j4
ctest --test-dir b -R '^paths_(math_shell_tensor_probability|math_number_field|math_exploration|math_layers|math_object|scene)_tests$|^paths_math_.*_cli$' --output-on-failure
./b/math_lab --validate --object flux --level 3 --set flux_field=2 --set flux_orientation=1 --set boundary_time=0 --advance 1 --check
./b/math_lab --validate --object tensor --level 3 --set basis_angle=45 --check
./b/math_lab --validate --object probability --level 3 --set chain=1 --set chain_steps=12 --check
```

For the user to open and visually try this batch:

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object flux
```

Use the object and layer controls to explore all three in one window. `--help`
lists every parameter; arguments apply in order. Walk CLI controls are
`--walk-step` and `--reset-probability-walk`.

Topic provenance: the user's TOC at
`/Users/kogaryu/Documents/ChatGPT/math terms and definitions/README.md`, with
calculus sections on flux/Stokes/divergence, linear-algebra sections on tensor
products/contraction, and discrete-math sections on Markov chains/random walks.
The notes were read as text only and were not modified.
