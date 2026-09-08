# P045 — Spherical harmonics, quadratic forms and roots of unity

Three more native objects, twelve working lessons, nineteen parameters. The
lab now has twenty-two objects. Implementation remains in MathObjects and the
existing UI/CPU scene path. Changes are uncommitted. No images, captures,
previews, rendering or windows were used for verification.

| Object / CLI key | Layer 1 | Layer 2 | Layer 3 | Layer 4 |
| --- | --- | --- | --- | --- |
| Harmonic sphere / `spherical` | Polar coordinates | 25 real modes through degree 4 | Superposition and orthogonality | Laplace-Beltrami spectrum and heat diffusion |
| Quadratic forms / `quadratic` | Evaluation and sections | Orthogonal diagonalization | Inertia and draggable zero sets | Rayleigh quotient and constrained extrema |
| Roots of unity / `roots` | Complex roots and orders | Power maps and collisions | Cyclic subgroup paths | Cyclotomic polynomial and Galois action |

## Mathematical contract

Spherical coordinates use z as the polar axis. Real modes use normalized
associated Legendre functions with the Condon-Shortley phase: m>0 is cosine,
m<0 is sine, m=0 is zonal. Mode IDs enumerate l*l through (l+1)*(l+1)-1.
The sphere becomes a signed-color lobe surface with radius 0.3+1.4*abs(f).
This radius is a display convention, not a physical wave radius. Eight
Gauss-Legendre nodes in cos(theta) and sixteen azimuth samples check the mode
inner products. Equal-mode interference is retained, including complete
cancellation. Heat time is 0-1, diffusivity is one, and each degree decays by
exp(-l*(l+1)*t). A constant component survives. The finite-difference Laplacian
uses a fixed interior probe; it never evaluates the polar coordinate singularity.
These conventions follow [DLMF 14.30](https://dlmf.nist.gov/14.30); the kernels
are original implementations.

Quadratic forms use A=Q D Q^T, with principal values in [-2,2] at quarter steps.
Q=Rz(yaw) Ry(pitch). The first layer uses Q=I. The graph is the section
q(x,y,z0) on [-2,2]^2, with height 0.15*q. It is not the level surface q=1.
The first two probe coordinates range over [-2,2], z0 over [-1.5,1.5]. Inertia
counts the signs of the chosen principal values without a numerical rank
heuristic. Zero contours use a 20-by-20 grid and linear edge interpolation;
they are approximate, may miss features smaller than a cell, and do not certify
whole-space classification. The entire zero section is reported explicitly.
The fourth layer uses a unit sphere with color from q. At x=0 the Rayleigh
quotient is explicitly undefined. Otherwise it lies between the extreme
principal values; extrema occur in their eigenspaces.

Roots use n=3-12 and exact residue arithmetic. A root's multiplicative order is
n/gcd(n,k); the power map has n/gcd(n,a) distinct images. The scene's radius 1.3
and vertical separation are display scales; complex tables use unit modulus.
Subgroup path height records the step, with the selected power reduced modulo
the cycle. Integer monic division constructs Phi_n. Only gcd(a,n)=1 defines an
automorphism of Q(zeta_n) fixing Q; nonunit candidates are identified as power
maps. Automorphism composition multiplies exponents modulo n. The displayed
primitive-root count is both the cyclotomic degree and Galois-group size.
This is the cyclotomic case, not a general Galois-group solver. See Milne,
[Fields and Galois Theory, Cyclotomic Extensions](https://www.jmilne.org/math/Books/FT0.pdf).

## Source direction and ownership

Read-only direction: `/Users/kogaryu/Documents/ChatGPT/math terms and definitions/README.md`,
with spherical harmonics, Euler/de Moivre and roots from `sections/trigonometry.md`,
quadratic forms and spectral theory from `sections/linear_algebra.md`, and
splitting fields/Galois groups from `sections/algebra.md`. These are selected
adaptations; this batch does not mark the corpus reviewed. Existing rank/nullity
reviewed notes, Library, sorter and unrelated work remain preserved. No borrowed
source code or renderer changes were introduced.

## Text-only verification

Eight targeted CPU suites and eighteen native `--validate` checks pass. The
native validation branch returns before constructing NativeVulkanHost.

The new suite independently differentiates explicit Legendre polynomials,
checks all 25-by-25 mode inner products and spectral heat decay, verifies rotated
quadratic matrices/inertia/Rayleigh bounds, and compares exact cyclotomic
coefficients and divisor products against x^n-1. It also verifies group orders,
nonunit rejection as automorphisms, challenge outcomes, parameter bounds,
atomic contour updates, cancellation, zero forms and undefined zero probes.

Measured coverage: 14,775 harmonic checks, 243 rotated quadratic cases, 1,800
root actions and 773 CPU geometry states. Maximum new-batch geometry is 3,960
vertices and 19,584 indices, below the unchanged buffers. Model and UI compile
cleanly with -Wall -Wextra -Wpedantic. Visual layout and pointer feel remain for
the user to test; automated success does not establish visual acceptance.

## User visual test

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object spherical --level 1
```

Select Quadratic forms or Roots of unity in the object selector, or replace
`spherical` with `quadratic` or `roots`. The four layers remain selectable.
