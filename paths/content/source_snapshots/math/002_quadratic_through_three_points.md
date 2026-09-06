+++
id       = "002"
book     = "meckes_meckes_linear_algebra"
chapter  = "ch_01"
exercise = "1.1.7"
method   = "substitution then elimination"
check    = "invariant"
setup    = "not started"
solve    = "not attempted"
opened   = ""
+++

# 002 — a quadratic through three points

## Question

> Suppose that  $f(x) = ax^2 + bx + c$  is a quadratic polynomial whose graph passes through the points (-1, 1), (0, 0), and (1, 2).
>   - (a) Find a linear system satisfied by a, b, and c.
>   - (b) Solve the linear system to determine what the function f is.

Source: `meckes_meckes_linear_algebra` ch_01, exercise 1.1.7.

## Attempt

Yours. No tool writes here.

- **GIVEN:**
- **FIND:**
- **CONDITIONS:**
- **METHOD:** … because …
- **START:**
- **CHECK:**

Help used before committing: none / notation / method 1 / method 2 / method 3

**A setup drill ends here.** Part (b) is the full-solve pass.

---

<details><summary><b>Notation</b> — what the symbols say</summary>

| written | say it | role | means here |
|---|---|---|---|
| $a, b, c$ | "a, b, c" | **unknown** | the three numbers to determine |
| $x$ | "x" | **function input** | supplied three times; **not** an unknown here |
| $f(x) = ax^2+bx+c$ | "f of x equals a x squared plus b x plus c" | the model | a known shape with unknown coefficients |
| $(-1, 1)$ | "the point minus one, one" | **supplied value** | an observation: $f(-1) = 1$ |
| "linear system" | | | equations in which each unknown appears only to the first power |

**The role column is the whole trap.** At school `x` is the unknown. Here `x` is
a function input you are handed three times, and `a, b, c` are the unknowns. The
expression is quadratic in x and **linear in a, b, c** — which is what makes this
a linear system.

</details>

<details><summary><b>Method 1</b> — what family of problem is this?</summary>

A linear system in disguise. Three unknowns; each supplied observation becomes
one equation.

</details>

<details><summary><b>Method 2</b> — the method, its cue, and its conditions</summary>

**Substitution, then elimination.**

**The cue:** a formula with unknown coefficients, plus separately supplied values
it must take. That pairing is always this method.

**Its conditions:** the unknowns must enter linearly — a sum of known multiples
of them. And the supplied inputs must be distinct, or two rows repeat and the
system stops determining three numbers.

In matrix form it is $M\theta = y$, which is `numpy.linalg.solve`.

</details>

<details><summary><b>Method 3</b> — setting it up</summary>

A row is the known functions evaluated at one input, with the unknowns stripped
out: for $ax^2+bx+c$ the row at x is $[x^2,\; x,\; 1]$. The right-hand side is
the observed height.

The `1` is the coefficient of `c` and is the one people drop, because nothing is
written beside c.

Start from the middle observation: $x = 0$ kills two terms and hands you an
unknown for free.

</details>

<details><summary><b>Setup key</b> — open after committing your attempt</summary>

| | |
|---|---|
| **GIVEN** | $f(x)=ax^2+bx+c$ with $a,b,c$ unknown reals; three observations $f(-1)=1$, $f(0)=0$, $f(1)=2$ |
| **FIND** | (a) the linear system; (b) the three coefficients, hence f |
| **CONDITIONS** | f is exactly quadratic — degree at most 2, nothing else. The three inputs are distinct, which is what makes the rows independent |
| **METHOD** | substitution to build the system, elimination to solve it, **because** the unknown coefficients enter linearly |
| **START** | $a - b + c = 1$, $c = 0$, $a + b + c = 2$ |
| **CHECK** | evaluate the recovered f at $-1, 0, 1$ and compare with $1, 0, 2$ to `1e-12`. That establishes it passes through the points; it does not establish uniqueness |

</details>

<details><summary><b>Solution</b> — contains the answer</summary>

| step | work | why it is valid |
|---|---|---|
| 1 | $f(-1)=1 \Rightarrow a - b + c = 1$ | "passes through the point" and "takes that value at that input" are the same statement |
| 2 | $f(0)=0 \Rightarrow c = 0$ | two terms are multiplied by zero. Scan for the equation with the most zeros first |
| 3 | $f(1)=2 \Rightarrow a + b + c = 2$ | same substitution |
| 4 | $c=0$, so $a-b=1$ and $a+b=2$ | substituting a value that holds everywhere in the problem |
| 5 | add them: $2a = 3$, so $a = \tfrac32$ | equals added to equals give equals — the justification for every elimination |
| 6 | $\tfrac32 + b = 2$, so $b = \tfrac12$ | back-substitution |

**Answer:** $f(x) = \tfrac{3}{2}x^2 + \tfrac{1}{2}x$, with $a=\tfrac32$,
$b=\tfrac12$, $c=0$.

**Check:** $f(-1)=1.5-0.5=1$, $f(0)=0$, $f(1)=1.5+0.5=2$. Performed by hand.

**Key review:** the arithmetic above was done by the assistant without running
anything, and checked by hand substitution only. Not an authoritative key —
`pp.py check 002` is what settles it.

</details>

<details><summary><b>Which line licenses this?</b> — one step, reason removed</summary>

Step 5 adds two equations and concludes $2a = 3$.

Name what permits adding two equations. Then the harder one: would it still be
permitted if one of them were an **inequality**?

</details>

<details><summary><b>Variant</b> — one thing changed</summary>

Same three points, but $f(x) = ax^3 + bx^2 + cx + d$ — four unknowns, three
equations. Does the method still apply? What does this system have that the
original did not, and what would pin down a single answer?

</details>

## Program

`work/002.py` — rows $[x^2, x, 1]$ at $x = -1, 0, 1$, right-hand side
$[1, 0, 2]$; solve; print $a, b, c$ and f at the three points. Exit 0 when the
check passes.

## Run

Written by `pp.py check 002`. Program output only — never your attempt.

```
```

## What I got wrong

