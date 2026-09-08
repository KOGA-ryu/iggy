# P041: symmetry, harmonics and motion

Implements the three additions recommended and authorised after P040. The
native math lab now contains ten objects. Topic direction comes from the
user's read-only `math terms and definitions/README.md` and its algebra,
trigonometry and calculus sections. Other proposed object families remain
future work. No source-note content or third-party code was copied or edited.

The user's no-images constraint applies throughout: no captures, renders,
image inspection, app windows or browser previews. Verification uses pure
CPU tests and `math_lab --validate`, which returns before the native host.

| Object | Layer 0 | Layer 1 | Layer 2 | Layer 3 |
| --- | --- | --- | --- | --- |
| Symmetry | Labelled cube rotations | Ordered composition and Undo | Cyclic subgroups and powers | Orbits, stabilisers and the permutation action |
| Harmonics | Complex rotor and sine projection | Lissajous curves | Fourier synthesis and RMS error | Orthogonality and coefficient extraction |
| Motion | Spring/pendulum initial state | Phase trajectories and conserved energy | Damping and dissipated energy | Forcing, driving work and spring convolution |

## Mathematical conventions

The symmetry object uses all 24 proper cube rotations, represented by exact
signed-permutation matrices. Breadth-first closure under world-axis X/Y/Z
quarter turns gives a stable rotation order with identity at index zero.
Applying R then S produces S R. A-H are vertex sign bits, with x=1, y=2,
z=4; A is (-,-,-), H is (+,+,+). Labels move with the cube. The permutation
diagram identifies fixed destination slots. Composition shows both orders
on separate small cubes. Undo has a bounded 64-turn history. Changing level
starts a fresh history. Cyclic generators have orders 4, 2 and 3. The final
layer counts the full vertex orbit and stabiliser and exposes the faithful
permutation representation; it does not claim a general group-theory engine.

Harmonic time spans 0 to 2*pi. A single phasor has complex coordinates
`a exp(i(n*t+phase))`. The Lissajous x-y projection uses equal plot scales;
its third spatial coordinate shows time, so the lifted wire does not close.
Square, sawtooth and triangle targets have period 2*pi and midpoint values
at jumps. One to eight nonzero sine terms form each Fourier approximation.
Signed spectrum stems show the included coefficients. Full-period RMS error
uses Parseval, and a separate 1,024-point midpoint integral checks the selected
coefficient. The periodic series and coefficient extraction are implemented;
a general continuous Fourier-transform workbench remains future work.

Motion uses unit mass/inertia, restoring coefficient k in [0.25,4], and
initial displacement/angle in [-1.2,1.2]. The spring obeys
`q'' + c*q' + k*q = F*cos(Omega*t)`. The pendulum uses `k*sin(q)` instead
of `k*q`, with angles in radians. Damping is enabled at layer 2; forcing at
layer 3. Mechanical energy, accumulated damping loss and driving work satisfy
`E(t) + loss(t) = E(0) + work(t)` within the reported integration error.

Classical RK4 uses steps no longer than 1/256 second. Every selected time is
recomputed from the initial state, with a fixed 129-point trajectory across
0-12 seconds; no frame-dependent integration state accumulates. A rebuild
uses at most 6,144 RK4 steps for the selected state and trace combined. Play,
Pause, Advance, Restart and plot scrubbing enter the same dispatcher. Playback
stops at the end of the time window. Scrubbing and checking pause it.

For a linear spring, the causal impulse response is the inverse Laplace
transform of `H(s)=1/(s^2+c*s+k)`. The final layer displays h(t), the free
response and the zero-initial-state convolution response. Each convolution
uses 512 Simpson panels and is compared with the independently integrated
ODE response. Critical damping and undamped resonance require no division
by a frequency-response pole. This linear decomposition is not applied to
the nonlinear pendulum. Geometry is scaled for inspection; plots/readouts
retain model units. Numerical challenges use tolerances, not formal proofs.

## Ownership and limits

`MathObjects::dispatch` remains the sole owner of parameters, level, turn
history, time, playback and challenge outcomes. Rejected actions do not alter
the revision or state. Lower levels restore hidden advanced defaults. The
snapshot adds a bounded permutation/history view and spectrum/equal-aspect
plot flags. It retains 192 parts, 16 labels, 12 measurements, three plots of
three 129-point series, and the existing matrix/surface/contour limits.

The existing scene adapter and native host are unchanged. The app presents
new controls and diagrams and forwards semantic actions. No scored-question,
sorter, progress-save or parent-project owner changes. Changes are uncommitted.

## Repeatable text-only checks

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b -t math_lab -j4
ctest --test-dir b -R '^paths_math_(function|matrix|surface|symmetry|harmonics|oscillator)_cli$' --output-on-failure

cmake -S . -B b-math-headless -DCMAKE_BUILD_TYPE=Release -DPATHS_BUILD_NATIVE=OFF
cmake --build b-math-headless -t paths_math_exploration_tests paths_math_layers_tests paths_math_object_tests paths_scene_tests -j4
ctest --test-dir b-math-headless -R '^(paths_math_exploration_tests|paths_math_layers_tests|paths_math_object_tests|paths_scene_tests)$' --output-on-failure

./b/math_lab --validate --object symmetry --level 1 --turn x --turn y --check
./b/math_lab --validate --object harmonics --level 3 --set waveform=0 --set probe_frequency=2 --check
./b/math_lab --validate --object oscillator --level 3 --advance 8 --check
```

`--turn` accepts x, y, z, x-inverse, y-inverse and z-inverse. `--undo-turn`
removes one turn; `--identity` clears the orientation history. `--advance`
accepts a finite positive duration up to 12 and stops at the object's time
limit. Arguments apply in order. `--help` lists parameter keys and ranges.

## Verification

On 2026-09-07 the Release native build and all ten targeted checks pass:
four pure model/scene tests and six CLI scenarios using `--validate`. The
new pure test links only libc++ and libSystem. It checks 24 rotations and
all 576 pairwise products, permutation composition, inverses, cyclic powers,
orbit/stabiliser counts, Fourier coefficients and RMS against independent
quadrature, analytic spring solutions in three damping regimes, resonant and
nonresonant forcing, convolution/ODE agreement, nonlinear pendulum energy,
all twelve challenges, playback, rejection, seeking and parameter boundaries.

The new test inspects 259 CPU geometry states, reaching 4,641 vertices and
19,584 indices. The earlier layer regression inspects 524 states; the original
object regression reaches 7,179 vertices and 22,800 indices. All remain below
the unchanged 8,192-vertex and 65,536-index capacities. Warning-enabled syntax
checks pass. No images were generated, captured, rendered or viewed. Visual
layout, human pointer interaction and swapchain acceptance remain unverified.
