# P058 — Vibrating Membrane Lab

`membrane` is the 29th reusable Math Lab object. It supplies animated rectangular
surfaces, mode inspection, a moving probe and energy views through the existing
compact inspector and indexed-mesh renderer. The preceding Patch Lab was visually
accepted by the user before this checkpoint.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object membrane --level 0 --object-preset 0
```

| Example | Construction | Try |
| --- | --- | --- |
| 0: Drumhead | One (1,1) mode, released from rest | Play; follow the centre through equilibrium |
| 1: Divided membrane | One (2,3) mode | Layer 1; inspect the interior nodal lines |
| 2: Interference | Opposed (1,1) and (3,1) initial displacements | Layer 2; compare u=.25 with u=.5 at time zero |
| 3: Damped pluck | Four modes approximating a centred tent, damping gamma=.35 | Layer 3; advance to model time 6 |

Four mode slots have editable integer `m,n`, initial displacement and initial
velocity. The slot dropdown exposes only that slot's four fields. Width/depth,
tension, areal density, damping, time and the UV probe use the same semantic
parameter route as earlier objects. Every layer retains all initial conditions
and the selected time. An edit pauses playback and evaluates those initial
conditions at the current time; it does not splice two physical trajectories.

Playback runs at half model speed: two wall-clock seconds advance one model
second. Pause, Advance and Restart use the existing controls. The model time
window is [0,12] seconds and stops at its endpoint. Restarting does not erase the
mode configuration. Resetting Profile restores all slots to canonical defaults;
choosing an example restores the full named configuration atomically.

## Views and four layers

The **Surface** dropdown chooses the combined membrane or the selected slot.
Surface colour, the gold probe and its section guides follow that choice.
Numerical probe values and global energies continue to describe the combined
membrane; a separate displayed-height readout makes the distinction explicit.
The view choice never changes the solution or the energy calculation.

| Layer | Linked quantities | Challenge |
| --- | --- | --- |
| 0: Displacement and motion | Height, velocity, component time traces and a spatial section | Cross below -.05 at the combined probe, at positive time |
| 1: Modes and nodal lines | Selected spatial basis, natural frequencies and rest-plane node guides | Locate an interior node of an excited selected slot |
| 2: Superposition | Combined, selected and remaining contributions | Cancel distinct nonzero mode contributions at the probe |
| 3: Energy and damping | Kinetic/strain/total energy, retained percentage and loss rate | After at least 3 seconds, retain less than 25% with positive damping |

World position is `(width*(u-.5), h, depth*(.5-v))`; world y is displacement.
Analytic normals point upward. The mesh has zero height on all four edges.
Displacement is coral above equilibrium and blue below. Layer 1 colours the
selected **spatial basis**, independent of its current amplitude. Gold nodal
lines are references on the rest plane at u=k/m and v=k/n. They are nodes of the
selected slot, and are not generally nodes of a mixed surface. Use Selected slot
to isolate that motion. A moment when the entire mode passes through zero is
not treated as a spatial node.

Layer 3 colours local energy density of the displayed surface. Global energy
readouts and the distinct-pair energy table always integrate the combined model.
The velocity arrow has a bounded display length; its direction is signed and
the table retains the actual value. Shape only hides construction guides.

## Mathematical and numerical contract

The input is a uniform rectangular membrane with fixed-displacement edges.
It models linear transverse waves and uniform viscous damping, with no forcing:

```text
h_tt + 2*gamma*h_t = (T/rho) * (h_xx + h_yy)
h(x,y,t) = sum q_mn(t) sin(m*pi*x/W) sin(n*pi*y/D)
omega_mn^2 = (T/rho)*pi^2*((m/W)^2 + (n/D)^2)
q'' + 2*gamma*q' + omega^2*q = 0
```

The solver evaluates the exact closed-form initial-value solution in double
precision, with no accumulated numerical timesteps. A sinc limit handles
critical damping. Overdamped motion uses decaying exponentials and `expm1` to
avoid subtracting nearly equal exponentials near the critical limit. Natural
frequency means omega/(2*pi), including when damping prevents oscillation.

Equal `(m,n)` pairs sum their initial states before integrated energy is
computed. Distinct spatial modes remain distinct even when their frequencies
coincide. This retains interference cross terms for duplicated slots and
orthogonality between different modes:

```text
K = rho*W*D/8 * sum qdot_mn^2
P = rho*W*D/8 * sum omega_mn^2*q_mn^2
E = K + P
dE/dt = -4*gamma*K
```

Energy lost is E(0)-E(t), with roundoff below zero clamped to zero. This displayed
difference is not an independent energy-balance certificate. A zero-energy state
has no retained-percentage measurement. Tests separately integrate local energy
over the rectangle and dissipated power over time.

Damped pluck uses a height-.5 centred product tent. Its four coefficients are
the exact projections for pairs (1,1), (1,3), (3,1), (3,3):
`a_mn = .5*64*sin(m*pi/2)*sin(n*pi/2)/(pi^4*m^2*n^2)`.
All initial velocities are zero. This finite sum approximates the tent; it does
not reproduce its sharp crest or an arbitrary impact. The evolving solution is
exact for the stated four-mode initial shape.

All loops and storage are bounded. There are four slots, mode numbers 1–6,
dimensions 1–4, tension .25–4, density .5–2, gamma 0–2, and per-slot initial
displacement/velocity -.6–.6. Preparing a state costs O(4^2) for duplicate grouping;
point evaluation costs O(4), and mesh generation O(4*n^2). The display supports
12–48 subdivisions, at most 2,401 vertices and 4,608 triangles. Mesh detail has
no effect on physical measurements. The large-amplitude settings still represent
the linear mathematical model, not nonlinear cloth, bending plates or collisions.

Plots use 129 samples. Their time window spans at most four cycles of the fastest
configured natural mode, so twice-frequency energy oscillations retain at least
16 intervals per cycle. The plot window moves with the selected time; the time
slider retains access to all 12 seconds. The display mesh is sampled and is not a
spatial approximation certificate; native playback can also be limited by frame
rate. Neither display limitation changes the analytic readouts.

The mode basis and natural frequencies follow the
[University of Manchester membrane notes](https://users.hep.manchester.ac.uk/u/rjones/rect_modes/modes.html).
The initial-condition projection follows the
[Waves and Fluids rectangular-domain lecture](https://gustavdelius.github.io/WavesAndFluids/lecture_08.html).
Implementation and tests are original; no external code or image assets were copied.

## Interfaces and verification

`Membrane.hpp/.cpp` owns validated modal dynamics, sampling and global energy.
`MembraneGeometry.hpp/.cpp` projects a prepared state into `MathTriangleSurface`.
`MathObjects` owns 28 parameters, four presets, playback, linked snapshots and
checks. Existing parameter numeric values are retained; the new IDs are appended.
The selector uses the existing compact control metadata. No renderer or shader
changes are needed. The document figure registry discovers this object; authored
textbook lessons remain with the textbook worker.

The CPU checks compare closed-form motion with independent RK4 integration,
including both sides of critical damping and high frequencies. Finite differences
check spatial/time derivatives and the PDE. Spatial quadrature checks energy,
including cancellation and equal-frequency distinct modes. Time quadrature checks
dissipated power. Further checks cover boundary nodes, frequency scaling, mesh
topology/normals, selected views, compact controls, presets, playback endpoints,
retained layers, challenge outcomes, invalid inputs and parameter extrema.

Release `math_lab` and `sorter` both built successfully after installation.
All 91 selected checks passed: 19 CPU suites and 72 early-return text-validation
commands. This includes all four new membrane challenges and the existing
curve, lathe, patch, Boolean, matrix and textbook regression checks.

The membrane suite passed 6,784,223 assertions across 122 CPU mesh states,
including a low-frequency initial-velocity case with displacement above 6 units.
The largest published scene used 5,167 vertices and 21,984 indices, within the
existing renderer capacities. The shared layout suite passed 153,905 assertions
across 6,480 layouts and all 104 object/layer states, retaining every curve and
lathe selector as well as the new mode selector. All existing object and parameter
numeric IDs retain their original values. Installed source checksums matched
the verified files after testing; the final text diff passed whitespace review.

No images, native windows or font-rasterization checks are used. Visual acceptance
of the membrane presets remains with the user. Changes remain uncommitted.
