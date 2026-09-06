+++
id       = "013"
book     = "meckes_meckes_linear_algebra"
chapter  = "ch_04"
exercise = "4.3.9"
method   = "orthogonal projection onto a span"
check    = "two methods"
setup    = "not started"
solve    = "not attempted"
opened   = ""
+++

# 013 — the closest point on a line

## Question

> Show that the point on the line y = mx which is closest to the point (a, b) is  $\left(\frac{a+mb}{m^2+1}, m\frac{a+mb}{m^2+1}\right)$ .

Source: `meckes_meckes_linear_algebra` ch_04, exercise 4.3.9.

## Attempt

Yours. No tool writes here.

- **GIVEN:**
- **FIND:**
- **CONDITIONS:**
- **METHOD:** … because …
- **START:**
- **CHECK:**

Help used before committing: none / notation / method 1 / method 2 / method 3

**A setup drill ends here.** Producing the displayed point is the full-solve pass.

---

<details><summary><b>Notation</b> — what the symbols say</summary>

| written | say it | role | means here |
|---|---|---|---|
| $y = mx$ | "y equals m x" | the set | the line through the origin of slope m; as a collection of points, everything of the form $(t, mt)$ |
| $m$ | "m" | **parameter** | a fixed slope. Fixed inside one instance of the problem, different between instances — that is why the program runs at three of them |
| $(a, b)$ | "the point a b" | **parameter** | the point you are approaching. Fixed inside one instance, different between instances — the same test that makes m a parameter, and the program runs at four points per slope. The question never gives it a number, so the answer must hold for every choice |
| $t$ | "t" | **unknown** | not in the question. It is the one number you introduce: how far along the line you are. See Method 3 |
| $\frac{a+mb}{m^2+1}$ | "a plus m b, all over m squared plus one" | | the first coordinate of the answer. The second is the same quantity multiplied by m, which is exactly what "on the line $y=mx$" demands |
| "closest" | | | smallest ordinary distance $\sqrt{(x-a)^2 + (y-b)^2}$ |
| $\langle u, v \rangle$ | "inner product of u and v" | | on $\mathbb{R}^2$ this is the dot product $u_1v_1 + u_2v_2$ |
| $\|v\|$ | "norm of v" | | its length, $\sqrt{\langle v, v\rangle}$ |
| $U^{\perp}$ | "U perp" | the set | everything perpendicular to every vector in U. It earns a row because Theorem 4.16 part 3 says the leftover $p - P_U p$ lands in $U^{\perp}$, and that is the property both checks below test |
| $P_U$ | "P sub U", "the orthogonal projection onto U" | function | takes a vector and returns the part of it lying in U |
| "Show that" | | | the answer is printed in the question. Your job is to **produce** it, not to recognise it. A page of work that ends in that expression is the deliverable |

**The role column is the whole trap.** Three letters sit in this problem — $m$, $a$, $b$ — and school reflex says "solve for one of them". None of them is an unknown. All three are parameters: handed to you, fixed inside one instance, free to differ between instances, and never given a number. The only unknown is a number that does not appear in the question at all: the position along the line.

Two symbols the book uses without warning. A **superscript perp**, $U^{\perp}$, is not a power; it names a different set. A **subscript on P**, $P_U$, is not a multiplication; it says which subspace is being projected onto.

</details>

<details><summary><b>Method 1</b> — what family of problem is this?</summary>

A closest-point problem: one point, one set, and a distance to be made as small as possible.

What matters about the set is its shape. It is a **line through the origin** — a one-dimensional subspace of $\mathbb{R}^2$, described by a single direction. A set described by one direction can be searched with one number, which turns "find the nearest of infinitely many points" into a question about a single quantity.

Chapter 4 exists to answer exactly this shape of question, so the machinery is nearby and you should expect to use it rather than start from scratch.

</details>

<details><summary><b>Method 2</b> — the method, its cue, and its conditions</summary>

**Orthogonal projection onto a span.**

For a subspace U spanned by one nonzero vector v, the projection of a point p is

$$P_U p = \frac{\langle p, v \rangle}{\langle v, v \rangle}\, v$$

which is Theorem 4.16 part 2 with the single orthonormal basis vector $v/\|v\|$, and Theorem 4.19 part 2 is what says that $P_U p$ is the closest point of U to p.

**The cue:** the words "closest point" together with a set that is a **line or plane through the origin**. Distance plus a subspace is always projection. If instead the set were curved, or were a line that misses the origin, this cue does not fire.

**Its conditions**, all three of which this problem satisfies and one of which is easy to lose:

1. **U must be a subspace** — it has to contain the origin and be closed under adding and scaling. $y = mx$ has no intercept term, so it does. $y = mx + 3$ would not, and Theorem 4.19 would not apply to it as written.
2. **The spanning vector must be nonzero**, or you divide by zero.
3. **"Closest" must mean the distance that comes from the inner product you are projecting with.** Here the ordinary dot product on $\mathbb{R}^2$, whose norm is ordinary Euclidean distance. Project with one inner product and measure with another and the answer is simply wrong.

**A second method that also works, with no projections in it.** Write the squared distance from $(a,b)$ to the moving point on the line as a function of one variable, and minimise it with calculus: set the derivative to zero, and confirm with the second derivative that the stationary point is a minimum rather than a maximum. Squared distance rather than distance, because squaring is increasing on the non-negative numbers so both are smallest in the same place, and the square has no square root to differentiate.

That the two routes land on the same point is what the program checks.

</details>

<details><summary><b>Method 3</b> — setting it up</summary>

Give the line one direction vector and give the point one name.

Every point of $y = mx$ has second coordinate m times its first. Call the first coordinate t, and the point is $(t, mt) = t\,(1, m)$. So take

$$v = (1, m), \qquad U = \operatorname{span}\{v\}, \qquad p = (a, b).$$

That single substitution is the whole setup, and it does two things at once: it writes every point of the line exactly once, and it makes t the only quantity free to move.

Notice what $v$ being $(1, m)$ buys you, because it is the reason nothing in this problem has an exceptional case: its first coordinate is 1, so $v$ is never the zero vector, no matter what m is.

Now you have a choice of two things to write down — the projection formula from Method 2, or the squared distance $\|p - tv\|^2$ as a function of t. Write one of them out in coordinates in terms of a, b, m and t.

Stop there and commit before opening anything below.

</details>

<details><summary><b>Setup key</b> — open after committing your attempt</summary>

| | |
|---|---|
| **GIVEN** | a fixed real slope m; the line $U = \{(t, mt) : t \in \mathbb{R}\} = \operatorname{span}\{(1,m)\} \subseteq \mathbb{R}^2$; a fixed point $p = (a,b) \in \mathbb{R}^2$ |
| **FIND** | a derivation ending at the point $\left(\frac{a+mb}{m^2+1},\; m\frac{a+mb}{m^2+1}\right)$. The value is supplied; the work is the deliverable |
| **CONDITIONS** | "closest" is Euclidean distance, from the standard dot product on $\mathbb{R}^2$. The line has no intercept, so it passes through the origin and is a subspace — this is the hypothesis Theorem 4.19 needs. m, a and b are unrestricted reals; there is no case to exclude |
| **METHOD** | orthogonal projection onto $\operatorname{span}\{(1,m)\}$, **because** the set is a one-dimensional subspace and the criterion is distance. Equivalently, minimise a quadratic in one variable |
| **START** | $P_U p = \dfrac{\langle p, v\rangle}{\langle v, v\rangle}\,v$ with $v = (1,m)$, $p = (a,b)$; or equivalently minimise $g(t) = (t-a)^2 + (mt-b)^2$ over $t \in \mathbb{R}$ |
| **CHECK** | the residual $p - P_U p$ must be perpendicular to $(1,m)$ — that is Theorem 4.16 part 3, $p - P_Up \in U^{\perp}$ — so their dot product is zero to `1e-12`. Independently, `numpy.linalg.lstsq` on the one-column matrix $[1;\,m]$ must return the same point to `1e-12`, and no point sampled along the line may be closer than the closed form by more than `1e-12`. **The extent:** any one of these would catch a wrong formula, and at the twelve instances the program runs none of them fires. That is falsification survived, not proof — the exercise says "Show that", a claim about every real m, a and b, and twelve floating-point cases cannot reach it. The general statement rests on the derivation, and the uniqueness carried by the word "the" on Theorem 4.19 part 2. Neither rests on the program |

</details>

<details><summary><b>Solution</b> — contains the answer</summary>

| step | work | why it is valid |
|---|---|---|
| 1 | Write $v = (1, m)$. Then $U = \{t\,v : t \in \mathbb{R}\} = \operatorname{span}\{v\}$ is exactly the line $y = mx$ | "$y = mx$" says the second coordinate is m times the first; naming the first coordinate t writes every such point once and no point twice |
| 2 | U is a subspace of $\mathbb{R}^2$: it holds the origin ($t=0$) and survives adding and scaling | step 1 wrote U as $\operatorname{span}\{v\}$, and a span is always a subspace: a sum of multiples of v is a multiple of v, and so is a scalar times one. It holds the origin because the line has no intercept term to shift it off. This is the hypothesis Theorems 4.16 and 4.19 both ask for |
| 3 | The closest point of U to $p=(a,b)$ is $P_U p$, and no other point of U ties it | Theorem 4.19 part 2: $\|p - P_Up\| \le \|p - u\|$ for every $u \in U$, with equality **if and only if** $u = P_U p$. That "only if" is what licenses the word "the" in the question |
| 4 | $v \neq 0$, so $(v/\|v\|)$ is an orthonormal basis of U and $P_U p = \dfrac{\langle p, v\rangle}{\langle v, v\rangle}\,v$ | Theorem 4.16 part 2 with one basis vector $e_1 = v/\|v\|$: $\langle p, e_1\rangle e_1 = \frac{\langle p,v\rangle}{\|v\|^2}v$. The first coordinate of v is 1, so v is never the zero vector and there is no case to exclude |
| 5 | $\langle p, v\rangle = a\cdot 1 + b\cdot m = a + mb$ and $\langle v, v\rangle = 1^2 + m^2 = m^2 + 1$ | the standard dot product on $\mathbb{R}^2$, entry by entry |
| 6 | $P_U p = \dfrac{a+mb}{m^2+1}\,(1, m) = \left(\dfrac{a+mb}{m^2+1},\; m\dfrac{a+mb}{m^2+1}\right)$ | scalar multiplication acts coordinate by coordinate. $m^2 + 1 \ge 1 > 0$ for every real m, so the division is always legal |

**The same answer without projections.** Minimise $g(t) = (t-a)^2 + (mt-b)^2$. Then $g'(t) = 2(t-a) + 2m(mt-b) = 2\big[t(1+m^2) - (a+mb)\big]$, which is zero exactly at $t = \frac{a+mb}{m^2+1}$, and $g''(t) = 2(1+m^2) > 0$ everywhere, so that stationary point is the minimum. Same t, same point.

**Answer:** the closest point is
$\left(\dfrac{a+mb}{m^2+1},\; m\dfrac{a+mb}{m^2+1}\right)$,
reached at $t = \dfrac{a+mb}{m^2+1}$ along the line, for every real m, a and b. It is the only closest point.

**Check:** performed by hand, two ways, not by machine.

Symbolically, with $t = \frac{a+mb}{m^2+1}$ the residual is $p - tv = (a - t,\; b - mt)$, and its dot product with the direction $(1, m)$ is $(a - t) + m(b - mt) = (a + mb) - t(1+m^2) = 0$ exactly. Perpendicular, as an orthogonal projection must be.

Numerically, one instance: $m = 2$, $(a,b) = (1,3)$ gives $t = \frac{1 + 6}{5} = 1.4$ and the point $(1.4,\, 2.8)$. Residual $(-0.4,\, 0.2)$, dotted with $(1,2)$ is $-0.4 + 0.4 = 0$; squared distance $0.16 + 0.04 = 0.20$. Two neighbours on the line: $t = 1.3$ gives squared distance $0.09 + 0.16 = 0.25$, and $t = 1.5$ gives $0.25 + 0 = 0.25$. Both larger.

**Key review:** every line above was written by the assistant without running anything, and confirmed only by the hand arithmetic just shown, which is a spot check at one m and cannot see an error that survives $m=2$. It is not an authoritative key. `pp.py check 013` can falsify it at an instance, and if the program disagrees the program wins; but this is a "Show that" over every real m, a and b, so a program that agrees settles nothing on its own — the derivation is what carries the claim.

The two theorem numbers were read out of the book: Theorem 4.16 part 2 is the orthonormal-basis formula for $P_U$, and Theorem 4.19 part 2 is $\|v - P_Uv\| \le \|v-u\|$ with equality if and only if $u = P_Uv$. Both require U finite-dimensional, which a line in $\mathbb{R}^2$ is.

</details>

<details><summary><b>Which line licenses this?</b> — one step, reason removed</summary>

Step 3 replaces "the closest point of U to p" with "$P_U p$" — and quietly also claims there is only one.

Name the result that permits the replacement, and separately name the part of it that supplies the uniqueness. They are not the same half of the statement.

Then the harder one, and stay inside step 4. It does not project with $v$ itself; it projects with $v/\|v\|$. Name the hypothesis in Theorem 4.16 part 2 that forces the normalising, and say what the formula would produce if you fed it $v$ unnormalised instead — by what factor, and in which direction, would the answer be wrong?

</details>

<details><summary><b>Variant</b> — one thing changed</summary>

Change the line to $y = mx + c$ for a fixed nonzero c, everything else the same.

Does the projection method still apply as written? If not, find the smallest repair — there is one, and it costs two extra operations rather than a new theorem. Then state the closest point in terms of a, b, m and c, and confirm it collapses to the original answer when $c = 0$.

A variant gets its own review: this one changes a hypothesis, not a number, so check that the method still has what it needs before trusting the formula it produces.

</details>

## Program

`work/013.py` — the closed-form point against two independent computations, at
three slopes, with no `==` anywhere.

Inputs. Three values of the parameter: $m = 0$ (a horizontal line), $m = 2$, and
$m = -0.5$ (which is perpendicular to $m = 2$, so the two runs should disagree
sharply and that is worth seeing). For each m, four points $(a,b)$: $(1,3)$,
$(-2, 5)$, the origin $(0,0)$, and a **planted** point already on the line,
$(s, ms)$ with $s = 1.7$, whose closest point must be itself.

Three computations per case.

1. The closed form from the exercise, $\frac{a+mb}{m^2+1}(1, m)$, evaluated directly.
2. `numpy.linalg.lstsq` on the one-column matrix $A = [[1],[m]]$ against
   $[a, b]$, giving $\hat{t}$; the point is $A\hat{t}$. This is a different code
   path through a library that knows nothing about the exercise.
3. A scan of $t$ over 200001 evenly spaced values in $[-50, 50]$, keeping the
   smallest squared distance found, plus `scipy.optimize.minimize_scalar` on
   $g(t) = (t-a)^2 + (mt-b)^2$.

Outputs, one line per case: m, $(a,b)$, the closed-form point, the `lstsq`
point, the `minimize_scalar` point, the residual's dot product with $(1,m)$, and
the gap between the scan's best squared distance and the closed form's.

Passes when all of these hold, for all twelve cases:

- closed form and `lstsq` agree, `numpy.allclose` with `rtol=1e-12, atol=1e-12`;
- closed form and `minimize_scalar` agree to `1e-7` — much looser, because
  Brent's default `xtol` is `1.48e-8` and the routine is entitled to stop that
  far from the minimiser. A tighter band here would fail for a reason that has
  nothing to do with the mathematics. It still decides: a wrong formula misses
  by a whole number, not by `1e-8`;
- the residual's dot product with $(1,m)$ is below `1e-12` in absolute value;
- the scan never beats the closed form by more than `1e-12` (it is allowed to
  lose by any amount — the grid has no reason to land on the exact minimiser);
- the planted case returns its own input point to `1e-12`.

Exit 0 when every case passes, 1 with the first failing case printed otherwise.

## Run

## What I got wrong
