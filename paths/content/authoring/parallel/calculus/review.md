# Calculus pilot review: Build a derivative from a difference quotient

Coordinator update, 2026-09-10: this pilot has been corrected, jointly checked
and locally published. See the [integrated subject review](../../../../docs/SUBJECT_PILOT_REVIEW.md)
for current status and review overrides. Visual acceptance remains with the
user. The worker delivery record below describes the earlier candidate and
retains its original hashes and pending-review statements as history.

## Scope and status

This candidate authors exactly one reading and six `choices.v1` questions for
`pilot_calculus_derivative`, version 1, in the existing Calculus →
Differentiation chapter (`topic_0060`). It stays bounded to real polynomials
`Ax^2+Bx+C` and derivatives at a real point. It does not add a runtime calculus
checker, a symbolic parser, integrals, a chain rule, discontinuities, a figure,
or a claim of independent written work.

Status at authoring completion:

- Authored: yes — one reading, six questions, seven decisions, fourteen specific wrong-choice corrections.
- Mathematically checked: yes — independent expansion and coefficient-differentiation certificates plus rejection tests.
- Compiled: yes — the canonical document compiler accepted the reading and all six `choices.v1` questions.
- Model-replayed: yes — the prepared-choice model replayed all six routes, with seven reached decisions and fourteen wrong choices.
- Teaching-reviewed: author review complete; coordinator review and user teaching review remain pending.
- Published: no — coordinator-only action, not performed.
- Visually accepted: no — user-only confirmation after integration.

## Prerequisites and notation inventory

The learner needs real-number substitution, binomial-square expansion,
collecting like terms, factorization, and the idea that a limit concerns nearby
values. The reading explicitly introduces:

| Symbol or phrase | Meaning and condition |
| --- | --- |
| `f`, `x` | A real-valued function and its real input. |
| `a` | The fixed real input where the derivative is requested. |
| `f(a)` | Function value after substituting `a` into the whole function. |
| `f'(a)` | Derivative, the instantaneous slope at `a` if the limit exists. |
| `h` | A real input increment; `h != 0` in the difference quotient. |
| `(f(a+h)-f(a))/h` | Average slope between the distinct inputs `a` and `a+h`. |
| `lim(h->0)` | Limit through permitted nearby nonzero increments; not substitution into the original zero-denominator quotient. |

No physical units are assumed: these examples are explicitly real and
dimensionless. The reading distinguishes average slope from instantaneous slope
and distinguishes `f(a)` from `f'(a)` before either appears as a distractor.

## Mathematics and completeness review

For `f(x)=Ax^2+Bx+C`, direct expansion gives

`f(a+h)-f(a) = Ah^2+(2Aa+B)h = h(Ah+2Aa+B)`.

For nonzero `h`, division yields `Ah+2Aa+B`; its polynomial limit at zero is
`2Aa+B`. Independently differentiating coefficients gives `f'(x)=2Ax+B`, and
evaluating at `a` produces the same result. The certificate provider performs
those two routes separately and bounds coefficients by absolute value 9 and
the integer point by absolute value 3. It rejects malformed arrays, unsupported
degree and noninteger/nonfinite-style input before an answer is produced.

The cancellation argument has two parts. At `h=0`, the original quotient has a
zero denominator and is undefined. At nearby nonzero `h`, its factorization
allows cancellation, and the resulting polynomial has a limit at zero. The
lesson therefore does not infer a quotient value at zero from its removable
discontinuity. The independent finite-difference-style idea is intentionally
supplemental only: coefficient equality proves the polynomial identity.

The separate worked example is exactly `f(x)=x^2+x` at `a=1`: `f(1)=2`,
`f(1+h)=2+3h+h^2`, quotient `3+h` for `h != 0`, and derivative `3`. Its Hint,
Answer and Solution are distinct closed disclosures. The solution includes the
separate coefficient-differentiation check `2x+1` at `1`.

## Question and distractor review

| Role | Decision | Wrong-choice diagnoses |
| --- | --- | --- |
| Read notation | Select the output-change-over-increment derivative definition. | The reciprocal reverses the ratio; `f(3)` is a function value rather than a limiting slope. |
| Worked check | Limit `4+h` to `4`. | `0` confuses the increment's limit with the expression's limit; `4+h` leaves the requested limit unevaluated. |
| Choose next step | Keep `h(6+h)/h` before cancellation. | `(6+h)/h` loses a numerator factor; `6h+h^2` drops the denominator and is only the numerator. |
| Explain step | Require `h != 0`. | `h=0` gives an undefined quotient; all-real-h incorrectly includes that forbidden input. |
| Repair error | Identify L2, then choose derivative `5`. | L1 is a correct square expansion; L3 inherits L2's earlier error; `4` retains the dropped linear contribution; `3` is `f(1)`, not `f'(1)`. |
| Independent | Choose `f'(2)=10`. | `9` is `f(2)`; `12` differentiates the quadratic term but omits the linear derivative `-2`. |

Each public final-question prompt only asks for `f'(2)` and supplies no method
or answer before a response. Its linked reading remains optional. The response
is still multiple choice, so it does not establish unassisted written reasoning
or mastery.

## Source check and original authorship

Primary reference checked on 2026-09-10:

- OpenStax, *Calculus Volume 1*, [section 3.1, “Defining the Derivative”](https://openstax.org/books/calculus-volume-1/pages/3-1-defining-the-derivative), specifically the difference quotient with increment `h` (Definition, equations 3.1–3.2), derivative-at-a definition (equations 3.5–3.6), and the `x^2` expansion/cancellation examples (Examples 3.1–3.2 and 3.5–3.6). Verified: the secant/average-slope quotient uses nonzero `h`; the derivative is its limit as `h` tends to zero when the limit exists; factoring/cancellation happens before evaluating the limit.

All lesson prose, the `x^2+x` worked example, question text, answer ordering,
distractors, feedback and certificates are original Paths material. No exercise
or explanatory prose was copied from the reference. The hasty corpus is not
used as an authority for definitions or conditions.

## Checks and evidence

The required commands were executed from `/Users/kogaryu/iggy3d/paths`:

```sh
python3 -B tools/check_authoring_pilot.py --packet-only
PYTHONPATH=tools python3 -B content/authoring/parallel/calculus/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject calculus
```

Results: packet check passed (four subjects, 24 reserved questions and four
reserved readings); certificate test passed; and the content gate passed with
`stage: content_checked`, six model routes, seven decisions and fourteen wrong
choices. The certificate test covers all six roles, the seven decisions, all fourteen
distractors, the required separate worked example, an altered compiled key, an
altered reached derivative, a missing linear contribution, a wrong expansion
coefficient, and cancellation at zero. It also rejects malformed degree,
coefficient and point inputs. The final shared command is expected to create an
immutable candidate and receipt beneath `build/parallel-authoring/calculus/`:

- Candidate authoring folder: `/Users/kogaryu/iggy3d/paths/build/parallel-authoring/calculus/4b018b841bc61f4458f5c16b39f679eada8fdcb1b20912be7385c6004246538d/authoring`
- Verification receipt: `/Users/kogaryu/iggy3d/paths/build/parallel-authoring/calculus/4b018b841bc61f4458f5c16b39f679eada8fdcb1b20912be7385c6004246538d/checks/25f8aeb7b90487e09740a2ef691a8239f3889d211ac8f533c5b65c16363eb451/verification.json`

The verified source hashes were `lesson.md.in`
`1b6f272b60e9f44e65912c8aef71ae569c9c3d6a218db390883558b11b4c64fb`,
`questions.paths.md.in`
`6d2cc1f33481f99b8689766e979bd0cfeda6ace72d3bacca3a42973266d3dd17`,
`certificates.py`
`da7384d1c5d218ab7ec69d2687d7db8d4928a0894fa98dcc00fade74bf2d6cdc`,
`authoring.json`
`bf4f869a4e32310ac229347adf32332a440e792c4cbc447f8e477757bb629822`,
and the fixed `sequence.json`
`0f3e1a91ac51c6c7e5af55394e6d27849bf1fcaf626a7fec101af6411451ebfd`.

## Remaining limits and future figure

This is a text-only, quadratic pilot. It does not prove a general differentiable
function theorem, provide finite-difference evidence as a proof, teach chain or
product rules, or cover a discontinuous function. A future figure could show a
secant through `(a,f(a))` and `(a+h,f(a+h))` converging to a tangent, with `h`
controlled as a real increment. It must remain an illustrative read-only
figure before a response and must not reveal an exercise key or judge a choice.
